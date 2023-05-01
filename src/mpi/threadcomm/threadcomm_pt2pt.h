/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef THREADCOMM_PT2PT_H_INCLUDED
#define THREADCOMM_PT2PT_H_INCLUDED

/* this is an internal header for threadcomm_pt2pt_impl.c */

typedef enum {
    MPIR_THREADCOMM_MSGTYPE_EAGER,
    MPIR_THREADCOMM_MSGTYPE_IPC,
    MPIR_THREADCOMM_MSGTYPE_SYNC,
} MPIR_Threadcomm_msgtype_t;

struct send_hdr {
    MPIR_Threadcomm_msgtype_t type;
    int src_id;
    int tag;
    int attr;
    union {
        /* eager */
        MPI_Aint data_sz;
        /* ipc */
        MPIR_Request *sreq;
    } u;
};

/* request union fields, e.g. sreq->u */
struct send_req {
    const void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int src_id;
    int dst_id;
    int tag;
    int attr;
};

struct posted_req {
    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int src_id;
    int tag;
    int attr;
};

static void threadcomm_data_copy(struct send_hdr *hdr, void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPIR_threadcomm_tls_t * p);
static void threadcomm_set_status(MPIR_Threadcomm * threadcomm, struct send_hdr *hdr,
                                  MPI_Status * status);

static MPIR_Request *threadcomm_enqueue_posted(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                               int src_id, int tag, int attr,
                                               int tid, MPIR_Request ** list);
static MPIR_Request *threadcomm_match_posted(int src_id, int tag, int attr, MPIR_Request ** list);
static void threadcomm_complete_posted(struct send_hdr *hdr, MPIR_Request * rreq,
                                       MPIR_threadcomm_tls_t * p);

static void threadcomm_enqueue_unexp(struct send_hdr *hdr, MPIR_threadcomm_unexp_t ** list);
static MPIR_threadcomm_unexp_t *threadcomm_match_unexp(int src_id, int tag, int attr,
                                                       MPIR_threadcomm_unexp_t ** list);

/* TODO: refactor transport, add queue implementation */
static int threadcomm_eager_send(int src_id, int dst_id, struct send_hdr *hdr,
                                 const void *data, MPI_Aint count, MPI_Datatype datatype,
                                 MPIR_Threadcomm * threadcomm);

static int threadcomm_progress_send(int *made_progress);
static int threadcomm_progress_recv(int *made_progress);

#ifndef MPIR_THREADCOMM_USE_FBOX
/* MPSC queue */
static void threadcomm_mpsc_enqueue(MPIR_threadcomm_queue_t * queue, MPIR_threadcomm_cell_t * cell)
{
    MPL_atomic_relaxed_store_ptr(&cell->next, NULL);

    void *tail_ptr = MPL_atomic_swap_ptr(&queue->tail, cell);
    if (tail_ptr == NULL) {
        /* queue was empty */
        MPL_atomic_store_ptr(&queue->head, cell);
    } else {
        MPL_atomic_store_ptr(&((MPIR_threadcomm_cell_t *) tail_ptr)->next, cell);
    }
}

static MPIR_threadcomm_cell_t *threadcomm_mpsc_dequeue(MPIR_threadcomm_queue_t * queue)
{
    MPIR_threadcomm_cell_t *cell = MPL_atomic_load_ptr(&queue->head);
    if (cell) {
        void *next = MPL_atomic_load_ptr(&cell->next);
        if (next != NULL) {
            /* just dequeue the head */
            MPL_atomic_store_ptr(&queue->head, next);
        } else {
            /* single element, tail == head,
             * have to make sure no enqueuing is in progress */
            MPL_atomic_store_ptr(&queue->head, NULL);
            if (MPL_atomic_cas_ptr(&queue->tail, cell, NULL) == cell) {
                /* no enqueuing in progress, we are done */
            } else {
                /* busy wait for the enqueuing to finish */
                do {
                    next = MPL_atomic_load_ptr(&cell->next);
                } while (next == NULL);
                /* then set the header */
                MPL_atomic_store_ptr(&queue->head, next);
            }
        }
    }
    return cell;
}
#endif /* !MPIR_THREADCOMM_USE_FBOX */

/* ---- The main functions: send, recv, progress ---- */

