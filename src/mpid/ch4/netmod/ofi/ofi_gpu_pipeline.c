/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpir_async_things.h"

/* ------------------------------------
 * recv_copy: async copy from host_buf to user buffer in recv event
 */
struct recv_copy {
    MPIR_Request *rreq;
    /* async handle */
    MPIR_async_req async_req;
    /* for cleanups */
    void *buf;
};

static int recv_copy_poll(MPIR_Async_thing * thing);
static void recv_copy_complete(MPIR_Request * rreq, void *buf);

int MPIDI_OFI_gpu_pipeline_recv_copy(MPIR_Request * rreq, void *buf, MPI_Aint chunk_sz,
                                     void *recv_buf, MPI_Aint count, MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint offset = MPIDI_OFI_REQUEST(rreq, pipeline_info.offset);
    int engine_type = MPIR_CVAR_CH4_OFI_GPU_PIPELINE_H2D_ENGINE_TYPE;

    /* FIXME: current design unpacks all bytes from host buffer, overflow check is missing. */
    MPIR_async_req async_req;
    mpi_errno = MPIR_Ilocalcopy_gpu(buf, chunk_sz, MPI_BYTE, 0, NULL,
                                    recv_buf, count, datatype, offset, NULL,
                                    MPL_GPU_COPY_H2D, engine_type, 1, &async_req);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_REQUEST(rreq, pipeline_info.offset) += chunk_sz;

    struct recv_copy *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    MPIR_Assert(p);

    p->rreq = rreq;
    p->async_req = async_req;
    p->buf = buf;

    mpi_errno = MPIR_Async_things_add(recv_copy_poll, p);

    return mpi_errno;
}

static int recv_copy_poll(MPIR_Async_thing * thing)
{
    int is_done = 0;

    struct recv_copy *p = MPIR_Async_thing_get_state(thing);
    MPIR_async_test(&(p->async_req), &is_done);

    if (is_done) {
        recv_copy_complete(p->rreq, p->buf);
        MPL_free(p);
        return MPIR_ASYNC_THING_DONE;
    }

    return MPIR_ASYNC_THING_NOPROGRESS;
}

static void recv_copy_complete(MPIR_Request * rreq, void *buf)
{
    int mpi_errno = MPI_SUCCESS;
    int c;
    MPIR_cc_decr(rreq->cc_ptr, &c);
    if (c == 0) {
        /* all chunks arrived and copied */
        if (unlikely(MPIDI_OFI_REQUEST(rreq, pipeline_info.is_sync))) {
            MPIR_Comm *comm = rreq->comm;
            uint64_t ss_bits =
                MPIDI_OFI_init_sendtag(MPL_atomic_relaxed_load_int
                                       (&MPIDI_OFI_REQUEST(rreq, util_id)),
                                       MPIR_Comm_rank(comm), rreq->status.MPI_TAG,
                                       MPIDI_OFI_SYNC_SEND_ACK);
            int r = rreq->status.MPI_SOURCE;
            int vci_src = MPIDI_get_vci(SRC_VCI_FROM_RECVER, comm, r, comm->rank,
                                        rreq->status.MPI_TAG);
            int vci_dst = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, r, comm->rank,
                                        rreq->status.MPI_TAG);
            int vci_local = vci_dst;
            int vci_remote = vci_src;
            int nic = 0;
            int ctx_idx = MPIDI_OFI_get_ctx_index(vci_local, nic);
            fi_addr_t dest_addr = MPIDI_OFI_comm_to_phys(comm, r, nic, vci_remote);
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_local).lock);
            MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx, NULL /* buf */ ,
                                                0 /* len */ ,
                                                MPIR_Comm_rank(comm), dest_addr, ss_bits),
                                 vci_local, tinjectdata);
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_local).lock);
        }

        MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(rreq, datatype));
        /* Set number of bytes in status. */
        MPIR_STATUS_SET_COUNT(rreq->status, MPIDI_OFI_REQUEST(rreq, pipeline_info.offset));

        MPIR_Request_free(rreq);
    }

    /* Free host buffer, yaksa request and task. */
    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool, buf);
    return;
  fn_fail:
    MPIR_Assertp(0);
}
