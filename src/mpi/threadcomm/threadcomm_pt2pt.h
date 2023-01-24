/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef THREADCOMM_PT2PT_H_INCLUDED
#define THREADCOMM_PT2PT_H_INCLUDED

/* this is an internal header for threadcomm_pt2pt_impl.c */

struct send_hdr {
    int src_id;
    int tag;
    int attr;
    bool is_eager;
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

static void threadcomm_data_copy(struct send_hdr *hdr,
                                 void *buf, MPI_Aint count, MPI_Datatype datatype);

static MPIR_Request *threadcomm_enqueue_posted(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                               int src_id, int tag, int attr,
                                               int tid, MPIR_Request ** list);
static MPIR_Request *threadcomm_match_posted(int src_id, int tag, int attr, MPIR_Request ** list);
static void threadcomm_complete_posted(struct send_hdr *hdr, MPIR_Request * rreq);

static void threadcomm_enqueue_unexp(struct send_hdr *hdr, MPIR_threadcomm_unexp_t ** list);
static MPIR_threadcomm_unexp_t *threadcomm_match_unexp(int src_id, int tag, int attr,
                                                       MPIR_threadcomm_unexp_t ** list);

/* TODO: refactor transport, add queue implementation */
static int threadcomm_fbox_send(int src_id, int dst_id, struct send_hdr *hdr,
                                const void *data, MPI_Aint count, MPI_Datatype datatype,
                                MPIR_Threadcomm * threadcomm);

static int threadcomm_progress_send(void);
static int threadcomm_progress_recv(void);

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
        hdr.src_id = p->tid;
        hdr.tag = tag;
        hdr.attr = attr;
        hdr.is_eager = true;
        hdr.u.data_sz = data_sz;

        int ret = threadcomm_fbox_send(p->tid, dst_id, &hdr, buf, count, datatype, threadcomm);
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
    hdr.src_id = p->tid;
    hdr.tag = tag;
    hdr.attr = attr;
    hdr.is_eager = false;
    hdr.u.sreq = sreq;

    int ret = threadcomm_fbox_send(p->tid, dst_id, &hdr, NULL, 0, MPI_DATATYPE_NULL, threadcomm);
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
            (*req)->status.MPI_SOURCE = MPIR_THREADCOMM_TID_TO_RANK(threadcomm, hdr->src_id);
            (*req)->status.MPI_TAG = hdr->tag;
            (*req)->status.MPI_ERROR = MPIR_PT2PT_ATTR_GET_ERRFLAG(hdr->attr);
            MPIR_Request_complete(*req);
        } else {
            *req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        }
        threadcomm_data_copy(hdr, buf, count, datatype);
        MPL_free(unexp);
    } else {
        *req = threadcomm_enqueue_posted(buf, count, datatype, src_id, tag, attr,
                                         p->tid, &p->posted_list);
    }

    return mpi_errno;
}

/* ---- internal functions ---- */

static void threadcomm_data_copy(struct send_hdr *hdr,
                                 void *buf, MPI_Aint count, MPI_Datatype datatype)
{
    if (hdr->is_eager) {
        MPI_Aint actual_unpack_bytes;
        const void *data = hdr + 1;
        MPIR_Typerep_unpack(data, hdr->u.data_sz, buf, count, datatype,
                            0, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_unpack_bytes == hdr->u.data_sz);
    } else {
        MPIR_Request *sreq = hdr->u.sreq;
        struct send_req *sreq_u = (struct send_req *) &(sreq->u);
        MPIR_Localcopy(sreq_u->buf, sreq_u->count, sreq_u->datatype, buf, count, datatype);
        MPIR_Request_complete(sreq);
    }
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
        if (u->src_id == src_id && u->tag == tag &&
            MPIR_PT2PT_ATTR_CONTEXT_OFFSET(u->attr) == MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr)) {
            DL_DELETE(*list, curr);
            req = curr;
            break;
        }
    }
    return req;
}

