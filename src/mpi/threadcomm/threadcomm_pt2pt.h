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

/* send packet, needed for both eager and ipc */
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

/* request union fields, e.g. sreq->u, needed for ipc */
struct send_req {
    const void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int src_id;
    int dst_id;
    int tag;
    int attr;
};

/* request union fields, e.g. rreq->u, needed for enqueue to posted_list */
struct posted_req {
    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int src_id;
    int tag;
    int attr;
};

/* cell to enqueue to unexp_list, hdr copied to cell */
struct unexp {
    struct unexp *next, *prev;
    struct send_hdr hdr;
};

static int threadcomm_eager_send(int src_id, int dst_id, struct send_hdr *hdr,
                                 const void *data, MPI_Aint count, MPI_Datatype datatype,
                                 MPIR_Threadcomm * threadcomm);

static void threadcomm_recv_event(struct send_hdr *hdr, MPIR_threadcomm_tls_t * p);
static void threadcomm_data_copy(struct send_hdr *hdr, void *buf, MPI_Aint count,
                                 MPI_Datatype datatype, MPIR_threadcomm_tls_t * p);
static void threadcomm_set_status(struct send_hdr *hdr, MPI_Status * status,
                                  MPIR_threadcomm_tls_t * p);
static void threadcomm_enqueue_posted(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                      int src_id, int tag, int attr,
                                      MPIR_Request * rreq, MPIR_threadcomm_tls_t * p);
static MPIR_Request *threadcomm_match_posted(int src_id, int tag, int attr,
                                             MPIR_threadcomm_tls_t * p);
static void threadcomm_complete_posted(struct send_hdr *hdr, MPIR_Request * rreq,
                                       MPIR_threadcomm_tls_t * p);
static void threadcomm_enqueue_unexp(struct send_hdr *hdr, MPIR_threadcomm_tls_t * p);
static struct send_hdr *threadcomm_match_unexp(int src_id, int tag, int attr,
                                               MPIR_threadcomm_tls_t * p);
static void threadcomm_complete_unexp(struct send_hdr *hdr, MPIR_threadcomm_tls_t * p);

/* NOTE: it's possible to skip vci lock if we ensure each thread access a unique pool.
 *       This is currently difficult to manage due to we dub the inter-process messages.
 */
#define MPIR_THREADCOMM_REQUEST_CREATE(req, kind, threadcomm, tls) \
    do { \
        int pool = (tls)->tid % MPIR_REQUEST_NUM_POOLS; \
        (req) = MPIR_Request_create_from_pool_safe(kind, pool, 2); \
        MPIR_ERR_CHKANDJUMP(!(req), mpi_errno, MPI_ERR_OTHER, "**nomem"); \
    } while (0)

/* ---- The main functions: send, recv, progress ---- */

MPL_STATIC_INLINE_PREFIX
    int MPIR_Threadcomm_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                             int dst_id, int tag, MPIR_Threadcomm * threadcomm,
                             int attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_COMPILE_TIME_ASSERT(sizeof(struct send_req) <= MPIR_REQUEST_UNION_SIZE);

    MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(threadcomm);
    MPIR_Assert(p);

    MPI_Aint data_sz;
    MPIR_Datatype_get_size_macro(datatype, data_sz);
    data_sz *= count;

    bool try_eager = (data_sz <= (MPIR_THREADCOMM_MAX_PAYLOAD - sizeof(struct send_hdr)));
    if (try_eager) {
        struct send_hdr hdr;
        hdr.type = MPIR_THREADCOMM_MSGTYPE_EAGER;
        hdr.src_id = p->tid;
        hdr.tag = tag;
        hdr.attr = attr;
        hdr.u.data_sz = data_sz;

        int ret = threadcomm_eager_send(p->tid, dst_id, &hdr, buf, count, datatype, threadcomm);
        if (ret == MPI_SUCCESS) {
            *req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
            goto fn_exit;
        }
    }

    /* ---- do ipc ---- */
    /* prepare sreq */
    MPIR_Request *sreq;
    MPIR_THREADCOMM_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__SEND, threadcomm, p);
    struct send_req *u = (struct send_req *) &sreq->u;
    u->buf = buf;
    u->count = count;
    u->datatype = datatype;
    u->src_id = p->tid;
    u->dst_id = dst_id;
    u->tag = tag;

    /* prepare send_hdr */
    struct send_hdr hdr;
    hdr.type = MPIR_THREADCOMM_MSGTYPE_IPC;
    hdr.src_id = p->tid;
    hdr.tag = tag;
    hdr.attr = attr;
    /* attach sreq */
    hdr.u.sreq = sreq;

    int ret = threadcomm_eager_send(p->tid, dst_id, &hdr, NULL, 0, MPI_DATATYPE_NULL, threadcomm);
    if (ret != MPI_SUCCESS) {
        /* TODO: enqueue request */
        MPIR_Assert(0);
    }

    *req = sreq;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX
    int MPIR_Threadcomm_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                             int src_id, int tag, MPIR_Threadcomm * threadcomm,
                             int attr, MPIR_Request ** req, bool has_status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_threadcomm_tls_t *p = MPIR_threadcomm_get_tls(threadcomm);

    struct send_hdr *hdr = threadcomm_match_unexp(src_id, tag, attr, p);
    if (hdr) {
        if (has_status) {
            MPIR_THREADCOMM_REQUEST_CREATE(*req, MPIR_REQUEST_KIND__RECV, threadcomm, p);
            threadcomm_set_status(hdr, &(*req)->status, p);
            MPIR_Request_complete(*req);
        } else {
            *req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        }
        threadcomm_data_copy(hdr, buf, count, datatype, p);
        threadcomm_complete_unexp(hdr, p);
    } else {
        MPIR_THREADCOMM_REQUEST_CREATE(*req, MPIR_REQUEST_KIND__RECV, threadcomm, p);
        threadcomm_enqueue_posted(buf, count, datatype, src_id, tag, attr, *req, p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int threadcomm_progress_send(int *made_progress)
{
    MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *);
    for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) {
        /* TODO: try send postponed send requests */
    }
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int threadcomm_progress_recv(int *made_progress)
{
    MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *);
    for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) {
        struct send_hdr *hdr = NULL;
        /* TODO: poll transport for incoming messages */
        if (hdr) {
            threadcomm_recv_event(hdr, &p[i]);
        }
    }
    return MPI_SUCCESS;
}

