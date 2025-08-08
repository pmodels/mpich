/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_rndv.h"

#define MPIDI_OFI_PIPILINE_INFLY_CHUNKS 10

struct send_chunk_req {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    MPI_Aint chunk_sz;
    void *chunk_buf;
    struct iovec iov;
    MPIR_Request *sreq;
};

struct recv_chunk_req {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    MPI_Aint chunk_sz;
    void *chunk_buf;
    MPIR_Request *rreq;
    MPI_Aint recv_offset;       /* need remember where to copy chunk */
};

static int pipeline_send_poll(MPIX_Async_thing thing);
static void spawn_send_copy(MPIX_Async_thing thing, MPIR_Request * sreq, MPIR_gpu_req * areq,
                            int chunk_index, void *chunk_buf, MPI_Aint chunk_sz);
static int send_copy_poll(MPIX_Async_thing thing);
static bool send_copy_complete(MPIR_Request * sreq, int chunk_index,
                               void *chunk_buf, MPI_Aint chunk_sz);
static void send_chunk_complete(MPIR_Request * sreq, void *chunk_buf, MPI_Aint chunk_sz);

static int pipeline_recv_poll(MPIX_Async_thing thing);
static void recv_chunk_copy(MPIR_Request * rreq, void *chunk_buf, MPI_Aint chunk_sz,
                            MPI_Aint offset);
static void add_recv_copy(MPIR_Request * rreq, MPIR_gpu_req * areq, void *chunk_buf,
                          MPI_Aint chunk_sz);
static int recv_copy_poll(MPIX_Async_thing thing);
static void recv_copy_complete(MPIR_Request * sreq, void *chunk_buf, MPI_Aint chunk_sz);

/* NOTE: fields cached or duplicated in MPIDI_OFI_pipeline_t for clarity.
 *       We can optimize if we are concerned with the overhead or request header size.
 */
int MPIDI_OFI_pipeline_send(MPIR_Request * sreq, int tag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(sreq);

    MPIDI_OFI_RNDV_INIT_COMMON_SEND;

    p->remain_sz = data_sz;
    p->chunk_index = 0;
    p->u.send.copy_offset = 0;
    p->u.send.copy_infly = 0;   /* control to avoid overwhelming async progress */
    p->u.send.send_infly = 0;   /* control to avoid overwhelming unexpected recv */

    mpi_errno = MPIR_Async_things_add(pipeline_send_poll, sreq, NULL);

    /* poke progress? */

    return mpi_errno;
}

int MPIDI_OFI_pipeline_recv(MPIR_Request * rreq, int tag, int vci_src, int vci_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(rreq);

    MPIDI_OFI_RNDV_INIT_COMMON_RECV;

    p->remain_sz = data_sz;
    p->chunk_index = 0;
    p->u.recv.recv_offset = 0;
    p->u.recv.recv_infly = 0;   /* just need enough to match infly send */

    mpi_errno = MPIR_Async_things_add(pipeline_recv_poll, rreq, NULL);

    /* poke progress? */

    return mpi_errno;
}

/* callback from MPIDI_OFI_dispatch_function in ofi_events.c */
int MPIDI_OFI_pipeline_send_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;

    struct send_chunk_req *chunk_req = (void *) r;
    send_chunk_complete(chunk_req->sreq, chunk_req->chunk_buf, chunk_req->chunk_sz);
    MPL_free(chunk_req);

    return mpi_errno;
}

int MPIDI_OFI_pipeline_recv_chunk_event(struct fi_cq_tagged_entry *wc, MPIR_Request * r)
{
    int mpi_errno = MPI_SUCCESS;

    struct recv_chunk_req *chunk_req = (void *) r;
    recv_chunk_copy(chunk_req->rreq, chunk_req->chunk_buf, chunk_req->chunk_sz,
                    chunk_req->recv_offset);
    MPL_free(chunk_req);

    return mpi_errno;
}

/* ---- static routines for send side ---- */

