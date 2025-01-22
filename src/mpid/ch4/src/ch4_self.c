/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "utlist.h"

/* This utitility handles messages within "self"-comms, i.e. MPI_COMM_SELF and all
 * communicators with size of 1.
 *
 * Note: we don't really need the rank argument since self-comm rank has to be 0. Nevertheless
 * we would like to maintain a similar interface as other send/recv. It may come handy in case
 * we ever want to extend the functionality to all self messages.
 */

static MPID_Thread_mutex_t MPIDIU_THREAD_SELF_MUTEX;
static MPIR_Request *self_send_queue;
static MPIR_Request *self_recv_queue;

/*
 *  MPIDI_self_request_t can be accessed via
 *     req->dev.ch4.self
 */

#define ENQUEUE_SELF(req_, buf_, count_, datatype_, tag_, context_id_, queue) \
    do { \
        MPIR_Request_add_ref(req_); \
        req_->dev.ch4.self.buf = (char *) buf_; \
        req_->dev.ch4.self.count = count_; \
        req_->dev.ch4.self.datatype = datatype_; \
        req_->dev.ch4.self.tag = tag_; \
        req_->dev.ch4.self.context_id = context_id_; \
        DL_APPEND(queue, req_); \
    } while (0)

#define ENQUEUE_SELF_SEND(req, buf, count, datatype, tag, context_id) \
    ENQUEUE_SELF(req, buf, count, datatype, tag, context_id, self_send_queue)

#define ENQUEUE_SELF_RECV(req, buf, count, datatype, tag, context_id) \
    ENQUEUE_SELF(req, buf, count, datatype, tag, context_id, self_recv_queue)

#define MATCH_TAG(tag1, tag2) (tag1 == tag2 || tag1 == MPI_ANY_TAG || tag2 == MPI_ANY_TAG)
#define DEQUEUE_SELF(req_, tag_, context_id_, queue) \
    do { \
        MPIR_Request *curr, *tmp; \
        DL_FOREACH_SAFE(queue, curr, tmp) { \
            if (curr->dev.ch4.self.context_id == context_id_ && MATCH_TAG(curr->dev.ch4.self.tag, tag_)) { \
                req_ = curr; \
                DL_DELETE(queue, curr); \
                break; \
            } \
        } \
    } while (0)

#define DEQUEUE_SELF_SEND(req, tag, context_id) DEQUEUE_SELF(req, tag, context_id, self_send_queue)
#define DEQUEUE_SELF_RECV(req, tag, context_id) DEQUEUE_SELF(req, tag, context_id, self_recv_queue)

#define FIND_SELF_SEND(req_, tag_, context_id_) \
    do { \
        MPIR_Request *curr, *tmp; \
        DL_FOREACH_SAFE(self_send_queue, curr, tmp) { \
            if (curr->dev.ch4.self.context_id == context_id_ && MATCH_TAG(curr->dev.ch4.self.tag, tag_)) { \
                req_ = curr; \
                break; \
            } \
        } \
    } while (0)

