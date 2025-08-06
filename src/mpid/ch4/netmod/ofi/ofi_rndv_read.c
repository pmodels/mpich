/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"
#include "ofi_rndv.h"

static int rndvread_read_poll(MPIX_Async_thing thing);
static int recv_issue_read(MPIR_Request * parent_request, int event_id,
                           void *buf, MPI_Aint data_sz, MPI_Aint offset,
                           MPIDI_av_entry_t * av, int vci_local, int vci_remote, int nic,
                           MPI_Aint remote_disp, uint64_t rkey);
static int async_recv_copy(MPIR_Request * rreq, void *chunk_buf, MPI_Aint chunk_sz,
                           void *buf, MPI_Aint count, MPI_Datatype datatype,
                           MPI_Aint offset, MPL_pointer_attr_t * attr);
static int recv_copy_poll(MPIX_Async_thing thing);
static void recv_copy_complete(MPIR_Request * rreq, void *chunk_buf, MPI_Aint chunk_sz);
static int check_recv_complete(MPIR_Request * rreq);

struct rdma_info {
    MPI_Aint data_sz;
    uint64_t base;              /* 0 unless MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS is true */
    int num_nics;               /* redundant since we assume sender/receiver agree on num_nics */
    uint64_t rkeys[];
};

/* utility routine for calculating chunks */
/* Each nic is assigned with chunks_per_nic chunks. The last nic may have less chunks */

static MPI_Aint get_chunks_per_nic(MPI_Aint data_sz, int num_nics)
{
    MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
    MPI_Aint num_chunks = data_sz / chunk_sz;
    if (chunk_sz * num_chunks < data_sz) {
        num_chunks++;
    }

    if (num_nics == 1) {
        return num_chunks;
    } else {
        MPI_Aint chunks_per_nic = num_chunks / num_nics;
        if (chunks_per_nic * num_nics < num_chunks) {
            chunks_per_nic++;
        }
        return chunks_per_nic;
    }
}

static void get_chunk_offsets(MPIDI_OFI_rndvread_t * p, MPI_Aint chunk_index,
                              MPI_Aint * total_offset_out, int *nic_out, MPI_Aint * nic_offset_out,
                              MPI_Aint * chunk_sz_out)
{
    MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
    if (p->num_nics == 1) {
        *nic_out = 0;
        *nic_offset_out = *total_offset_out = chunk_index * chunk_sz;
    } else {
        int nic = chunk_index % p->num_nics;
        MPI_Aint nic_chunk_index = chunk_index / p->u.recv.chunks_per_nic;
        *total_offset_out = (nic * p->u.recv.chunks_per_nic + nic_chunk_index) * chunk_sz;
        *nic_offset_out = nic_chunk_index * chunk_sz;
    }
    if (*total_offset_out + chunk_sz < p->u.recv.remote_data_sz) {
        *chunk_sz_out = chunk_sz;
    } else {
        /* incomplete chunks */
        *chunk_sz_out = MPL_MAX(0, p->u.recv.remote_data_sz - *total_offset_out);
    }
}

static bool need_pack(bool dt_contig, MPL_pointer_attr_t * attr)
{
    if (!dt_contig) {
        return true;
    }

    if (MPL_gpu_attr_is_dev(attr)) {
        if (!MPIDI_OFI_ENABLE_HMEM) {
            return true;
        } else if (!MPL_gpu_attr_is_strict_dev(attr)) {
            return true;
        }
    }

    return false;
}

/* -- sender side -- */

