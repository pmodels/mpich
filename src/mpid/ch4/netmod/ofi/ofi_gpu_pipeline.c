/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "mpir_async_things.h"

struct chunk_req {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIR_Request *parent;       /* Parent request           */
    void *buf;
};

struct pipeline_header {
    int n_chunks;
    int pipeline_tag;
};

static void spawn_send_copy(MPIR_Async_thing * thing, MPIR_Request * sreq, MPIR_async_req * areq,
                            const void *buf, MPI_Aint chunk_sz);
static int start_recv_chunk(MPIR_Request * rreq, int n_chunks);
static int start_recv_copy(MPIR_Request * rreq, void *buf, MPI_Aint chunk_sz,
                           void *recv_buf, MPI_Aint count, MPI_Datatype datatype);

/* ------------------------------------
 * send_alloc: allocate send chunks and start copy, may postpone as async
 */
struct send_alloc {
    MPIR_Request *sreq;
    const void *send_buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    MPL_pointer_attr_t attr;
    MPI_Aint offset, left_sz, chunk_sz;
    int n_chunks;
};

static int send_alloc_poll(MPIR_Async_thing * thing);

int MPIDI_OFI_gpu_pipeline_send(MPIR_Request * sreq, const void *send_buf,
                                MPI_Aint count, MPI_Datatype datatype,
                                MPL_pointer_attr_t attr, MPI_Aint data_sz,
                                uint64_t cq_data, fi_addr_t remote_addr,
                                int vci_local, int ctx_idx, uint64_t match_bits, int pipeline_tag)
{
    int mpi_errno = MPI_SUCCESS;

    uint32_t n_chunks = 0;
    MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ;
    if (data_sz <= chunk_sz) {
        /* data fits in a single chunk */
        chunk_sz = data_sz;
        n_chunks = 1;
    } else {
        n_chunks = data_sz / chunk_sz;
        if (data_sz % chunk_sz > 0) {
            n_chunks++;
        }
    }

    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.num_remain) = n_chunks;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.cq_data) = cq_data;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.remote_addr) = remote_addr;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.vci_local) = vci_local;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.ctx_idx) = ctx_idx;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.match_bits) = match_bits;
    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.pipeline_tag) = pipeline_tag;

    struct pipeline_header hdr;
    hdr.n_chunks = n_chunks;
    hdr.pipeline_tag = pipeline_tag;

    /* Send the initial empty packet for matching */
    MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                        &hdr, sizeof(hdr), cq_data | MPIDI_OFI_IDATA_PIPELINE,
                                        remote_addr, match_bits), vci_local, tinjectdata);

    struct send_alloc *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    MPIR_Assert(p);

    p->sreq = sreq;
    p->send_buf = send_buf;
    p->count = count;
    p->datatype = datatype;
    p->attr = attr;
    p->left_sz = data_sz;
    p->chunk_sz = chunk_sz;
    p->offset = 0;
    p->n_chunks = 0;

    mpi_errno = MPIR_Async_things_add(send_alloc_poll, p);
    /* TODO: kick the progress right away */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
        MPI_Aint chunk_sz = MPL_MIN(p->left_sz, p->chunk_sz);
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

        spawn_send_copy(thing, p->sreq, &async_req, host_buf, chunk_sz);

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
    int vci_local = MPIDI_OFI_REQUEST(sreq, pipeline_info.send.vci_local);

    struct chunk_req *chunk_req = MPL_malloc(sizeof(struct chunk_req), MPL_MEM_BUFFER);
    MPIR_Assertp(chunk_req);

    chunk_req->parent = sreq;
    chunk_req->event_id = MPIDI_OFI_EVENT_SEND_GPU_PIPELINE;
    chunk_req->buf = (void *) buf;

    int ctx_idx = MPIDI_OFI_REQUEST(sreq, pipeline_info.send.ctx_idx);
    fi_addr_t remote_addr = MPIDI_OFI_REQUEST(sreq, pipeline_info.send.remote_addr);
    uint64_t cq_data = MPIDI_OFI_REQUEST(sreq, pipeline_info.send.cq_data);
    uint64_t match_bits = MPIDI_OFI_REQUEST(sreq, pipeline_info.send.pipeline_tag) |
        MPIDI_OFI_GPU_PIPELINE_SEND;
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
 * send_event: callback for MPIDI_OFI_dispatch_function in ofi_events.c
 */