int MPIDI_Self_init(void)
{
    MPIR_FUNC_ENTER;

    int err;
    MPID_Thread_mutex_create(&MPIDIU_THREAD_SELF_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDI_Self_finalize(void)
{
    MPIR_FUNC_ENTER;

    int err;
    MPID_Thread_mutex_destroy(&MPIDIU_THREAD_SELF_MUTEX, &err);
    MPIR_Assert(err == 0);

    MPIR_Assert(self_send_queue == NULL);
    MPIR_Assert(self_recv_queue == NULL);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

#define SELF_COPY(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, status, tag) \
    do { \
        MPI_Aint sdata_sz, rdata_sz; \
        MPIR_Datatype_get_size_macro(sendtype, sdata_sz); \
        MPIR_Datatype_get_size_macro(recvtype, rdata_sz); \
        sdata_sz *= sendcnt; \
        rdata_sz *= recvcnt; \
        MPIR_Localcopy(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype); \
        status.MPI_SOURCE = 0; \
        status.MPI_TAG = tag; \
        if (sdata_sz > rdata_sz) { \
            MPIR_STATUS_SET_COUNT(status, rdata_sz); \
            status.MPI_ERROR =  MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, \
                                                     __func__, __LINE__, MPI_ERR_TRUNCATE, \
                                                     "**truncate", "**truncate %d %d %d %d", \
                                                     status.MPI_SOURCE, status.MPI_TAG, \
                                                     (int) rdata_sz, (int) sdata_sz); \
        } else { \
            MPIR_STATUS_SET_COUNT(status, sdata_sz); \
            status.MPI_ERROR = MPI_SUCCESS; \
        } \
    } while (0)

int MPIDI_Self_isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag,
                     MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;
    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SELF_MUTEX);

    DEQUEUE_SELF_RECV(rreq, tag, comm->context_id);
    if (rreq) {
        SELF_COPY(buf, count, datatype, rreq->dev.ch4.self.buf,
                  rreq->dev.ch4.self.count, rreq->dev.ch4.self.datatype, rreq->status, tag);
        /* comm will be released by MPIR_Request_free(rreq) */
        MPIR_Datatype_release_if_not_builtin(rreq->dev.ch4.self.datatype);
        MPIR_Request_complete(rreq);
        sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
    } else {
        sreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        ENQUEUE_SELF_SEND(sreq, buf, count, datatype, tag, comm->context_id);
        MPII_UNEXPQ_REMEMBER(sreq, rank, tag, comm->context_id);
        sreq->comm = comm;
        MPIR_Comm_add_ref(comm);
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
    }

    *request = sreq;
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SELF_MUTEX);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDI_Self_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int rank, int tag,
                     MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;
    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SELF_MUTEX);

    rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);

    DEQUEUE_SELF_SEND(sreq, tag, comm->context_id);
    if (sreq) {
        SELF_COPY(sreq->dev.ch4.self.buf,
                  sreq->dev.ch4.self.count,
                  sreq->dev.ch4.self.datatype,
                  buf, count, datatype, rreq->status, sreq->dev.ch4.self.tag);
        /* comm will be released by MPIR_Request_free(sreq) */
        MPIR_Datatype_release_if_not_builtin(sreq->dev.ch4.self.datatype);
        MPIR_Request_complete(sreq);
        MPIR_Request_complete_nofree(rreq);
        MPII_UNEXPQ_FORGET(sreq);
    } else {
        ENQUEUE_SELF_RECV(rreq, buf, count, datatype, tag, comm->context_id);
        rreq->comm = comm;
        MPIR_Comm_add_ref(comm);
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
    }

    *request = rreq;
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SELF_MUTEX);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDI_Self_iprobe(int rank, int tag, MPIR_Comm * comm, int attr, int *flag, MPI_Status * status)
{
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SELF_MUTEX);

    FIND_SELF_SEND(sreq, tag, comm->context_id);
    if (sreq) {
        if (status != MPI_STATUS_IGNORE) {
            MPI_Aint sdata_sz;
            MPIR_Datatype_get_size_macro(sreq->dev.ch4.self.datatype, sdata_sz);
            sdata_sz *= sreq->dev.ch4.self.count;
            MPIR_STATUS_SET_COUNT(*status, sdata_sz);
            status->MPI_SOURCE = 0;
            status->MPI_TAG = sreq->dev.ch4.self.tag;
            status->MPI_ERROR = MPI_SUCCESS;
        }
        *flag = TRUE;
    } else {
        *flag = FALSE;
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SELF_MUTEX);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDI_Self_improbe(int rank, int tag, MPIR_Comm * comm, int attr,
                       int *flag, MPIR_Request ** message, MPI_Status * status)
{
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SELF_MUTEX);

    DEQUEUE_SELF_SEND(sreq, tag, comm->context_id);
    if (sreq) {
        if (status != MPI_STATUS_IGNORE) {
            MPI_Aint sdata_sz;
            MPIR_Datatype_get_size_macro(sreq->dev.ch4.self.datatype, sdata_sz);
            sdata_sz *= sreq->dev.ch4.self.count;
            MPIR_STATUS_SET_COUNT(*status, sdata_sz);
            status->MPI_SOURCE = 0;
            status->MPI_TAG = sreq->dev.ch4.self.tag;
            status->MPI_ERROR = MPI_SUCCESS;
        }
        *flag = TRUE;

        *message = MPIR_Request_create(MPIR_REQUEST_KIND__MPROBE);
        (*message)->dev.ch4.self.match_req = sreq;
        (*message)->comm = sreq->comm;  /* set so we can check it in MPI_{M,Im}recv */
        MPIR_Comm_add_ref((*message)->comm);
        MPII_UNEXPQ_FORGET(sreq);
    } else {
        *flag = FALSE;
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SELF_MUTEX);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDI_Self_imrecv(char *buf, MPI_Aint count, MPI_Datatype datatype,
                      MPIR_Request * message, MPIR_Request ** request)
{
    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SELF_MUTEX);

    MPIR_Request *sreq = message->dev.ch4.self.match_req;
    MPIR_Request *rreq = message;
    rreq->kind = MPIR_REQUEST_KIND__RECV;
    SELF_COPY(sreq->dev.ch4.self.buf,
              sreq->dev.ch4.self.count,
              sreq->dev.ch4.self.datatype,
              buf, count, datatype, rreq->status, sreq->dev.ch4.self.tag);
    /* comm will be released by MPIR_Request_free(sreq) */
    MPIR_Datatype_release_if_not_builtin(sreq->dev.ch4.self.datatype);
    MPIR_Request_complete(sreq);
    MPIR_Request_complete_nofree(rreq);

    *request = rreq;
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SELF_MUTEX);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

#define DELETE_SELF(req_, found, queue) \
    do { \
        MPIR_Request *curr, *tmp; \
        DL_FOREACH_SAFE(queue, curr, tmp) { \
            if (curr == req_) { \
                found = 1; \
                DL_DELETE(queue, curr); \
                break; \
            } \
        } \
    } while (0)

int MPIDI_Self_cancel(MPIR_Request * request)
{
    MPIR_FUNC_ENTER;
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_SELF_MUTEX);

    if (!MPIR_Request_is_complete(request) && !MPIR_STATUS_GET_CANCEL_BIT(request->status)) {
        int found = 0;
        if (request->kind == MPIR_REQUEST_KIND__SEND) {
            DELETE_SELF(request, found, self_send_queue);
        } else {
            DELETE_SELF(request, found, self_recv_queue);
        }
        if (found) {
            /* comm will be released at MPIR_Request_free */
            MPIR_Datatype_release_if_not_builtin(request->dev.ch4.self.datatype);
        } else {
            goto fn_exit;
        }
        MPIR_STATUS_SET_CANCEL_BIT(request->status, TRUE);
        MPIR_STATUS_SET_COUNT(request->status, 0);
        MPIR_Request_complete(request);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_SELF_MUTEX);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}