MPL_STATIC_INLINE_PREFIX
    int MPIR_Threadcomm_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                             int dst_id, int tag, MPIR_Threadcomm * threadcomm, int attr,
                             MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(threadcomm);
    MPIR_Assert(p);

    MPI_Aint data_sz;
    MPIR_Datatype_get_size_macro(datatype, data_sz);
    data_sz *= count;

    bool try_eager = (data_sz <= (MPIR_THREADCOMM_MAX_PAYLOAD - sizeof(struct send_hdr))) &&
        !(MPIR_PT2PT_ATTR_GET_SYNCFLAG(attr));
    if (try_eager) {
        struct send_hdr hdr;
        hdr.type = MPIR_THREADCOMM_MSGTYPE_EAGER;
        hdr.src_id = p->tid;
        hdr.tag = tag;
        hdr.attr = attr;
        hdr.u.data_sz = data_sz;

        int ret = threadcomm_eager_send(p->tid, dst_id, &hdr, buf, count, datatype, threadcomm);
        if (ret == 0) {
            *req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
            goto fn_exit;
        }
    }

    /* ---- do ipc ---- */
    /* prepare request */
    int pool = p->tid % MPIR_REQUEST_NUM_POOLS;
    MPIR_Request *sreq = MPIR_Request_create_from_pool_safe(MPIR_REQUEST_KIND__SEND, pool, 2);
    MPIR_Assert(sreq);
    MPIR_Assert(sizeof(struct send_req) <= sizeof(sreq->u));
    struct send_req *u = (struct send_req *) &sreq->u;
    u->buf = buf;
    u->count = count;
    u->datatype = datatype;
    u->src_id = p->tid;
    u->dst_id = dst_id;
    u->tag = tag;
    u->attr = attr;

    /* prepare send_hdr */
    struct send_hdr hdr;
    hdr.type = MPIR_THREADCOMM_MSGTYPE_IPC;
    hdr.src_id = p->tid;
    hdr.tag = tag;
    hdr.attr = attr;
    hdr.u.sreq = sreq;

    int ret = threadcomm_eager_send(p->tid, dst_id, &hdr, NULL, 0, MPI_DATATYPE_NULL, threadcomm);
    if (ret) {
        /* enqueue request */
        DL_APPEND(p->pending_list, sreq);
    }

    *req = sreq;
  fn_exit:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
    int MPIR_Threadcomm_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                             int src_id, int tag, MPIR_Threadcomm * threadcomm,
                             int attr, MPIR_Request ** req, bool has_status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(threadcomm);

    /* TODO: search mailboxes first */

    MPIR_threadcomm_unexp_t *unexp = threadcomm_match_unexp(src_id, tag, attr, &p->unexp_list);
    if (unexp) {
        struct send_hdr *hdr = (struct send_hdr *) unexp->cell;
        if (has_status) {
            int pool = p->tid % MPIR_REQUEST_NUM_POOLS;
            *req = MPIR_Request_create_from_pool_safe(MPIR_REQUEST_KIND__RECV, pool, 2);
            threadcomm_set_status(threadcomm, hdr, &(*req)->status);
            MPIR_Request_complete(*req);
        } else {
            *req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        }
        threadcomm_data_copy(hdr, buf, count, datatype, p);
        MPL_free(unexp);
    } else {
        *req = threadcomm_enqueue_posted(buf, count, datatype, src_id, tag, attr,
                                         p->tid, &p->posted_list);
        /* we need threadcomm to update status (tid -> source). Need make sure to reset it
         * to NULL before free since we don't refcount it. FIXME: this is hacky. */
        (*req)->comm = (void *) threadcomm;
    }

    return mpi_errno;
}

/* ---- internal functions ---- */

static void threadcomm_data_copy(struct send_hdr *hdr, void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPIR_threadcomm_tls_t * p)
{
    if (hdr->type == MPIR_THREADCOMM_MSGTYPE_EAGER) {
        MPI_Aint actual_unpack_bytes;
        const void *data = hdr + 1;
        MPIR_Typerep_unpack(data, hdr->u.data_sz, buf, count, datatype,
                            0, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_unpack_bytes == hdr->u.data_sz);
    } else if (hdr->type == MPIR_THREADCOMM_MSGTYPE_IPC) {
        MPIR_Request *sreq = hdr->u.sreq;
        struct send_req *sreq_u = (struct send_req *) &(sreq->u);
        MPIR_Localcopy(sreq_u->buf, sreq_u->count, sreq_u->datatype, buf, count, datatype);

        /* send ack */
        struct send_hdr tmp_hdr;
        tmp_hdr.type = MPIR_THREADCOMM_MSGTYPE_SYNC;
        tmp_hdr.src_id = p->tid;
        tmp_hdr.u.sreq = sreq;
        int ret = threadcomm_eager_send(p->tid, hdr->src_id, &tmp_hdr, NULL, 0, MPI_DATATYPE_NULL,
                                        p->threadcomm);
        if (ret) {
            /* HACK: use tag=-1 to mark as sync message */
            struct send_req *u = (struct send_req *) &sreq->u;
            u->tag = -1;
            u->dst_id = hdr->src_id;
            DL_APPEND(p->pending_list, sreq);
        }
    }
}