int MPIDI_OFI_gpu_pipeline_send_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;

    struct chunk_req *chunk_req = (void *) r;
    MPIR_Request *sreq = chunk_req->parent;;
    void *host_buf = chunk_req->buf;
    MPL_free(chunk_req);

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.gpu_pipeline_send_pool, host_buf);

    MPIDI_OFI_REQUEST(sreq, pipeline_info.send.num_remain) -= 1;
    if (MPIDI_OFI_REQUEST(sreq, pipeline_info.send.num_remain) == 0) {
        MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));
        MPIDI_Request_complete_fast(sreq);
    }

    return mpi_errno;
}

/* ------------------------------------
 * recv_alloc: allocate recv chunk buffer and post fi_trecv
 *     There are actually two async things: issuing the initial recv and
 *     issuing recvs for the rest of the chunks.
 */

/* the state for recv_init_alloc is just the rreq */

static bool issue_recv_alloc(MPIR_Request * rreq, bool is_init);
static int recv_init_alloc_poll(MPIR_Async_thing * thing);

int MPIDI_OFI_gpu_pipeline_recv(MPIR_Request * rreq,
                                void *recv_buf, MPI_Aint count, MPI_Datatype datatype,
                                fi_addr_t remote_addr, int vci_local,
                                uint64_t match_bits, uint64_t mask_bits,
                                MPI_Aint data_sz, int ctx_idx)
{
    int mpi_errno = MPI_SUCCESS;

    /* The 1st recv is an empty chunk for matching. We need initialize rreq. */
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.offset) = 0;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_inrecv) = 0;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_remain) = 0;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.is_sync) = false;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.remote_addr) = remote_addr;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.vci_local) = vci_local;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.match_bits) = match_bits;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.mask_bits) = mask_bits;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.ctx_idx) = ctx_idx;

    /* Save original buf, datatype and count */
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.buf) = recv_buf;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.count) = count;
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.datatype) = datatype;

    mpi_errno = MPIR_Async_things_add(recv_init_alloc_poll, rreq);

    return mpi_errno;
}

static int recv_init_alloc_poll(MPIR_Async_thing * thing)
{
    MPIR_Request *rreq = MPIR_Async_thing_get_state(thing);

    int ret = issue_recv_alloc(rreq, true /* is_init */);
    if (ret) {
        return MPIR_ASYNC_THING_DONE;
    }

    return MPIR_ASYNC_THING_NOPROGRESS;
}

/* ---- */
struct recv_chunk_alloc {
    MPIR_Request *rreq;
    int n_chunks;
    int issued_chunks;
};

static int recv_chunk_alloc_poll(MPIR_Async_thing * thing);

/* this is called from recv_event */
static int start_recv_chunk(MPIR_Request * rreq, int n_chunks)
{
    int mpi_errno = MPI_SUCCESS;

    struct recv_chunk_alloc *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    MPIR_Assert(p);

    p->rreq = rreq;
    p->n_chunks = n_chunks;
    p->issued_chunks = 0;

    mpi_errno = MPIR_Async_things_add(recv_chunk_alloc_poll, p);

    return mpi_errno;
}

static int recv_chunk_alloc_poll(MPIR_Async_thing * thing)
{
    struct recv_chunk_alloc *p = MPIR_Async_thing_get_state(thing);
    MPIR_Request *rreq = p->rreq;

    /* arbitrary threshold */
    if (MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_inrecv) > 1) {
        return MPIR_ASYNC_THING_NOPROGRESS;
    }

    bool ret = issue_recv_alloc(rreq, false /* is_init */);
    if (ret) {
        p->issued_chunks++;
        if (p->issued_chunks == p->n_chunks) {
            MPL_free(p);
            return MPIR_ASYNC_THING_DONE;
        } else {
            return MPIR_ASYNC_THING_UPDATED;
        }
    }

    return MPIR_ASYNC_THING_NOPROGRESS;
}