static void threadcomm_complete_posted(struct send_hdr *hdr, MPIR_Request * rreq)
{
    struct posted_req *u = (struct posted_req *) &rreq->u;
    threadcomm_data_copy(hdr, u->buf, u->count, u->datatype);
    MPIR_Request_complete(rreq);
}

#define UNEXP MPIR_threadcomm_unexp_t
static void threadcomm_enqueue_unexp(struct send_hdr *hdr, UNEXP ** list)
{
    MPI_Aint cell_sz = sizeof(*hdr);
    if (hdr->is_eager) {
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
        if (hdr->src_id == src_id && hdr->tag == tag &&
            MPIR_PT2PT_ATTR_CONTEXT_OFFSET(hdr->attr) == MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr)) {
            DL_DELETE(*list, curr);
            req = curr;
            break;
        }
    }
    return req;
}

static int threadcomm_fbox_send(int src_id, int dst_id, struct send_hdr *hdr,
                                const void *data, MPI_Aint count, MPI_Datatype datatype,
                                MPIR_Threadcomm * threadcomm)
{
    MPIR_threadcomm_fbox_t *fbox = MPIR_THREADCOMM_MAILBOX(threadcomm, src_id, dst_id);

    if (MPL_atomic_load_int(&fbox->u.data_ready)) {
        return -1;
    }

    memcpy(fbox->cell, hdr, sizeof(struct send_hdr));
    if (count > 0) {
        MPIR_Assert(hdr->is_eager);
        MPI_Aint actual_pack_bytes;
        void *p = (char *) fbox->cell + sizeof(struct send_hdr);
        MPIR_Typerep_pack(data, count, datatype, 0, p, hdr->u.data_sz, &actual_pack_bytes,
                          MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_pack_bytes == hdr->u.data_sz);
    }
    MPL_atomic_store_int(&fbox->u.data_ready, 1);

    return 0;
}

static int threadcomm_progress_send(void)
{
    MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *);
    for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) {
        MPIR_Request *curr, *tmp;
        DL_FOREACH_SAFE(p[i].pending_list, curr, tmp) {
            struct send_req *u = (struct send_req *) &curr->u;
            struct send_hdr hdr;
            hdr.src_id = p[i].tid;
            hdr.tag = u->tag;
            hdr.attr = u->attr;
            hdr.is_eager = false;
            hdr.u.sreq = curr;

            int ret = threadcomm_fbox_send(p[i].tid, u->dst_id, &hdr,
                                           NULL, 0, MPI_DATATYPE_NULL, p[i].threadcomm);
            if (ret == 0) {
                DL_DELETE(p[i].pending_list, curr);
            }
        }
    }

    return MPI_SUCCESS;
}

static int threadcomm_progress_recv(void)
{
    MPIR_threadcomm_tls_t *p = ut_type_array(MPIR_threadcomm_array, MPIR_threadcomm_tls_t *);
    for (int i = 0; i < utarray_len(MPIR_threadcomm_array); i++) {
        int num_threads = p[i].threadcomm->num_threads;
        for (int j = 0; j < num_threads; j++) {
            MPIR_threadcomm_fbox_t *fbox = MPIR_THREADCOMM_MAILBOX(p[i].threadcomm, j, p[i].tid);
            if (MPL_atomic_load_int(&fbox->u.data_ready)) {
                struct send_hdr *hdr = (void *) fbox->cell;
                MPIR_Request *rreq =
                    threadcomm_match_posted(hdr->src_id, hdr->tag, hdr->attr, &p[i].posted_list);
                if (rreq) {
                    threadcomm_complete_posted(hdr, rreq);
                    rreq->status.MPI_TAG = hdr->tag;
                    rreq->status.MPI_SOURCE = MPIR_THREADCOMM_TID_TO_RANK(p[i].threadcomm, j);
                } else {
                    threadcomm_enqueue_unexp(hdr, &p[i].unexp_list);
                }
                MPL_atomic_store_int(&fbox->u.data_ready, 0);
            }
        }
    }

    return MPI_SUCCESS;
}

#endif /* THREADCOMM_PT2PT_H_INCLUDED */