static void threadcomm_set_status(MPIR_Threadcomm * threadcomm, struct send_hdr *hdr,
                                  MPI_Status * status)
{
    status->MPI_SOURCE = MPIR_THREADCOMM_TID_TO_RANK(threadcomm, hdr->src_id);
    status->MPI_TAG = hdr->tag;
    status->MPI_ERROR = MPIR_PT2PT_ATTR_GET_ERRFLAG(hdr->attr);

    MPI_Aint data_sz;
    if (hdr->type == MPIR_THREADCOMM_MSGTYPE_EAGER) {
        data_sz = hdr->u.data_sz;
    } else if (hdr->type == MPIR_THREADCOMM_MSGTYPE_IPC) {
        MPIR_Request *sreq = hdr->u.sreq;
        struct send_req *sreq_u = (struct send_req *) &(sreq->u);
        MPIR_Datatype_get_size_macro(sreq_u->datatype, data_sz);
        data_sz *= sreq_u->count;
    } else {
        MPIR_Assert(0);
    }
    MPIR_STATUS_SET_COUNT((*status), data_sz);
}

static MPIR_Request *threadcomm_enqueue_posted(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                               int src_id, int tag, int attr,
                                               int tid, MPIR_Request ** list)
{
    MPIR_Request *rreq;
    int pool = tid % MPIR_REQUEST_NUM_POOLS;
    rreq = MPIR_Request_create_from_pool_safe(MPIR_REQUEST_KIND__RECV, pool, 2);
    struct posted_req *u = (void *) &rreq->u;
    u->buf = buf;
    u->count = count;
    u->datatype = datatype;
    u->src_id = src_id;
    u->tag = tag;
    u->attr = attr;

    DL_APPEND(*list, rreq);

    return rreq;
}

static MPIR_Request *threadcomm_match_posted(int src_id, int tag, int attr, MPIR_Request ** list)
{
    MPIR_Request *req = NULL;
    MPIR_Request *curr, *tmp;
    DL_FOREACH_SAFE(*list, curr, tmp) {
        struct posted_req *u = (struct posted_req *) &curr->u;
        if ((u->src_id == src_id || u->src_id == MPI_ANY_SOURCE) && u->tag == tag &&
            MPIR_PT2PT_ATTR_CONTEXT_OFFSET(u->attr) == MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr)) {
            DL_DELETE(*list, curr);
            req = curr;
            break;
        }
    }
    return req;
}

static void threadcomm_complete_posted(struct send_hdr *hdr, MPIR_Request * rreq,
                                       MPIR_threadcomm_tls_t * p)
{
    struct posted_req *u = (struct posted_req *) &rreq->u;
    threadcomm_data_copy(hdr, u->buf, u->count, u->datatype, p);
    MPIR_Assert(rreq->comm);
    threadcomm_set_status((MPIR_Threadcomm *) rreq->comm, hdr, &rreq->status);
    rreq->comm = NULL;
    MPIR_Request_complete(rreq);
}

#define UNEXP MPIR_threadcomm_unexp_t
static void threadcomm_enqueue_unexp(struct send_hdr *hdr, UNEXP ** list)
{
    MPI_Aint cell_sz = sizeof(*hdr);
    if (hdr->type == MPIR_THREADCOMM_MSGTYPE_EAGER) {
        cell_sz += hdr->u.data_sz;
    }
    UNEXP *unexp = MPL_malloc(sizeof(UNEXP) + cell_sz, MPL_MEM_OTHER);
    memcpy(unexp->cell, hdr, cell_sz);
    DL_APPEND(*list, unexp);
}

static UNEXP *threadcomm_match_unexp(int src_id, int tag, int attr, UNEXP ** list)
{
    UNEXP *req = NULL;
    UNEXP *curr, *tmp;
    DL_FOREACH_SAFE(*list, curr, tmp) {
        struct send_hdr *hdr = (struct send_hdr *) curr->cell;
        if ((hdr->src_id == src_id || src_id == MPI_ANY_SOURCE) && hdr->tag == tag &&
            MPIR_PT2PT_ATTR_CONTEXT_OFFSET(hdr->attr) == MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr)) {
            DL_DELETE(*list, curr);
            req = curr;
            break;
        }
    }
    return req;
}

static inline void threadcomm_load_cell(void *cell, struct send_hdr *hdr,
                                        const void *data, MPI_Aint count, MPI_Datatype datatype)
{
    memcpy(cell, hdr, sizeof(struct send_hdr));
    if (count > 0) {
        MPIR_Assert(hdr->type == MPIR_THREADCOMM_MSGTYPE_EAGER);
        MPI_Aint actual_pack_bytes;
        void *p = (char *) cell + sizeof(struct send_hdr);
        MPIR_Typerep_pack(data, count, datatype, 0, p, hdr->u.data_sz, &actual_pack_bytes,
                          MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_pack_bytes == hdr->u.data_sz);
    }
}