int MPIDI_OFI_rndvread_send(MPIR_Request * sreq, int tag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(sreq);
    MPIR_FUNC_ENTER;

    /* initialize request header */
    p->buf = MPIDIG_REQUEST(sreq, buffer);
    p->count = MPIDIG_REQUEST(sreq, count);
    p->datatype = MPIDIG_REQUEST(sreq, datatype);
    p->vci_local = MPIDIG_REQUEST(sreq, req->local_vci);
    p->vci_remote = MPIDIG_REQUEST(sreq, req->remote_vci);
    p->av = MPIDIU_comm_rank_to_av(sreq->comm, MPIDIG_REQUEST(sreq, u.send.dest));
    p->match_bits = MPIDI_OFI_init_sendtag(sreq->comm->context_id, 0, tag) | MPIDI_OFI_AM_SEND;

    MPI_Aint data_sz;
    MPIR_Datatype_get_size_macro(p->datatype, data_sz);
    data_sz *= p->count;

    /* assuming contig datatype */
    MPI_Aint true_extent, true_lb;
    MPIR_Type_get_true_extent_impl(p->datatype, &true_lb, &true_extent);
    p->u.send.data = MPIR_get_contig_ptr(p->buf, true_lb);

    p->data_sz = data_sz;
    p->num_nics = MPIDI_OFI_global.num_nics;
    p->u.send.mrs = MPL_malloc((p->num_nics * sizeof(struct fid_mr *)), MPL_MEM_OTHER);

    /* prepare mr and rkey */
    uint64_t rkeys[MPIDI_OFI_MAX_NICS];
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* Set up a memory region for the lmt data transfer */
        for (int i = 0; i < p->num_nics; i++) {
            rkeys[i] = MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
            MPIR_ERR_CHKANDJUMP(rkeys[i] == MPIDI_OFI_INVALID_MR_KEY, mpi_errno,
                                MPI_ERR_OTHER, "**ofid_mr_key");
        }
    } else {
        /* zero them to avoid warnings */
        for (int i = 0; i < p->num_nics; i++) {
            rkeys[i] = 0;
        }
    }
    MPI_Aint chunks_per_nic = get_chunks_per_nic(p->data_sz, p->num_nics);
    MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
    MPI_Aint sz_per_nic = chunks_per_nic * chunk_sz;
    for (int i = 0; i < p->num_nics; i++) {
        MPIDI_OFI_context_t *ctx = &MPIDI_OFI_global.ctx[MPIDI_OFI_get_ctx_index(p->vci_local, i)];
        /* note: fi_mr_reg is expensive, distribute over num_nics */
        void *data = (char *) p->u.send.data + i * sz_per_nic;
        MPI_Aint sz = (i != p->num_nics - 1) ? sz_per_nic : (p->data_sz - i * sz_per_nic);
        MPIDI_OFI_CALL(fi_mr_reg(ctx->domain, data, sz, FI_REMOTE_READ, 0ULL,
                                 rkeys[i], 0ULL, &p->u.send.mrs[i], NULL), mr_reg);
        mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], p->u.send.mrs[i], ctx->ep,
                                      NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }
    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        for (int i = 0; i < p->num_nics; i++) {
            rkeys[i] = fi_mr_key(p->u.send.mrs[i]);
        }
    }

    /* prepare rdma_info */
    int hdr_sz = sizeof(struct rdma_info) + p->num_nics * sizeof(uint64_t);
    struct rdma_info *hdr = MPL_malloc(hdr_sz, MPL_MEM_OTHER);
    MPIR_Assertp(hdr);

    hdr->data_sz = p->data_sz;
    hdr->base = (uintptr_t) p->u.send.data;
    hdr->num_nics = p->num_nics;
    for (int i = 0; i < p->num_nics; i++) {
        hdr->rkeys[i] = rkeys[i];
    }

    /* send rdma_info */
    mpi_errno = MPIDI_OFI_RNDV_send_hdr(hdr, hdr_sz,
                                        p->av, p->vci_local, p->vci_remote, p->match_bits);
    MPIR_ERR_CHECK(mpi_errno);

    /* issue recv for ack */
    mpi_errno = MPIDI_OFI_RNDV_recv_hdr(sreq, MPIDI_OFI_EVENT_RNDVREAD_ACK, 0,
                                        p->av, p->vci_local, p->vci_remote, p->match_bits);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_rndvread_ack_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = MPIDI_OFI_RNDV_GET_CONTROL_REQ(r);
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(sreq);

    for (int i = 0; i < p->num_nics; i++) {
        uint64_t key = fi_mr_key(p->u.send.mrs[i]);
        MPIDI_OFI_CALL(fi_close(&p->u.send.mrs[i]->fid), mr_unreg);
        if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            MPIDI_OFI_mr_key_free(MPIDI_OFI_LOCAL_MR_KEY, key);
        }
    }
    MPL_free(p->u.send.mrs);
    MPL_free(r);

    /* complete sreq */
    MPIR_Datatype_release_if_not_builtin(p->datatype);
    MPIDI_Request_complete_fast(sreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- receiver side -- */

int MPIDI_OFI_rndvread_recv(MPIR_Request * rreq, int tag, int vci_src, int vci_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(rreq);
    MPIR_FUNC_ENTER;

    p->buf = MPIDIG_REQUEST(rreq, buffer);
    p->count = MPIDIG_REQUEST(rreq, count);
    p->datatype = MPIDIG_REQUEST(rreq, datatype);
    p->vci_local = vci_dst;
    p->vci_remote = vci_src;
    p->av = MPIDIU_comm_rank_to_av(rreq->comm, rreq->status.MPI_SOURCE);
    p->match_bits = MPIDI_OFI_init_sendtag(rreq->comm->recvcontext_id, 0, tag) | MPIDI_OFI_AM_SEND;
    p->num_nics = MPIDI_OFI_global.num_nics;

    MPIR_GPU_query_pointer_attr(p->buf, &p->attr);

    MPIR_Datatype_get_size_macro(p->datatype, p->data_sz);
    p->data_sz *= p->count;

    int dt_contig;
    MPIR_Datatype_is_contig(p->datatype, &dt_contig);
    p->u.recv.need_pack = need_pack(dt_contig, &p->attr);

    if (!p->u.recv.need_pack) {
        MPI_Aint true_extent, true_lb;
        MPIR_Type_get_true_extent_impl(p->datatype, &true_lb, &true_extent);
        p->u.recv.u.data = MPIR_get_contig_ptr(p->buf, true_lb);
    } else {
        p->u.recv.u.copy_infly = 0;
    }

    /* recv the mrs */
    MPI_Aint hdr_sz = sizeof(struct rdma_info) + p->num_nics * sizeof(uint64_t);
    mpi_errno = MPIDI_OFI_RNDV_recv_hdr(rreq, MPIDI_OFI_EVENT_RNDVREAD_RECV_MRS, hdr_sz,
                                        p->av, p->vci_local, p->vci_remote, p->match_bits);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_rndvread_recv_mrs_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;
    struct rdma_info *hdr = MPIDI_OFI_RNDV_GET_CONTROL_HDR(r);
    MPIR_Request *rreq = MPIDI_OFI_RNDV_GET_CONTROL_REQ(r);
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(rreq);

    MPIR_Assert(hdr->num_nics == p->num_nics);
    p->u.recv.remote_data_sz = hdr->data_sz;
    p->u.recv.remote_base = hdr->base;
    p->u.recv.rkeys = MPL_malloc(p->num_nics * sizeof(uint64_t), MPL_MEM_OTHER);
    for (int i = 0; i < p->num_nics; i++) {
        p->u.recv.rkeys[i] = hdr->rkeys[i];
    }

    MPL_free(r);

    /* setup chunks */
    p->u.recv.chunks_per_nic = get_chunks_per_nic(p->u.recv.remote_data_sz, p->num_nics);

    p->u.recv.cur_chunk_index = 0;
    p->u.recv.num_infly = 0;

    /* issue fi_read */
    mpi_errno = MPIR_Async_things_add(rndvread_read_poll, rreq, NULL);

    return mpi_errno;
}

static int rndvread_read_poll(MPIX_Async_thing thing)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = MPIR_Async_thing_get_state(thing);
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(rreq);

    while (p->u.recv.cur_chunk_index < p->u.recv.chunks_per_nic * p->num_nics) {
        if (p->u.recv.num_infly >= 10) {
            return MPIX_ASYNC_NOPROGRESS;
        }
        int nic;
        MPI_Aint total_offset, nic_offset, chunk_sz;
        get_chunk_offsets(p, p->u.recv.cur_chunk_index,
                          &total_offset, &nic, &nic_offset, &chunk_sz);

        if (chunk_sz > 0) {
            void *read_buf;
            if (p->u.recv.need_pack) {
                /* alloc a chunk */
                MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.
                                                   per_vci[p->vci_local].pipeline_pool, &read_buf);
                if (!read_buf) {
                    return MPIX_ASYNC_NOPROGRESS;
                }
            } else {
                read_buf = (char *) p->u.recv.u.data + total_offset;
            }
            uint64_t disp;
            if (MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
                disp = p->u.recv.remote_base + total_offset;
            } else {
                disp = nic_offset;
            }
            mpi_errno = recv_issue_read(rreq, MPIDI_OFI_EVENT_RNDVREAD_READ_CHUNK,
                                        read_buf, chunk_sz, total_offset,
                                        p->av, p->vci_local, p->vci_remote, nic, disp,
                                        p->u.recv.rkeys[nic]);
            MPIR_ERR_CHECK(mpi_errno);
            p->u.recv.num_infly++;
        }
        p->u.recv.cur_chunk_index++;
    }

    p->u.recv.all_issued = true;
    return MPIX_ASYNC_DONE;
  fn_fail:
    return MPIX_ASYNC_NOPROGRESS;
}

