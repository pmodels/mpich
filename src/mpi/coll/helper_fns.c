/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

#if !defined(MPIC_REQUEST_PTR_ARRAY_SIZE)
#define MPIC_REQUEST_PTR_ARRAY_SIZE 64
#endif

/* These functions are used in the implementation of collective
   operations. They are wrappers around MPID send/recv functions. They do
   sends/receives by setting the context offset to
   MPIR_CONTEXT_INTRA_COLL or MPIR_CONTEXT_INTER_COLL. */

int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int attr = 0;
    MPIR_Comm *comm_ptr;

    /* Return immediately for dummy process */
    if (unlikely(source == MPI_PROC_NULL)) {
        MPIR_Status_set_procnull(status);
        goto fn_exit;
    }

    MPIR_Comm_get_ptr(comm, comm_ptr);

    attr |= (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Probe(source, tag, comm_ptr, attr, status);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    goto fn_exit;
}


/* FIXME: For the brief-global and finer-grain control, we must ensure that
   the global lock is *not* held when this routine is called. (unless we change
   progress_start/end to grab the lock, in which case we must *still* make
   sure that the lock is not held when this routine is called). */
int MPIC_Wait(MPIR_Request * request_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_S(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %s", *errflag ? "TRUE" : "FALSE");

    if (request_ptr->kind == MPIR_REQUEST_KIND__SEND)
        request_ptr->status.MPI_TAG = 0;

    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    MPIR_ERR_CHECK(mpi_errno);

    if (request_ptr->kind == MPIR_REQUEST_KIND__RECV)
        MPIR_Process_status(&request_ptr->status, errflag);

    MPIR_TAG_CLEAR_ERROR_BITS(request_ptr->status.MPI_TAG);

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* Fault-tolerance versions.  When a process fails, collectives will
   still complete, however the result may be invalid.  Processes
   directly communicating with the failed process can detect the
   failure, however another mechanism is needed to commuinicate the
   failure to other processes receiving the invalid data.  To do this
   we introduce the _ft versions of the MPIC_ helper functions.  These
   functions take a pointer to an error flag.  When this is set to
   TRUE, the send functions will communicate the failure to the
   receiver.  If a function detects a failure, either by getting a
   failure in the communication operation, or by receiving an error
   indicator from a remote process, it sets the error flag to TRUE.

   In this implementation, we indicate an error to a remote process by
   sending an empty message instead of the requested buffer.  When a
   process receives an empty message, it knows to set the error flag.
   We count on the fact that collectives that exchange data (as
   opposed to barrier) will never send an empty message.  The barrier
   collective will not communicate failure information this way, but
   this is OK since there is no data that can be received corrupted. */

int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int attr = 0;
    MPIR_Request *request_ptr = NULL;

    MPIR_FUNC_ENTER;

    /* Return immediately for dummy process */
    if (unlikely(dest == MPI_PROC_NULL)) {
        goto fn_exit;
    }

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    attr |= (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Send_coll(buf, count, datatype, dest, tag, comm_ptr, attr,
                               &request_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Request_free(request_ptr);
    }

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    if (request_ptr)
        MPIR_Request_free(request_ptr);
    if (mpi_errno && !*errflag) {
        if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(mpi_errno)) {
            *errflag = MPIR_ERR_PROC_FAILED;
        } else {
            *errflag = MPIR_ERR_OTHER;
        }
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
              MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int attr = 0;
    MPI_Status mystatus;
    MPIR_Request *request_ptr = NULL;

    MPIR_FUNC_ENTER;

    /* Return immediately for dummy process */
    if (unlikely(source == MPI_PROC_NULL)) {
        MPIR_Status_set_procnull(status);
        goto fn_exit;
    }

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    attr |= (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;

    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr, attr, status, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);

        *status = request_ptr->status;
        mpi_errno = status->MPI_ERROR;
        MPIR_Request_free(request_ptr);
    } else {
        MPIR_Process_status(status, errflag);

        MPIR_TAG_CLEAR_ERROR_BITS(status->MPI_TAG);
    }

    if (MPI_SUCCESS == MPIR_ERR_GET_CLASS(status->MPI_ERROR)) {
        MPIR_Assert(status->MPI_TAG == tag);
    }

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    if (request_ptr)
        MPIR_Request_free(request_ptr);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIC_Ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int attr = 0;
    MPIR_Request *request_ptr = NULL;

    MPIR_FUNC_ENTER;

    /* Return immediately for dummy process */
    if (unlikely(dest == MPI_PROC_NULL)) {
        goto fn_exit;
    }

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    attr |= (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    switch (*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    mpi_errno = MPID_Ssend(buf, count, datatype, dest, tag, comm_ptr, attr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Request_free(request_ptr);
    }

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    if (request_ptr)
        MPIR_Request_free(request_ptr);
    if (mpi_errno && !*errflag) {
        if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(mpi_errno)) {
            *errflag = MPIR_ERR_PROC_FAILED;
        } else {
            *errflag = MPIR_ERR_OTHER;
        }
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int attr = 0;
    MPI_Status mystatus;
    MPIR_Request *recv_req_ptr = NULL, *send_req_ptr = NULL;

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_S(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %s", *errflag ? "TRUE" : "FALSE");

    MPIR_ERR_CHKANDJUMP1((sendcount < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", sendcount);
    MPIR_ERR_CHKANDJUMP1((recvcount < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", recvcount);

    attr |= (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;

    /* If source is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(source == MPI_PROC_NULL)) {
        recv_req_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        MPIR_ERR_CHKANDSTMT(recv_req_ptr == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        MPIR_Status_set_procnull(&recv_req_ptr->status);
    } else {
        mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag,
                               comm_ptr, attr, &recv_req_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* If dest is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(dest == MPI_PROC_NULL)) {
        send_req_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(send_req_ptr == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
    } else {
        mpi_errno = MPID_Isend_coll(sendbuf, sendcount, sendtype, dest, sendtag,
                                    comm_ptr, attr, &send_req_ptr, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIC_Wait(send_req_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIC_Wait(recv_req_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POPFATAL(mpi_errno);

    *status = recv_req_ptr->status;

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = recv_req_ptr->status.MPI_ERROR;

        if (mpi_errno == MPI_SUCCESS) {
            mpi_errno = send_req_ptr->status.MPI_ERROR;
        }
    }

    MPIR_Request_free(send_req_ptr);
    MPIR_Request_free(recv_req_ptr);

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);

    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    if (send_req_ptr)
        MPIR_Request_free(send_req_ptr);
    if (recv_req_ptr)
        MPIR_Request_free(recv_req_ptr);
    goto fn_exit;
}

/* NOTE: for regular collectives (as opposed to irregular collectives) calling
 * this function repeatedly will almost always be slower than performing the
 * equivalent inline because of the overhead of the repeated malloc/free */
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status mystatus;
    int attr = 0;
    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;
    void *tmpbuf = NULL;
    MPI_Aint tmpbuf_size = 0;
    MPI_Aint actual_pack_bytes = 0;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;
    switch (*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(sendtag);
            /* fall through */
        default:
            MPIR_TAG_SET_ERROR_BIT(sendtag);
    }

    attr |= (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    if (count > 0 && dest != MPI_PROC_NULL) {
        MPIR_Pack_size(count, datatype, &tmpbuf_size);
        MPIR_CHKLMEM_MALLOC(tmpbuf, void *, tmpbuf_size, mpi_errno, "temporary send buffer",
                            MPL_MEM_BUFFER);

        mpi_errno =
            MPIR_Typerep_pack(buf, count, datatype, 0, tmpbuf, tmpbuf_size, &actual_pack_bytes,
                              MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* If source is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(source == MPI_PROC_NULL)) {
        rreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Status_set_procnull(&rreq->status);
    } else {
        mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag, comm_ptr, attr, &rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* If dest is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(dest == MPI_PROC_NULL)) {
        sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        mpi_errno = MPID_Isend_coll(tmpbuf, actual_pack_bytes, MPI_PACKED, dest,
                                    sendtag, comm_ptr, attr, &sreq, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        if (mpi_errno != MPI_SUCCESS) {
            /* --BEGIN ERROR HANDLING-- */
            /* FIXME: should we cancel the pending (possibly completed) receive
             * request or wait for it to complete? */
            MPIR_Request_free(rreq);
            MPIR_ERR_POP(mpi_errno);
            /* --END ERROR HANDLING-- */
        }
    }

    mpi_errno = MPIC_Wait(sreq, errflag);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIC_Wait(rreq, errflag);
    MPIR_ERR_CHECK(mpi_errno);

    *status = rreq->status;

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = rreq->status.MPI_ERROR;

        if (mpi_errno == MPI_SUCCESS) {
            mpi_errno = sreq->status.MPI_ERROR;
        }
    }

    MPIR_Request_free(sreq);
    MPIR_Request_free(rreq);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    if (sreq)
        MPIR_Request_free(sreq);
    if (rreq)
        MPIR_Request_free(rreq);
    goto fn_exit;
}

int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Request ** request_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;

    MPIR_FUNC_ENTER;

    /* Create a completed request and return immediately for dummy process */
    if (unlikely(dest == MPI_PROC_NULL)) {
        *request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT((*request_ptr) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Isend_coll(buf, count, datatype, dest, tag, comm_ptr, context_id,
                                request_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    goto fn_exit;
}

int MPIC_Issend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                MPIR_Comm * comm_ptr, MPIR_Request ** request_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;

    MPIR_FUNC_ENTER;

    /* Create a completed request and return immediately for dummy process */
    if (unlikely(dest == MPI_PROC_NULL)) {
        *request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT((*request_ptr) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    switch (*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Issend(buf, count, datatype, dest, tag, comm_ptr, context_id, request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    goto fn_exit;
}

int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
               int tag, MPIR_Comm * comm_ptr, MPIR_Request ** request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;

    MPIR_FUNC_ENTER;

    /* Create a completed request and return immediately for dummy process */
    if (unlikely(source == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        *request_ptr = rreq;
        MPIR_Status_set_procnull(&rreq->status);
        goto fn_exit;
    }

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr, context_id, request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (mpi_errno == MPIX_ERR_NOREQ)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
    goto fn_exit;
}


int MPIC_Waitall(int numreq, MPIR_Request * requests[], MPI_Status * statuses,
                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPI_Request request_ptr_array[MPIC_REQUEST_PTR_ARRAY_SIZE];
    MPI_Request *request_ptrs = request_ptr_array;
    MPI_Status status_static_array[MPIC_REQUEST_PTR_ARRAY_SIZE];
    MPI_Status *status_array = statuses;
    MPIR_CHKLMEM_DECL(2);

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_S(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %s", *errflag ? "TRUE" : "FALSE");

    if (statuses == MPI_STATUSES_IGNORE) {
        status_array = status_static_array;
    }

    if (numreq > MPIC_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC(request_ptrs, MPI_Request *, numreq * sizeof(MPI_Request), mpi_errno,
                            "request pointers", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(status_array, MPI_Status *, numreq * sizeof(MPI_Status), mpi_errno,
                            "status objects", MPL_MEM_BUFFER);
    }

    for (i = 0; i < numreq; ++i) {
        /* The MPI_TAG field is not set for send operations, so if we want
         * to check for the error bit in the tag below, we should initialize all
         * tag fields here. */
        status_array[i].MPI_TAG = 0;
        status_array[i].MPI_SOURCE = MPI_PROC_NULL;

        /* Convert the MPIR_Request objects to MPI_Request objects */
        request_ptrs[i] = requests[i]->handle;
    }

    mpi_errno = MPIR_Waitall(numreq, request_ptrs, status_array);

    /* The errflag value here is for all requests, not just a single one.  If
     * in the future, this function is used for multiple collectives at a
     * single time, we may have to change that. */
    for (i = 0; i < numreq; ++i) {
        MPIR_Process_status(&status_array[i], errflag);

        MPIR_TAG_CLEAR_ERROR_BITS(status_array[i].MPI_TAG);
    }

  fn_exit:
    if (numreq > MPIC_REQUEST_PTR_ARRAY_SIZE)
        MPIR_CHKLMEM_FREEALL();

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