/* async send chunks until done */
static int pipeline_send_poll(MPIX_Async_thing thing)
{
    MPIR_Request *sreq = MPIR_Async_thing_get_state(thing);
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(sreq);

    while (p->u.send.copy_offset < p->data_sz) {
        /* limit copy_infly so it doesn't overwhelm async progress */
        if (p->u.send.copy_infly >= MPIDI_OFI_PIPILINE_INFLY_CHUNKS) {
            return MPIX_ASYNC_NOPROGRESS;
        }

        void *chunk_buf;
        MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
        if (chunk_sz > p->data_sz - p->u.send.copy_offset) {
            chunk_sz = p->data_sz - p->u.send.copy_offset;
        }

        /* alloc a chunk */
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.per_vci[p->vci_local].pipeline_pool,
                                           &chunk_buf);
        if (!chunk_buf) {
            return MPIX_ASYNC_NOPROGRESS;
        }

        /* async copy */
        MPIR_gpu_req async_req;
        int mpi_errno;
        int engine_type = MPIDI_OFI_gpu_get_send_engine_type();
        int copy_dir = MPL_GPU_COPY_DIRECTION_NONE;
        mpi_errno = MPIR_Ilocalcopy_gpu(p->buf, p->count, p->datatype,
                                        p->u.send.copy_offset, &p->attr, chunk_buf, chunk_sz,
                                        MPIR_BYTE_INTERNAL, 0, NULL, copy_dir, engine_type,
                                        1, &async_req);
        MPIR_Assertp(mpi_errno == MPI_SUCCESS);
        spawn_send_copy(thing, sreq, &async_req, p->chunk_index, chunk_buf, chunk_sz);
        p->chunk_index++;
        p->u.send.copy_offset += chunk_sz;
        p->u.send.copy_infly++;
    }

    return MPIX_ASYNC_DONE;
}

/* ---- send_copy ---- */
struct send_copy {
    MPIR_Request *sreq;
    /* async handle */
    bool copy_done;
    MPIR_gpu_req async_req;
    /* for sending data */
    int chunk_index;
    void *chunk_buf;
    MPI_Aint chunk_sz;
};

static void spawn_send_copy(MPIX_Async_thing thing, MPIR_Request * sreq, MPIR_gpu_req * areq,
                            int chunk_index, void *chunk_buf, MPI_Aint chunk_sz)
{
    struct send_copy *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    MPIR_Assert(p);

    p->sreq = sreq;
    p->copy_done = false;
    p->async_req = *areq;
    p->chunk_index = chunk_index;
    p->chunk_buf = chunk_buf;
    p->chunk_sz = chunk_sz;

    MPIR_Async_thing_spawn(thing, send_copy_poll, p, NULL);
}

static int send_copy_poll(MPIX_Async_thing thing)
{
    struct send_copy *p = MPIR_Async_thing_get_state(thing);

    /* this async task contains two parts: copy and send. Use the copy_done field to allow each part to be pending */
    int is_done = p->copy_done;
    if (!is_done) {
        MPIR_async_test(&(p->async_req), &is_done);
        if (is_done) {
            MPIDI_OFI_pipeline_t *p_req = &MPIDI_OFI_AMREQ_PIPELINE(p->sreq);
            p_req->u.send.copy_infly--;
            p->copy_done = true;
        }
    }

    if (!is_done) {
        return MPIX_ASYNC_NOPROGRESS;
    } else {
        if (send_copy_complete(p->sreq, p->chunk_index, p->chunk_buf, p->chunk_sz)) {
            MPL_free(p);
            return MPIX_ASYNC_DONE;
        } else {
            /* We can't send the chunk for some reason. We'll try again */
            return MPIX_ASYNC_NOPROGRESS;
        }
    }
}

static bool send_copy_complete(MPIR_Request * sreq, int chunk_index,
                               void *chunk_buf, MPI_Aint chunk_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(sreq);

    if (p->u.send.send_infly >= MPIDI_OFI_PIPILINE_INFLY_CHUNKS) {
        return false;
    }

    struct send_chunk_req *chunk_req = MPL_malloc(sizeof(struct send_chunk_req), MPL_MEM_BUFFER);
    MPIR_Assertp(chunk_req);

    chunk_req->event_id = MPIDI_OFI_EVENT_PIPELINE_SEND_CHUNK;
    chunk_req->sreq = sreq;
    chunk_req->chunk_buf = (void *) chunk_buf;
    chunk_req->chunk_sz = chunk_sz;
    chunk_req->iov.iov_base = chunk_buf;
    chunk_req->iov.iov_len = chunk_sz;

    /* send */
    int nic = chunk_index % MPIDI_OFI_global.num_nics;
    int ctx_idx = MPIDI_OFI_get_ctx_index(p->vci_local, nic);
    fi_addr_t addr = MPIDI_OFI_av_to_phys(p->av, p->vci_local, nic, p->vci_remote, nic);

    struct fi_msg_tagged msg;
    msg.msg_iov = &chunk_req->iov;
    msg.desc = NULL;
    msg.iov_count = 1;
    msg.addr = addr;
    msg.tag = p->match_bits;
    msg.context = (void *) &(chunk_req->context);
    msg.data = 0;

    uint64_t flags = FI_COMPLETION | FI_MATCH_COMPLETE | FI_DELIVERY_COMPLETE;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(p->vci_local));
    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[ctx_idx].tx, &msg, flags),
                         p->vci_local, tsendv);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(p->vci_local));

    p->u.send.send_infly++;
    /* both send buffer and chunk_req will be freed in pipeline_send_event */

    return true;
  fn_fail:
    MPIR_Assert(0);
    return false;
}

