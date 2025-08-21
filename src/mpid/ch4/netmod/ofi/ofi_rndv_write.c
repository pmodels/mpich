/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"
#include "ofi_rndv.h"

#include "ofi_rndv_rdma_common.inc"

#define MPIDI_OFI_RNDVWRITE_INFLY_CHUNKS 10

static int rndvwrite_write_poll(MPIX_Async_thing thing);
static int async_send_copy(MPIX_Async_thing thing, MPIR_Request * sreq, int nic, uint64_t disp,
                           void *chunk_buf, MPI_Aint chunk_sz,
                           const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           MPI_Aint offset, MPL_pointer_attr_t * attr);
static int send_copy_poll(MPIX_Async_thing thing);
static int send_issue_write(MPIR_Request * sreq, void *buf, MPI_Aint data_sz,
                            int nic, MPI_Aint disp);

/* export functions:
 * int MPIDI_OFI_rndvwrite_send(MPIR_Request * sreq, int tag);
 * int MPIDI_OFI_rndvwrite_write_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
 * int MPIDI_OFI_rndvwrite_ack_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
 *
 * int MPIDI_OFI_rndvwrite_recv(MPIR_Request * rreq, int tag, int vci_src, int vci_dst);
 * int MPIDI_OFI_rndvwrite_recv_mrs_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r);
 */
/* -- sender side -- */

int MPIDI_OFI_rndvwrite_send(MPIR_Request * sreq, int tag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_rndvwrite_t *p = &MPIDI_OFI_AMREQ_WRITE(sreq);
    MPIR_FUNC_ENTER;

    if (!p->need_pack) {
        MPI_Aint true_extent, true_lb;
        MPIR_Type_get_true_extent_impl(p->datatype, &true_lb, &true_extent);
        p->u.send.u.data = MPIR_get_contig_ptr(p->buf, true_lb);
    } else {
        p->u.send.u.copy_infly = 0;
    }

    /* recv the mrs */
    int num_nics = MPIDI_OFI_global.num_nics;
    MPI_Aint hdr_sz = sizeof(struct rdma_info) + num_nics * sizeof(uint64_t);
    mpi_errno = MPIDI_OFI_RNDV_recv_hdr(sreq, MPIDI_OFI_EVENT_RNDVWRITE_RECV_MRS, hdr_sz,
                                        p->av, p->vci_local, p->vci_remote, p->match_bits);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_rndvwrite_recv_mrs_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;
    struct rdma_info *hdr = MPIDI_OFI_RNDV_GET_CONTROL_HDR(r);
    MPIR_Request *sreq = MPIDI_OFI_RNDV_GET_CONTROL_REQ(r);
    MPIDI_OFI_rndvwrite_t *p = &MPIDI_OFI_AMREQ_WRITE(sreq);

    int num_nics = MPIDI_OFI_global.num_nics;
    p->remote_data_sz = MPL_MIN(hdr->data_sz, p->data_sz);
    p->u.send.remote_base = hdr->base;
    p->u.send.rkeys = MPL_malloc(num_nics * sizeof(uint64_t), MPL_MEM_OTHER);
    for (int i = 0; i < num_nics; i++) {
        p->u.send.rkeys[i] = hdr->rkeys[i];
    }

    MPL_free(r);

    /* setup chunks */
    p->u.send.chunks_per_nic = get_chunks_per_nic(p->remote_data_sz, num_nics);
    p->u.send.chunks_remain = p->u.send.chunks_per_nic * num_nics;

    p->u.send.cur_chunk_index = 0;
    p->u.send.write_infly = 0;

    /* issue fi_write */
    mpi_errno = MPIR_Async_things_add(rndvwrite_write_poll, sreq, NULL);

    return mpi_errno;
}

