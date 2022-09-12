/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "stream_workq.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ
      category    : CH4
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Enable stream enqueue operations via stream work queue. Requires progress
        thread on the corresponding MPIX stream. Reference: MPIX_Stream_progress
        and MPIX_Start_progress_thread.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define GET_STREAM_AND_WORKQ(comm, gpu_stream, workq) \
    do { \
        MPIR_Stream *stream_ptr = MPIR_stream_comm_get_local_stream(comm_ptr); \
        MPIR_ERR_CHKANDJUMP(!stream_ptr || stream_ptr->type != MPIR_STREAM_GPU, \
                            mpi_errno, MPI_ERR_OTHER, "**notgpustream"); \
        gpu_stream = stream_ptr->u.gpu_stream; \
        workq = stream_ptr->dev.workq; \
    } while (0)

#define REQUEST_GET_STREAM_AND_WORKQ(req, gpu_stream, workq) \
    do { \
        MPIR_Stream *stream_ptr = req->u.enqueue.stream_ptr; \
        gpu_stream = stream_ptr->u.gpu_stream; \
        workq = stream_ptr->dev.workq; \
    } while (0)

/* ---- send enqueue ---- */
struct send_data {
    const void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int dest;
    int tag;
    MPIR_Comm *comm_ptr;
    /* req is a send request for Send, but an enqueue request for Isend */
    MPIR_Request *req;
};

static void send_enqueue_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct send_data *p = data;
    mpi_errno = MPID_Send(p->buf, p->count, p->datatype, p->dest, p->tag, p->comm_ptr,
                          MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    MPIR_Assertp(request_ptr != NULL);

    p->req = request_ptr;
}