static int threadcomm_eager_send(int src_id, int dst_id, struct send_hdr *hdr,
                                 const void *data, MPI_Aint count, MPI_Datatype datatype,
                                 MPIR_Threadcomm * threadcomm)
{
#ifdef MPIR_THREADCOMM_USE_FBOX
    MPIR_threadcomm_fbox_t *fbox = MPIR_THREADCOMM_MAILBOX(threadcomm, src_id, dst_id);

    if (MPL_atomic_load_int(&fbox->u.data_ready)) {
        return -1;
    }

    threadcomm_load_cell(fbox->cell, hdr, data, count, datatype);
    MPL_atomic_store_int(&fbox->u.data_ready, 1);

#else
    MPI_Aint cell_sz = sizeof(MPIR_threadcomm_cell_t) + sizeof(struct send_hdr);
    if (hdr->type == MPIR_THREADCOMM_MSGTYPE_EAGER) {
        cell_sz += hdr->u.data_sz;
    }
    MPIR_threadcomm_cell_t *cell = threadcomm_mpsc_dequeue(&threadcomm->pools[src_id]);
    MPIR_Assert(cell);

    threadcomm_load_cell(cell->payload, hdr, data, count, datatype);

    threadcomm_mpsc_enqueue(&threadcomm->queues[dst_id], cell);

#endif
    return 0;
}

static int threadcomm_progress_send(int *made_progress)
{
#ifdef MPIR_THREADCOMM_USE_FBOX
    MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *);
    for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) {
        MPIR_Request *curr, *tmp;
        DL_FOREACH_SAFE(p[i].pending_list, curr, tmp) {
            struct send_req *u = (struct send_req *) &curr->u;
            struct send_hdr hdr;

            if (u->tag == -1) {
                /* sync message */
                hdr.type = MPIR_THREADCOMM_MSGTYPE_SYNC;
                hdr.src_id = p[i].tid;
                hdr.u.sreq = curr;
            } else {
                hdr.type = MPIR_THREADCOMM_MSGTYPE_IPC;
                hdr.src_id = p[i].tid;
                hdr.tag = u->tag;
                hdr.attr = u->attr;
                hdr.u.sreq = curr;
            }

            int ret = threadcomm_eager_send(p[i].tid, u->dst_id, &hdr,
                                            NULL, 0, MPI_DATATYPE_NULL, p[i].threadcomm);
            if (ret == 0) {
                DL_DELETE(p[i].pending_list, curr);
            } else {
                break;
            }
        }
    }
#else
    /* queues won't need pending_list sends */
#endif

    return MPI_SUCCESS;
}

static inline void threadcomm_recv_event(void *cell, MPIR_threadcomm_tls_t * p)
{
    struct send_hdr *hdr = cell;

    if (hdr->type == MPIR_THREADCOMM_MSGTYPE_SYNC) {
        /* This is sync message from receiver */
        MPIR_Request_complete(hdr->u.sreq);
        return;
    }

    MPIR_Request *rreq = threadcomm_match_posted(hdr->src_id, hdr->tag, hdr->attr, &p->posted_list);
    if (rreq) {
        threadcomm_complete_posted(hdr, rreq, p);
        rreq->status.MPI_TAG = hdr->tag;
        rreq->status.MPI_SOURCE = MPIR_THREADCOMM_TID_TO_RANK(p->threadcomm, hdr->src_id);
    } else {
        threadcomm_enqueue_unexp(hdr, &p->unexp_list);
    }
}

static int threadcomm_progress_recv(int *made_progress)
{
    MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *);
    for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) {
        int num_threads = p[i].threadcomm->num_threads;
#ifdef MPIR_THREADCOMM_USE_FBOX
        for (int j = 0; j < num_threads; j++) {
            MPIR_threadcomm_fbox_t *fbox = MPIR_THREADCOMM_MAILBOX(p[i].threadcomm, j, p[i].tid);
            if (MPL_atomic_load_int(&fbox->u.data_ready)) {
                *made_progress = 1;
                threadcomm_recv_event(fbox->cell, &p[i]);
                MPL_atomic_store_int(&fbox->u.data_ready, 0);
            }
        }
#else
        MPIR_threadcomm_cell_t *cell =
            threadcomm_mpsc_dequeue(&(p[i].threadcomm->queues[p[i].tid]));
        if (cell) {
            *made_progress = 1;
            threadcomm_recv_event(cell->payload, &p[i]);
            struct send_hdr *hdr = (void *) (cell->payload);
            threadcomm_mpsc_enqueue(&(p[i].threadcomm->pools[hdr->src_id]), cell);
        }
#endif
    }

    return MPI_SUCCESS;
}

#endif /* THREADCOMM_PT2PT_H_INCLUDED */
