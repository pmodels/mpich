/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpir_async_things.h"

/* ------------------------------------
 * send_alloc: allocate send chunks and start copy, may postpone as async
 */
struct send_alloc {
    MPIR_Request *sreq;
    const void *send_buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    MPL_pointer_attr_t attr;
    MPI_Aint offset, left_sz;
    int n_chunks;
};

static int send_alloc_poll(MPIR_Async_thing * thing);

int MPIDI_OFI_gpu_pipeline_send(MPIR_Request * sreq, const void *send_buf,
                                MPI_Aint count, MPI_Datatype datatype,
                                MPL_pointer_attr_t attr, MPI_Aint data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    struct send_alloc *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    MPIR_Assert(p);

    p->sreq = sreq;
    p->send_buf = send_buf;
    p->count = count;
    p->datatype = datatype;
    p->attr = attr;
    p->left_sz = data_sz;
    p->offset = 0;
    p->n_chunks = 0;

    mpi_errno = MPIR_Async_things_add(send_alloc_poll, p);

    return mpi_errno;
}

static int send_alloc_poll(MPIR_Async_thing * thing)
{
    int num_new_chunks = 0;
    struct send_alloc *p = MPIR_Async_thing_get_state(thing);

    while (p->left_sz > 0) {
        void *host_buf;
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.gpu_pipeline_send_pool, &host_buf);
        if (host_buf == NULL) {
            return (num_new_chunks == 0) ? MPIR_ASYNC_THING_NOPROGRESS : MPIR_ASYNC_THING_UPDATED;
        }
        MPIR_async_req async_req;
        MPI_Aint chunk_sz = MPL_MIN(p->left_sz, MPIDI_OFI_REQUEST(p->sreq, pipeline_info.chunk_sz));
        MPL_gpu_engine_type_t engine_type =
            MPIDI_OFI_gpu_get_send_engine_type(MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE);
        int commit = p->left_sz <= chunk_sz ? 1 : 0;
        if (!commit &&
            !MPIR_CVAR_GPU_USE_IMMEDIATE_COMMAND_LIST &&
            p->n_chunks % MPIR_CVAR_CH4_OFI_GPU_PIPELINE_NUM_BUFFERS_PER_CHUNK ==
            MPIR_CVAR_CH4_OFI_GPU_PIPELINE_NUM_BUFFERS_PER_CHUNK - 1)
            commit = 1;
        int mpi_errno;
        mpi_errno = MPIR_Ilocalcopy_gpu(p->send_buf, p->count, p->datatype,
                                        p->offset, &p->attr, host_buf, chunk_sz,
                                        MPI_BYTE, 0, NULL, MPL_GPU_COPY_D2H, engine_type,
                                        commit, &async_req);
        MPIR_Assertp(mpi_errno == MPI_SUCCESS);

        p->offset += (size_t) chunk_sz;
        p->left_sz -= (size_t) chunk_sz;
        p->n_chunks++;
        /* Increase request completion cnt, cc is 1 more than necessary
         * to prevent parent request being freed prematurally. */
        MPIR_cc_inc(p->sreq->cc_ptr);

        spawn_pipeline_send(thing, p->sreq, &async_req, host_buf, chunk_sz);

        num_new_chunks++;
    }
    /* all done */
    MPL_free(p);
    return MPIR_ASYNC_THING_DONE;
};

/* ------------------------------------
 * send_copy: async copy before sending the chunk data
 */
struct send_copy {
    MPIR_Request *sreq;
    /* async handle */
    MPIR_async_req async_req;
    /* for sending data */
    const void *buf;
    MPI_Aint chunk_sz;
};

static int send_copy_poll(MPIR_Async_thing * thing);
static void send_copy_complete(MPIR_Request * sreq, const void *buf, MPI_Aint chunk_sz);

static void spawn_send_copy(MPIR_Async_thing * thing,
                            MPIR_Request * sreq, MPIR_async_req * areq,
                            const void *buf, MPI_Aint chunk_sz)
{
    struct send_copy *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    MPIR_Assert(p);

    p->sreq = sreq;
    p->async_req = *areq;
    p->buf = buf;
    p->chunk_sz = chunk_sz;

    MPIR_Async_thing_spawn(thing, send_copy_poll, p);
}

static int send_copy_poll(MPIR_Async_thing * thing)
{
    int is_done = 0;

    struct send_copy *p = MPIR_Async_thing_get_state(thing);
    MPIR_async_test(&(p->async_req), &is_done);

    if (is_done) {
        /* finished copy, go ahead send the data */
        send_copy_complete(p->sreq, p->buf, p->chunk_sz);
        MPL_free(p);
        return MPIR_ASYNC_THING_DONE;
    }

    return MPIR_ASYNC_THING_NOPROGRESS;
}

static void send_copy_complete(MPIR_Request * sreq, const void *buf, MPI_Aint chunk_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int vci_local = MPIDI_OFI_REQUEST(sreq, pipeline_info.vci_local);

    MPIDI_OFI_gpu_pipeline_request *chunk_req = (MPIDI_OFI_gpu_pipeline_request *)
        MPL_malloc(sizeof(MPIDI_OFI_gpu_pipeline_request), MPL_MEM_BUFFER);
    MPIR_Assertp(chunk_req);

    chunk_req->parent = sreq;
    chunk_req->event_id = MPIDI_OFI_EVENT_SEND_GPU_PIPELINE;
    chunk_req->buf = (void *) buf;

    int ctx_idx = MPIDI_OFI_REQUEST(sreq, pipeline_info.ctx_idx);
    fi_addr_t remote_addr = MPIDI_OFI_REQUEST(sreq, pipeline_info.remote_addr);
    uint64_t cq_data = MPIDI_OFI_REQUEST(sreq, pipeline_info.cq_data);
    uint64_t match_bits = MPIDI_OFI_REQUEST(sreq, pipeline_info.match_bits);
    match_bits |= MPIDI_OFI_GPU_PIPELINE_SEND;
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_local).lock);
    MPIDI_OFI_CALL_RETRY(fi_tsenddata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                      buf, chunk_sz, NULL /* desc */ ,
                                      cq_data, remote_addr, match_bits,
                                      (void *) &chunk_req->context), vci_local, fi_tsenddata);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_local).lock);
    /* both send buffer and chunk_req will be freed in pipeline_send_event */

    return;
  fn_fail:
    MPIR_Assert(0);
}

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

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
