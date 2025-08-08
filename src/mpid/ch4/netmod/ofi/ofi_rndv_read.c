/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"
#include "ofi_rndv.h"

#include "ofi_rndv_rdma_common.inc"

#define MPIDI_OFI_RNDVREAD_INFLY_CHUNKS 10

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

/* -- sender side -- */

int MPIDI_OFI_rndvread_send(MPIR_Request * sreq, int tag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_rndvread_t *p = &MPIDI_OFI_AMREQ_READ(sreq);
    MPIR_FUNC_ENTER;

    MPIR_Assert(!p->need_pack);

    MPI_Aint true_extent, true_lb;
    MPIR_Type_get_true_extent_impl(p->datatype, &true_lb, &true_extent);
    p->u.send.data = MPIR_get_contig_ptr(p->buf, true_lb);

    int num_nics = MPIDI_OFI_global.num_nics;
    p->u.send.mrs = MPL_malloc((num_nics * sizeof(struct fid_mr *)), MPL_MEM_OTHER);

    int hdr_sz = sizeof(struct rdma_info) + num_nics * sizeof(uint64_t);
    struct rdma_info *hdr = MPL_malloc(hdr_sz, MPL_MEM_OTHER);
    MPIR_Assertp(hdr);

    mpi_errno = prepare_rdma_info((void *) p->buf, p->datatype, p->data_sz, p->vci_local,
                                  FI_REMOTE_READ, p->u.send.mrs, hdr);
    MPIR_ERR_CHECK(mpi_errno);

    /* send rdma_info */
    mpi_errno = MPIDI_OFI_RNDV_send_hdr(hdr, hdr_sz,
                                        p->av, p->vci_local, p->vci_remote, p->match_bits);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_free(hdr);

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

    int num_nics = MPIDI_OFI_global.num_nics;
    for (int i = 0; i < num_nics; i++) {
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

    if (!p->need_pack) {
        MPI_Aint true_extent, true_lb;
        MPIR_Type_get_true_extent_impl(p->datatype, &true_lb, &true_extent);
        p->u.recv.u.data = MPIR_get_contig_ptr(p->buf, true_lb);
    } else {
        p->u.recv.u.copy_infly = 0;
    }

    /* recv the mrs */
    int num_nics = MPIDI_OFI_global.num_nics;
    MPI_Aint hdr_sz = sizeof(struct rdma_info) + num_nics * sizeof(uint64_t);
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

    int num_nics = MPIDI_OFI_global.num_nics;
    p->u.recv.remote_data_sz = hdr->data_sz;
    p->u.recv.remote_base = hdr->base;
    p->u.recv.rkeys = MPL_malloc(num_nics * sizeof(uint64_t), MPL_MEM_OTHER);
    for (int i = 0; i < num_nics; i++) {
        p->u.recv.rkeys[i] = hdr->rkeys[i];
    }

    MPL_free(r);

    /* setup chunks */
    p->u.recv.chunks_per_nic = get_chunks_per_nic(p->u.recv.remote_data_sz, num_nics);

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

    int num_nics = MPIDI_OFI_global.num_nics;
    while (p->u.recv.cur_chunk_index < p->u.recv.chunks_per_nic * num_nics) {
        if (p->u.recv.num_infly >= MPIDI_OFI_RNDVREAD_INFLY_CHUNKS) {
            return MPIX_ASYNC_NOPROGRESS;
        }
        int nic;
        MPI_Aint total_offset, nic_offset, chunk_sz;
        get_chunk_offsets(p->u.recv.cur_chunk_index, num_nics,
                          p->u.recv.chunks_per_nic, p->u.recv.remote_data_sz,
                          &total_offset, &nic, &nic_offset, &chunk_sz);

        if (chunk_sz > 0) {
            void *read_buf;
            if (p->need_pack) {
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
    if (!p->need_pack) {
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
        (!p->need_pack || p->u.recv.u.copy_infly == 0)) {
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