/* ---- internal functions ---- */

static int threadcomm_eager_send(int src_id, int dst_id, struct send_hdr *hdr,
                                 const void *data, MPI_Aint count, MPI_Datatype datatype,
                                 MPIR_Threadcomm * threadcomm)
{
    MPIR_Assert(0);     /* TODO */
    return MPI_SUCCESS;
}

static void threadcomm_recv_event(struct send_hdr *hdr, MPIR_threadcomm_tls_t * p)
{
    if (hdr->type == MPIR_THREADCOMM_MSGTYPE_SYNC) {
        /* This is sync message from receiver */
        MPIR_Request_complete(hdr->u.sreq);
        return;
    }

    MPIR_Request *rreq = threadcomm_match_posted(hdr->src_id, hdr->tag, hdr->attr, p);
    if (rreq) {
        threadcomm_complete_posted(hdr, rreq, p);
        threadcomm_set_status(hdr, &rreq->status, p);
    } else {
        threadcomm_enqueue_unexp(hdr, p);
    }
}

/* copy data from hdr to recv buffer, send ack unless it's eager */
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
        if (ret != MPI_SUCCESS) {
            /* TODO: enqueue sending the sync message */
            MPIR_Assert(0);
        }
    }
}

static void threadcomm_set_status(struct send_hdr *hdr, MPI_Status * status,
                                  MPIR_threadcomm_tls_t * p)
{
    status->MPI_SOURCE = MPIR_THREADCOMM_TID_TO_RANK(p->threadcomm, hdr->src_id);
    status->MPI_TAG = hdr->tag;
    status->MPI_ERROR = MPIR_PT2PT_ATTR_GET_ERRFLAG(hdr->attr);

    /* set count */
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
        data_sz = 0;
    }
    MPIR_STATUS_SET_COUNT((*status), data_sz);
}

static void threadcomm_enqueue_posted(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                      int src_id, int tag, int attr,
                                      MPIR_Request * rreq, MPIR_threadcomm_tls_t * p)
{
    MPIR_Assert(0);     /* TODO */
    return rreq;
}

static MPIR_Request *threadcomm_match_posted(int src_id, int tag, int attr,
                                             MPIR_threadcomm_tls_t * p)
{
    MPIR_Assert(0);     /* TODO */
    return NULL;
}

static void threadcomm_complete_posted(struct send_hdr *hdr, MPIR_Request * rreq,
                                       MPIR_threadcomm_tls_t * p)
{
    MPIR_Assert(0);     /* TODO */
}

static void threadcomm_enqueue_unexp(struct send_hdr *hdr, MPIR_threadcomm_tls_t * p)
{
    MPIR_Assert(0);     /* TODO */
}

static struct send_hdr *threadcomm_match_unexp(int src_id, int tag, int attr,
                                               MPIR_threadcomm_tls_t * p)
{
    MPIR_Assert(0);     /* TODO */
    return NULL;
}

static void threadcomm_complete_unexp(struct send_hdr *hdr, MPIR_threadcomm_tls_t * p)
{
    MPIR_Assert(0);     /* TODO */
}

#endif /* THREADCOMM_PT2PT_H_INCLUDED */