int MPID_Send_enqueue(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                      int dest, int tag, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (!MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ) {
        mpi_errno = MPIR_Send_enqueue_impl(buf, count, datatype, dest, tag, comm_ptr);
        goto fn_exit;
    }

    MPL_gpu_stream_t gpu_stream;
    MPIDU_stream_workq_t *workq;
    GET_STREAM_AND_WORKQ(comm_ptr, gpu_stream, workq);

    struct send_data *p;
    p = MPL_malloc(sizeof(struct send_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIDU_stream_workq_op_t *op;
    op = MPL_malloc(sizeof(MPIDU_stream_workq_op_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!op, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPL_gpu_event_t *trigger_event;
    MPL_gpu_event_t *done_event;
    MPIDU_stream_workq_alloc_event(&trigger_event);
    MPIDU_stream_workq_alloc_event(&done_event);

    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    p->dest = dest;
    p->tag = tag;
    p->comm_ptr = comm_ptr;

    op->cb = send_enqueue_cb;
    op->data = p;
    op->trigger_event = trigger_event;
    op->done_event = done_event;
    op->request = &(p->req);
    op->status = NULL;

    MPL_gpu_enqueue_trigger(trigger_event, gpu_stream);
    MPL_gpu_enqueue_wait(done_event, gpu_stream);
    MPIDU_stream_workq_enqueue(workq, op);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- recv enqueue ---- */
struct recv_data {
    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int source;
    int tag;
    MPIR_Comm *comm_ptr;
    /* req is a recv request for Recv, but an enqueue request for Irecv */
    MPIR_Request *req;
};

static void recv_enqueue_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct recv_data *p = data;
    mpi_errno = MPID_Irecv(p->buf, p->count, p->datatype, p->source, p->tag, p->comm_ptr,
                           MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    MPIR_Assertp(request_ptr != NULL);

    p->req = request_ptr;
}

int MPID_Recv_enqueue(void *buf, MPI_Aint count, MPI_Datatype datatype,
                      int source, int tag, MPIR_Comm * comm_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (!MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ) {
        mpi_errno = MPIR_Recv_enqueue_impl(buf, count, datatype, source, tag, comm_ptr, status);
        goto fn_exit;
    }

    MPL_gpu_stream_t gpu_stream;
    MPIDU_stream_workq_t *workq;
    GET_STREAM_AND_WORKQ(comm_ptr, gpu_stream, workq);

    struct recv_data *p;
    p = MPL_malloc(sizeof(struct recv_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIDU_stream_workq_op_t *op;
    op = MPL_malloc(sizeof(MPIDU_stream_workq_op_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!op, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPL_gpu_event_t *trigger_event;
    MPL_gpu_event_t *done_event;
    MPIDU_stream_workq_alloc_event(&trigger_event);
    MPIDU_stream_workq_alloc_event(&done_event);

    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    p->source = source;
    p->tag = tag;
    p->comm_ptr = comm_ptr;

    op->cb = recv_enqueue_cb;
    op->data = p;
    op->trigger_event = trigger_event;
    op->done_event = done_event;
    op->request = &(p->req);
    if (status != MPI_STATUSES_IGNORE) {
        op->status = status;
    } else {
        op->status = NULL;
    }

    MPL_gpu_enqueue_trigger(trigger_event, gpu_stream);
    MPL_gpu_enqueue_wait(done_event, gpu_stream);
    MPIDU_stream_workq_enqueue(workq, op);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- isend enqueue ---- */
static void isend_enqueue_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct send_data *p = data;
    mpi_errno = MPID_Send(p->buf, p->count, p->datatype, p->dest, p->tag, p->comm_ptr,
                          MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    MPIR_Assertp(request_ptr != NULL);

    p->req->u.enqueue.real_request = request_ptr;
}

int MPID_Isend_enqueue(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int dest, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (!MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ) {
        mpi_errno = MPIR_Isend_enqueue_impl(buf, count, datatype, dest, tag, comm_ptr, req);
        goto fn_exit;
    }

    MPL_gpu_stream_t gpu_stream;
    MPIDU_stream_workq_t *workq;
    GET_STREAM_AND_WORKQ(comm_ptr, gpu_stream, workq);

    struct send_data *p;
    p = MPL_malloc(sizeof(struct send_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIDU_stream_workq_op_t *op;
    op = MPL_malloc(sizeof(MPIDU_stream_workq_op_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!op, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPL_gpu_event_t *trigger_event;
    MPIDU_stream_workq_alloc_event(&trigger_event);

    mpi_errno = MPIR_allocate_enqueue_request(comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);
    (*req)->u.enqueue.is_send = true;
    (*req)->u.enqueue.data = p;

    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    p->dest = dest;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    p->req = *req;

    op->cb = isend_enqueue_cb;
    op->data = p;
    op->trigger_event = trigger_event;
    op->done_event = NULL;
    op->request = &(*req)->u.enqueue.real_request;
    op->status = NULL;

    MPL_gpu_enqueue_trigger(trigger_event, gpu_stream);
    MPIDU_stream_workq_enqueue(workq, op);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- irecv enqueue ---- */
static void irecv_enqueue_cb(void *data)
{
    int mpi_errno;
    MPIR_Request *request_ptr = NULL;

    struct recv_data *p = data;
    mpi_errno = MPID_Irecv(p->buf, p->count, p->datatype, p->source, p->tag, p->comm_ptr,
                           MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    MPIR_Assertp(mpi_errno == MPI_SUCCESS);
    MPIR_Assertp(request_ptr != NULL);

    p->req->u.enqueue.real_request = request_ptr;
}

int MPID_Irecv_enqueue(void *buf, MPI_Aint count, MPI_Datatype datatype,
                       int source, int tag, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (!MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ) {
        mpi_errno = MPIR_Irecv_enqueue_impl(buf, count, datatype, source, tag, comm_ptr, req);
        goto fn_exit;
    }

    MPL_gpu_stream_t gpu_stream;
    MPIDU_stream_workq_t *workq;
    GET_STREAM_AND_WORKQ(comm_ptr, gpu_stream, workq);

    struct recv_data *p;
    p = MPL_malloc(sizeof(struct recv_data), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIDU_stream_workq_op_t *op;
    op = MPL_malloc(sizeof(MPIDU_stream_workq_op_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!op, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPL_gpu_event_t *trigger_event;
    MPIDU_stream_workq_alloc_event(&trigger_event);

    mpi_errno = MPIR_allocate_enqueue_request(comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);
    (*req)->u.enqueue.is_send = false;
    (*req)->u.enqueue.data = p;

    p->buf = buf;
    p->count = count;
    p->datatype = datatype;
    p->source = source;
    p->tag = tag;
    p->comm_ptr = comm_ptr;
    p->req = *req;

    op->cb = irecv_enqueue_cb;
    op->data = p;
    op->trigger_event = trigger_event;
    op->done_event = NULL;
    op->request = &(*req)->u.enqueue.real_request;
    op->status = NULL;

    MPL_gpu_enqueue_trigger(trigger_event, gpu_stream);
    MPIDU_stream_workq_enqueue(workq, op);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- wait enqueue ---- */
int MPID_Wait_enqueue(MPIR_Request * req_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (!MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ) {
        mpi_errno = MPIR_Wait_enqueue_impl(req_ptr, status);
        goto fn_exit;
    }

    MPL_gpu_stream_t gpu_stream;
    MPIDU_stream_workq_t *workq;
    REQUEST_GET_STREAM_AND_WORKQ(req_ptr, gpu_stream, workq);

    MPIDU_stream_workq_op_t *op;
    op = MPL_malloc(sizeof(MPIDU_stream_workq_op_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!op, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPL_gpu_event_t *trigger_event;
    MPL_gpu_event_t *done_event;
    MPIDU_stream_workq_alloc_event(&trigger_event);
    MPIDU_stream_workq_alloc_event(&done_event);

    op->cb = NULL;
    op->data = NULL;
    op->trigger_event = trigger_event;
    op->done_event = done_event;
    op->request = &req_ptr->u.enqueue.real_request;
    if (status != MPI_STATUS_IGNORE) {
        op->status = status;
    } else {
        op->status = NULL;
    }

    MPL_gpu_enqueue_trigger(trigger_event, gpu_stream);
    MPL_gpu_enqueue_wait(done_event, gpu_stream);
    MPIDU_stream_workq_enqueue(workq, op);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- waitall enqueue ---- */
int MPID_Waitall_enqueue(int count, MPI_Request * array_of_requests, MPI_Status * array_of_statuses)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (!MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ) {
        mpi_errno = MPIR_Waitall_enqueue_impl(count, array_of_requests, array_of_statuses);
        goto fn_exit;
    }

    MPL_gpu_event_t *trigger_event;
    MPL_gpu_event_t *done_event;
    MPIDU_stream_workq_alloc_event(&trigger_event);
    MPIDU_stream_workq_alloc_event(&done_event);
    MPL_gpu_event_init_count(done_event, count);

    MPL_gpu_stream_t the_gpu_stream;
    for (int i = 0; i < count; i++) {
        MPIR_Request *req_ptr;
        MPIR_Request_get_ptr(array_of_requests[i], req_ptr);
        MPIR_Assert(req_ptr && req_ptr->kind == MPIR_REQUEST_KIND__ENQUEUE);

        MPL_gpu_stream_t gpu_stream;
        MPIDU_stream_workq_t *workq;
        REQUEST_GET_STREAM_AND_WORKQ(req_ptr, gpu_stream, workq);

        if (i == 0) {
            MPL_gpu_enqueue_trigger(trigger_event, gpu_stream);
            MPL_gpu_enqueue_wait(done_event, gpu_stream);
            the_gpu_stream = gpu_stream;
        } else {
            MPIR_Assertp(the_gpu_stream == gpu_stream);
        }

        MPIDU_stream_workq_op_t *op;
        op = MPL_malloc(sizeof(MPIDU_stream_workq_op_t), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!op, mpi_errno, MPI_ERR_OTHER, "**nomem");

        op->cb = NULL;
        op->data = NULL;
        op->trigger_event = trigger_event;
        op->done_event = done_event;
        op->request = &req_ptr->u.enqueue.real_request;
        if (array_of_statuses != MPI_STATUSES_IGNORE) {
            op->status = &array_of_statuses[i];
        } else {
            op->status = NULL;
        }

        MPIDU_stream_workq_enqueue(workq, op);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