/* ---- */
static bool issue_recv_alloc(MPIR_Request * rreq, bool is_init)
{
    void *host_buf;
    MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool, &host_buf);
    if (!host_buf) {
        return MPIR_ASYNC_THING_NOPROGRESS;
    }

    fi_addr_t remote_addr = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.remote_addr);
    int ctx_idx = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.ctx_idx);
    int vci = MPIDI_Request_get_vci(rreq);
    uint64_t mask_bits = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.mask_bits);

    struct chunk_req *chunk_req;
    chunk_req = MPL_malloc(sizeof(*chunk_req), MPL_MEM_BUFFER);
    MPIR_Assert(chunk_req);

    chunk_req->parent = rreq;
    chunk_req->buf = host_buf;

    uint64_t match_bits;
    if (is_init) {
        match_bits = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.match_bits);
        chunk_req->event_id = MPIDI_OFI_EVENT_RECV_GPU_PIPELINE_INIT;
    } else {
        match_bits = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.pipeline_tag) |
            MPIDI_OFI_GPU_PIPELINE_SEND;
        chunk_req->event_id = MPIDI_OFI_EVENT_RECV_GPU_PIPELINE;
    }
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    int ret = fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                       host_buf, MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ, NULL, remote_addr,
                       match_bits, mask_bits, (void *) &chunk_req->context);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    if (ret == 0) {
        MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_inrecv) += 1;
        /* chunk_req and host_buf will be freed in recv_events */
        return true;
    }
    if (ret != -FI_EAGAIN && ret != -FI_ENOMEM) {
        /* unexpected error */
        MPIR_Assert(0);
    }
    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool, host_buf);
    MPL_free(chunk_req);
    return false;
};

/* ------------------------------------
 * recv_event: callback for MPIDI_OFI_dispatch_function in ofi_events.c
 */
int MPIDI_OFI_gpu_pipeline_recv_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;

    struct chunk_req *chunk_req = (void *) r;
    int event_id = chunk_req->event_id;
    MPIR_Request *rreq = chunk_req->parent;
    void *host_buf = chunk_req->buf;

    MPL_free(chunk_req);

    void *recv_buf = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.buf);
    size_t recv_count = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.count);
    MPI_Datatype datatype = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.datatype);

    if (event_id == MPIDI_OFI_EVENT_RECV_GPU_PIPELINE_INIT) {
        rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, true);
        rreq->status.MPI_ERROR = MPIDI_OFI_idata_get_error_bits(wc->data);
        rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);

        if (unlikely(MPIDI_OFI_is_tag_sync(wc->tag))) {
            MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.is_sync) = true;
        }

        bool is_pipeline = (wc->data & MPIDI_OFI_IDATA_PIPELINE);
        if (!is_pipeline) {
            /* message from a normal send */
            MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_remain) = 1;
            mpi_errno = start_recv_copy(rreq, host_buf, wc->len, recv_buf, recv_count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            struct pipeline_header *p_hdr = host_buf;
            MPIR_Assert(p_hdr->n_chunks > 0);
            MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_remain) = p_hdr->n_chunks;
            MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.pipeline_tag) = p_hdr->pipeline_tag;
            /* There is no data in the init chunk, free the buffer */
            MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool, host_buf);
            /* Post recv for the remaining chunks. */
            mpi_errno = start_recv_chunk(rreq, p_hdr->n_chunks);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        MPIR_Assert(event_id == MPIDI_OFI_EVENT_RECV_GPU_PIPELINE);
        MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_inrecv) -= 1;
        mpi_errno = start_recv_copy(rreq, host_buf, wc->len, recv_buf, recv_count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    rreq->status.MPI_ERROR = mpi_errno;
    goto fn_exit;
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

static int start_recv_copy(MPIR_Request * rreq, void *buf, MPI_Aint chunk_sz,
                           void *recv_buf, MPI_Aint count, MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint offset = MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.offset);
    int engine_type = MPIR_CVAR_CH4_OFI_GPU_PIPELINE_H2D_ENGINE_TYPE;

    /* FIXME: current design unpacks all bytes from host buffer, overflow check is missing. */
    MPIR_async_req async_req;
    mpi_errno = MPIR_Ilocalcopy_gpu(buf, chunk_sz, MPI_BYTE, 0, NULL,
                                    recv_buf, count, datatype, offset, NULL,
                                    MPL_GPU_COPY_H2D, engine_type, 1, &async_req);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.offset) += chunk_sz;

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
    MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_remain) -= 1;
    if (MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.num_remain) == 0) {
        /* all chunks arrived and copied */
        if (unlikely(MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.is_sync))) {
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
        MPIR_STATUS_SET_COUNT(rreq->status, MPIDI_OFI_REQUEST(rreq, pipeline_info.recv.offset));

        MPIDI_Request_complete_fast(rreq);
    }

    /* Free host buffer, yaksa request and task. */
    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.gpu_pipeline_recv_pool, buf);
    return;
  fn_fail:
    MPIR_Assertp(0);
}