struct read_req {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    MPIR_Request *rreq;
    /* In case we need unpack after read */
    void *chunk_buf;
    MPI_Aint chunk_sz;
    MPI_Aint offset;
};

static int recv_issue_read(MPIR_Request * parent_request, int event_id,
                           void *buf, MPI_Aint data_sz, MPI_Aint offset,
                           MPIDI_av_entry_t * av, int vci_local, int vci_remote, int nic,
                           MPI_Aint remote_disp, uint64_t rkey)
{
    int mpi_errno = MPI_SUCCESS;

    struct read_req *r = MPL_malloc(sizeof(struct read_req), MPL_MEM_OTHER);
    MPIR_Assertp(r);

    r->event_id = event_id;
    r->rreq = parent_request;
    r->chunk_buf = buf;
    r->chunk_sz = data_sz;
    r->offset = offset;

    /* control message always use nic 0 */
    int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, nic);
    fi_addr_t addr = MPIDI_OFI_av_to_phys(av, vci_local, nic, vci_remote, nic);

    MPIDI_OFI_CALL_RETRY(fi_read(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                 buf, data_sz, NULL,
                                 addr, remote_disp, rkey, (void *) &r->context),
                         vci_local, rdma_readfrom);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_rndvread_read_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;
    struct read_req *t = (void *) r;
    MPIR_Request *rreq = t->rreq;
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(rreq);

    p->u.recv.num_infly--;
    if (!p->u.recv.need_pack) {
        check_recv_complete(rreq);
    } else {
        /* async copy */
        mpi_errno = async_recv_copy(rreq, t->chunk_buf, t->chunk_sz,
                                    (void *) p->buf, p->count, p->datatype, t->offset, &p->attr);
        p->u.recv.u.copy_infly++;

    }
    MPL_free(r);
    return mpi_errno;
}