static int rndvwrite_write_poll(MPIX_Async_thing thing)
{
    int ret = MPIX_ASYNC_NOPROGRESS;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = MPIR_Async_thing_get_state(thing);
    MPIDI_OFI_rndvwrite_t *p = &MPIDI_OFI_AMREQ_WRITE(sreq);

    /* CS required for genq pool and gpu imemcpy */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(p->vci_local));

    int num_nics = MPIDI_OFI_global.num_nics;
    while (p->u.send.cur_chunk_index < p->u.send.chunks_per_nic * num_nics) {
        int nic;
        MPI_Aint total_offset, nic_offset, chunk_sz;
        get_chunk_offsets(p->u.send.cur_chunk_index, num_nics,
                          p->u.send.chunks_per_nic, p->remote_data_sz,
                          &total_offset, &nic, &nic_offset, &chunk_sz);

        if (chunk_sz <= 0) {
            p->u.send.cur_chunk_index++;
            p->u.send.chunks_remain--;
            continue;
        }

        uint64_t disp;
        if (MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
            disp = p->u.send.remote_base + total_offset;
        } else {
            disp = nic_offset;
        }

        if (p->need_pack) {
            if (p->u.send.u.copy_infly >= MPIDI_OFI_RNDVWRITE_INFLY_CHUNKS) {
                goto fn_exit;
            }

            /* alloc a chunk */
            void *chunk_buf;
            MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.per_vci[p->vci_local].pipeline_pool,
                                               &chunk_buf);
            if (!chunk_buf) {
                goto fn_exit;
            }
            /* issue async copy */
            mpi_errno = async_send_copy(thing, sreq, nic, disp, chunk_buf, chunk_sz,
                                        p->buf, p->count, p->datatype, total_offset, &p->attr);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            if (p->u.send.write_infly >= MPIDI_OFI_RNDVWRITE_INFLY_CHUNKS) {
                goto fn_exit;
            }
            void *write_buf = (char *) p->u.send.u.data + total_offset;
            /* issue rdma write */
            mpi_errno = send_issue_write(sreq, write_buf, chunk_sz, nic, disp);
            MPIR_ERR_CHECK(mpi_errno);
        }
        p->u.send.cur_chunk_index++;
    }

    ret = MPIX_ASYNC_DONE;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(p->vci_local));
    return ret;
  fn_fail:
    goto fn_exit;
}

struct send_copy {
    MPIR_Request *sreq;
    MPIR_gpu_req async_req;
    void *chunk_buf;
    MPI_Aint chunk_sz;
    MPI_Aint offset;
    int nic;
    uint64_t disp;
    bool is_done;
};

static int async_send_copy(MPIX_Async_thing thing, MPIR_Request * sreq, int nic, uint64_t disp,
                           void *chunk_buf, MPI_Aint chunk_sz,
                           const void *buf, MPI_Aint count, MPI_Datatype datatype,
                           MPI_Aint offset, MPL_pointer_attr_t * attr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_gpu_req async_req;
    int engine_type = MPIDI_OFI_gpu_get_send_engine_type();
    int copy_dir = MPL_GPU_COPY_DIRECTION_NONE;
    mpi_errno = MPIR_Ilocalcopy_gpu((void *) buf, count, datatype, offset, attr,
                                    chunk_buf, chunk_sz, MPIR_BYTE_INTERNAL, 0, NULL,
                                    copy_dir, engine_type, 1, &async_req);
    MPIR_ERR_CHECK(mpi_errno);

    struct send_copy *p = MPL_malloc(sizeof(struct send_copy), MPL_MEM_OTHER);
    p->sreq = sreq;
    p->async_req = async_req;
    p->chunk_buf = chunk_buf;
    p->chunk_sz = chunk_sz;
    p->offset = offset;
    p->nic = nic;
    p->disp = disp;

    MPIR_Async_thing_spawn(thing, send_copy_poll, p, NULL);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int send_copy_poll(MPIX_Async_thing thing)
{
    struct send_copy *p = MPIR_Async_thing_get_state(thing);
    MPIDI_OFI_rndvwrite_t *q = &MPIDI_OFI_AMREQ_WRITE(p->sreq);

    if (!p->is_done) {
        int is_done = 0;
        MPIR_async_test(&(p->async_req), &is_done);
        if (is_done) {
            p->is_done = true;
            q->u.send.u.copy_infly--;
        }
    }

    if (!p->is_done) {
        return MPIX_ASYNC_NOPROGRESS;
    } else if (q->u.send.write_infly >= MPIDI_OFI_RNDVWRITE_INFLY_CHUNKS) {
        return MPIX_ASYNC_NOPROGRESS;
    } else {
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(q->vci_local));
        int mpi_errno = send_issue_write(p->sreq, p->chunk_buf, p->chunk_sz, p->nic, p->disp);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(q->vci_local));
        MPIR_Assertp(mpi_errno == MPI_SUCCESS);

        MPL_free(p);
        return MPIX_ASYNC_DONE;
    }
}

struct write_req {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    MPIR_Request *sreq;
    /* In case we need free the chunk after write */
    void *chunk_buf;
};