static void send_chunk_complete(MPIR_Request * sreq, void *chunk_buf, MPI_Aint chunk_sz)
{
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(sreq);

    p->u.send.send_infly--;
    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vci[p->vci_local].pipeline_pool,
                                      chunk_buf);

    p->remain_sz -= chunk_sz;
    if (p->remain_sz == 0) {
        MPIR_Datatype_release_if_not_builtin(p->datatype);
        MPIDI_Request_complete_fast(sreq);
    }
}

/* ---- static routines for recv side ---- */

/* async post recv chunks until done */
static int pipeline_recv_poll(MPIX_Async_thing thing)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = MPIR_Async_thing_get_state(thing);
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(rreq);

    while (p->u.recv.recv_offset < p->data_sz) {
        /* only need issue enough recv_infly to match send_infly */
        if (p->u.recv.recv_infly >= MPIDI_OFI_PIPILINE_INFLY_CHUNKS) {
            return MPIX_ASYNC_NOPROGRESS;
        }

        void *chunk_buf;
        MPI_Aint chunk_sz = MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ;
        if (chunk_sz > p->data_sz - p->u.recv.recv_offset) {
            chunk_sz = p->data_sz - p->u.recv.recv_offset;
        }

        /* alloc a chunk */
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.per_vci[p->vci_local].pipeline_pool,
                                           &chunk_buf);
        if (!chunk_buf) {
            return MPIX_ASYNC_NOPROGRESS;
        }

        struct recv_chunk_req *chunk_req =
            MPL_malloc(sizeof(struct recv_chunk_req), MPL_MEM_BUFFER);
        MPIR_Assertp(chunk_req);

        chunk_req->event_id = MPIDI_OFI_EVENT_PIPELINE_RECV_CHUNK;
        chunk_req->rreq = rreq;
        chunk_req->chunk_buf = (void *) chunk_buf;
        chunk_req->chunk_sz = chunk_sz;
        chunk_req->recv_offset = p->u.recv.recv_offset;

        /* post recv */
        int nic = p->chunk_index % MPIDI_OFI_global.num_nics;
        int ctx_idx = MPIDI_OFI_get_ctx_index(p->vci_local, nic);
        fi_addr_t addr = MPIDI_OFI_av_to_phys(p->av, p->vci_local, nic, p->vci_remote, nic);

        MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                      chunk_buf, chunk_sz, NULL, addr, p->match_bits, 0,
                                      (void *) &chunk_req->context), p->vci_local, trecv);
        p->chunk_index++;
        p->u.recv.recv_offset += chunk_sz;
        p->u.recv.recv_infly++;
    }

    return MPIX_ASYNC_DONE;
  fn_fail:
    MPIR_Assert(0);
    return MPIX_ASYNC_NOPROGRESS;
}

static void recv_chunk_copy(MPIR_Request * rreq, void *chunk_buf, MPI_Aint chunk_sz,
                            MPI_Aint offset)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(rreq);
    p->u.recv.recv_infly--;

    MPIR_gpu_req async_req;
    int engine_type = MPIDI_OFI_gpu_get_recv_engine_type();
    int copy_dir = MPL_GPU_COPY_DIRECTION_NONE;
    mpi_errno = MPIR_Ilocalcopy_gpu(chunk_buf, chunk_sz, MPIR_BYTE_INTERNAL, 0, NULL,
                                    (void *) p->buf, p->count, p->datatype, offset, &p->attr,
                                    copy_dir, engine_type, 1, &async_req);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);

    add_recv_copy(rreq, &async_req, chunk_buf, chunk_sz);
}

/* async recv copy */
struct recv_copy {
    MPIR_Request *rreq;
    MPIR_gpu_req async_req;
    void *chunk_buf;
    MPI_Aint chunk_sz;
};

static void add_recv_copy(MPIR_Request * rreq, MPIR_gpu_req * areq,
                          void *chunk_buf, MPI_Aint chunk_sz)
{
    struct recv_copy *p;
    p = MPL_malloc(sizeof(*p), MPL_MEM_OTHER);
    MPIR_Assert(p);

    p->rreq = rreq;
    p->async_req = *areq;
    p->chunk_buf = chunk_buf;
    p->chunk_sz = chunk_sz;

    MPIR_Async_things_add(recv_copy_poll, p, NULL);
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
    MPIDI_OFI_pipeline_t *p = &MPIDI_OFI_AMREQ_PIPELINE(rreq);

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vci[p->vci_local].pipeline_pool,
                                      chunk_buf);

    p->remain_sz -= chunk_sz;
    if (p->remain_sz == 0) {
        MPIR_Datatype_release_if_not_builtin(p->datatype);
        MPIDI_Request_complete_fast(rreq);
    }
}