struct recv_copy {
    MPIR_Request *rreq;
    MPIR_gpu_req async_req;
    void *chunk_buf;
    MPI_Aint chunk_sz;
};

static int async_recv_copy(MPIR_Request * rreq, void *chunk_buf, MPI_Aint chunk_sz,
                           void *buf, MPI_Aint count, MPI_Datatype datatype,
                           MPI_Aint offset, MPL_pointer_attr_t * attr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_gpu_req async_req;
    int engine_type = MPIDI_OFI_gpu_get_recv_engine_type();
    int copy_dir = MPL_GPU_COPY_DIRECTION_NONE;
    mpi_errno = MPIR_Ilocalcopy_gpu(chunk_buf, chunk_sz, MPIR_BYTE_INTERNAL, 0, NULL,
                                    buf, count, datatype, offset, attr,
                                    copy_dir, engine_type, 1, &async_req);
    MPIR_ERR_CHECK(mpi_errno);

    struct recv_copy *p = MPL_malloc(sizeof(struct recv_copy), MPL_MEM_OTHER);
    p->rreq = rreq;
    p->async_req = async_req;
    p->chunk_buf = chunk_buf;
    p->chunk_sz = chunk_sz;

    mpi_errno = MPIR_Async_things_add(recv_copy_poll, p, NULL);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int recv_copy_poll(MPIX_Async_thing thing)
{
    struct recv_copy *p = MPIR_Async_thing_get_state(thing);

    int is_done = 0;
    MPIR_async_test(&(p->async_req), &is_done);

    if (!is_done) {
        return MPIX_ASYNC_NOPROGRESS;
    } else {
        recv_copy_complete(p->rreq, p->chunk_buf, p->chunk_sz);
        MPL_free(p);
        return MPIX_ASYNC_DONE;
    }
}

static void recv_copy_complete(MPIR_Request * rreq, void *chunk_buf, MPI_Aint chunk_sz)
{
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(rreq);

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vci[p->vci_local].pipeline_pool,
                                      chunk_buf);

    p->u.recv.u.copy_infly--;
    check_recv_complete(rreq);
}

static int check_recv_complete(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(rreq);
    if (p->u.recv.all_issued && p->u.recv.num_infly == 0 &&
        (!p->u.recv.need_pack || p->u.recv.u.copy_infly == 0)) {
        /* done. send ack */
        mpi_errno = MPIDI_OFI_RNDV_send_hdr(NULL, 0, p->av, p->vci_local, p->vci_remote,
                                            p->match_bits);
        /* complete request */
        MPL_free(p->u.recv.rkeys);
        MPIR_Datatype_release_if_not_builtin(p->datatype);
        MPIDI_Request_complete_fast(rreq);
    }
    return mpi_errno;
}