static int send_issue_write(MPIR_Request * sreq, void *buf, MPI_Aint data_sz,
                            int nic, MPI_Aint disp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_rndvwrite_t *p = &MPIDI_OFI_AMREQ_WRITE(sreq);

    struct write_req *t = MPL_malloc(sizeof(struct write_req), MPL_MEM_OTHER);
    MPIR_Assertp(t);

    t->event_id = MPIDI_OFI_EVENT_RNDVWRITE_WRITE_CHUNK;
    t->sreq = sreq;
    if (p->need_pack) {
        t->chunk_buf = buf;
    }

    /* control message always use nic 0 */
    int ctx_idx = MPIDI_OFI_get_ctx_index(p->vci_local, nic);
    fi_addr_t addr = MPIDI_OFI_av_to_phys(p->av, p->vci_local, nic, p->vci_remote, nic);
    uint64_t rkey = p->u.send.rkeys[nic];

    MPIDI_OFI_CALL_RETRY(fi_write(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                  buf, data_sz, NULL, addr, disp, rkey, (void *) &t->context),
                         p->vci_local, rdma_write);
    p->u.send.write_infly++;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_rndvwrite_write_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;
    struct write_req *t = (void *) r;
    MPIR_Request *sreq = t->sreq;
    MPIDI_OFI_rndvwrite_t *p = &MPIDI_OFI_AMREQ_WRITE(sreq);

    if (p->need_pack) {
        MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vci[p->vci_local].pipeline_pool,
                                          t->chunk_buf);
    }

    p->u.send.write_infly--;
    p->u.send.chunks_remain--;
    if (p->u.send.chunks_remain == 0) {
        /* done. send ack. Also inform receiver our data_sz */
        mpi_errno = MPIDI_OFI_RNDV_send_hdr(&p->data_sz, sizeof(MPI_Aint),
                                            p->av, p->vci_local, p->vci_remote, p->match_bits);
        /* complete request */
        MPL_free(p->u.send.rkeys);
        MPIR_Datatype_release_if_not_builtin(p->datatype);
        MPIDI_Request_complete_fast(sreq);
    }

    MPL_free(r);
    return mpi_errno;
}

/* -- receiver side -- */

int MPIDI_OFI_rndvwrite_recv(MPIR_Request * rreq, int tag, int vci_src, int vci_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_rndvwrite_t *p = &MPIDI_OFI_AMREQ_WRITE(rreq);
    MPIR_FUNC_ENTER;

    MPIR_Assert(!p->need_pack);

    int num_nics = MPIDI_OFI_global.num_nics;
    p->u.recv.mrs = MPL_malloc((num_nics * sizeof(struct fid_mr *)), MPL_MEM_OTHER);

    int hdr_sz = sizeof(struct rdma_info) + num_nics * sizeof(uint64_t);
    struct rdma_info *hdr = MPL_malloc(hdr_sz, MPL_MEM_OTHER);
    MPIR_Assertp(hdr);

    mpi_errno = prepare_rdma_info(p->buf, p->datatype, p->data_sz, p->vci_local,
                                  FI_REMOTE_WRITE, p->u.recv.mrs, hdr);
    MPIR_ERR_CHECK(mpi_errno);

    /* send rdma_info */
    mpi_errno = MPIDI_OFI_RNDV_send_hdr(hdr, hdr_sz,
                                        p->av, p->vci_local, p->vci_remote, p->match_bits);
    MPIR_ERR_CHECK(mpi_errno);
    MPL_free(hdr);

    /* issue recv for ack */
    mpi_errno = MPIDI_OFI_RNDV_recv_hdr(rreq, MPIDI_OFI_EVENT_RNDVWRITE_ACK,
                                        sizeof(MPI_Aint) /* remote data_sz */ ,
                                        p->av, p->vci_local, p->vci_remote, p->match_bits);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_rndvwrite_ack_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = MPIDI_OFI_RNDV_GET_CONTROL_REQ(r);
    MPIDI_OFI_rndvwrite_t *p = &MPIDI_OFI_AMREQ_WRITE(rreq);

    /* check sender data_sz */
    MPI_Aint *hdr_data_sz = MPIDI_OFI_RNDV_GET_CONTROL_HDR(r);
    MPIDI_OFI_RNDV_update_count(rreq, *hdr_data_sz);

    int num_nics = MPIDI_OFI_global.num_nics;
    for (int i = 0; i < num_nics; i++) {
        uint64_t key = fi_mr_key(p->u.recv.mrs[i]);
        MPIDI_OFI_CALL(fi_close(&p->u.recv.mrs[i]->fid), mr_unreg);
        if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            MPIDI_OFI_mr_key_free(MPIDI_OFI_LOCAL_MR_KEY, key);
        }
    }
    MPL_free(p->u.recv.mrs);
    MPL_free(r);

    /* complete rreq */
    MPIR_Datatype_release_if_not_builtin(p->datatype);
    MPIDI_Request_complete_fast(rreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
