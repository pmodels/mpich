/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

#undef FUNCNAME
#define FUNCNAME MPIC_Probe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIR_Comm *comm_ptr;

    MPIR_Comm_get_ptr(comm, comm_ptr);

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Probe(source, tag, comm_ptr, context_id, status);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* FIXME: For the brief-global and finer-grain control, we must ensure that
   the global lock is *not* held when this routine is called. (unless we change
   progress_start/end to grab the lock, in which case we must *still* make
   sure that the lock is not held when this routine is called). */
#undef FUNCNAME
#define FUNCNAME MPIC_Wait
#undef FCNAME
#define FCNAME "MPIC_Wait"
int MPIC_Wait(MPIR_Request * request_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_WAIT);

    MPIR_FUNC_VERBOSE_PT2PT_ENTER(MPID_STATE_MPIC_WAIT);

    MPL_DBG_MSG_S(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %s", *errflag ? "TRUE" : "FALSE");

    if (request_ptr->kind == MPIR_REQUEST_KIND__SEND)
        request_ptr->status.MPI_TAG = 0;

    if (!MPIR_Request_is_complete(request_ptr)) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (!MPIR_Request_is_complete(request_ptr)) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
        MPID_Progress_end(&progress_state);
    }

    if (request_ptr->kind == MPIR_REQUEST_KIND__RECV)
        MPIR_Process_status(&request_ptr->status, errflag);

    MPIR_TAG_CLEAR_ERROR_BITS(request_ptr->status.MPI_TAG);

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_VERBOSE_PT2PT_EXIT(MPID_STATE_MPIC_WAIT);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
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

#undef FUNCNAME
#define FUNCNAME MPIC_Send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_SEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_SEND);

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

    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr, context_id, &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_Request_free(request_ptr);
    }

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_SEND);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
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

#undef FUNCNAME
#define FUNCNAME MPIC_Recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
              MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPI_Status mystatus;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_RECV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_RECV);

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;

    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr,
                          context_id, status, &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_RECV);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (request_ptr)
        MPIR_Request_free(request_ptr);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIC_Ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_SSEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_SSEND);

    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %d", (int) *errflag);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
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

    mpi_errno = MPID_Ssend(buf, count, datatype, dest, tag, comm_ptr, context_id, &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (request_ptr) {
        mpi_errno = MPIC_Wait(request_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_Request_free(request_ptr);
    }

  fn_exit:
    MPL_DBG_MSG_D(MPIR_DBG_PT2PT, TYPICAL, "OUT: errflag = %d", (int) *errflag);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_SSEND);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
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

#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPI_Status mystatus;
    MPIR_Request *recv_req_ptr = NULL, *send_req_ptr = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_SENDRECV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_SENDRECV);

    MPL_DBG_MSG_S(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %s", *errflag ? "TRUE" : "FALSE");

    MPIR_ERR_CHKANDJUMP1((sendcount < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", sendcount);
    MPIR_ERR_CHKANDJUMP1((recvcount < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", recvcount);

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    if (status == MPI_STATUS_IGNORE)
        status = &mystatus;
    switch (*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(sendtag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(sendtag);
    }

    mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag,
                           comm_ptr, context_id, &recv_req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag,
                           comm_ptr, context_id, &send_req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIC_Wait(send_req_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_SENDRECV);
    return mpi_errno;
  fn_fail:
    if (send_req_ptr)
        MPIR_Request_free(send_req_ptr);
    if (recv_req_ptr)
        MPIR_Request_free(recv_req_ptr);
    goto fn_exit;
}

/* NOTE: for regular collectives (as opposed to irregular collectives) calling
 * this function repeatedly will almost always be slower than performing the
 * equivalent inline because of the overhead of the repeated malloc/free */
#undef FUNCNAME
#define FUNCNAME MPIC_Sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Status mystatus;
    MPIR_Context_id_t context_id_offset;
    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;
    void *tmpbuf = NULL;
    MPI_Aint tmpbuf_size = 0;
    MPI_Aint tmpbuf_count = 0;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_SENDRECV_REPLACE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_SENDRECV_REPLACE);

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
            MPL_FALLTHROUGH;
        default:
            MPIR_TAG_SET_ERROR_BIT(sendtag);
    }

    context_id_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    if (count > 0 && dest != MPI_PROC_NULL) {
        MPIR_Pack_size_impl(count, datatype, &tmpbuf_size);
        MPIR_CHKLMEM_MALLOC(tmpbuf, void *, tmpbuf_size, mpi_errno, "temporary send buffer",
                            MPL_MEM_BUFFER);

        mpi_errno = MPIR_Pack_impl(buf, count, datatype, tmpbuf, tmpbuf_size, &tmpbuf_count);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag,
                           comm_ptr, context_id_offset, &rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_Isend(tmpbuf, tmpbuf_count, MPI_PACKED, dest,
                           sendtag, comm_ptr, context_id_offset, &sreq);
    if (mpi_errno != MPI_SUCCESS) {
        /* --BEGIN ERROR HANDLING-- */
        /* FIXME: should we cancel the pending (possibly completed) receive
         * request or wait for it to complete? */
        MPIR_Request_free(rreq);
        MPIR_ERR_POP(mpi_errno);
        /* --END ERROR HANDLING-- */
    }

    mpi_errno = MPIC_Wait(sreq, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIC_Wait(rreq, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_SENDRECV_REPLACE);
    return mpi_errno;
  fn_fail:
    if (sreq)
        MPIR_Request_free(sreq);
    if (rreq)
        MPIR_Request_free(rreq);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Request ** request_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_ISEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_ISEND);

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

    mpi_errno = MPID_Isend(buf, count, datatype, dest, tag, comm_ptr, context_id, request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Issend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                MPIR_Comm * comm_ptr, MPIR_Request ** request_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_ISSEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_ISSEND);

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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_ISSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_Irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
               int tag, MPIR_Comm * comm_ptr, MPIR_Request ** request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int context_id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_IRECV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_IRECV);

    MPIR_ERR_CHKANDJUMP1((count < 0), mpi_errno, MPI_ERR_COUNT,
                         "**countneg", "**countneg %d", count);

    context_id = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr, context_id, request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIC_Waitall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_Waitall(int numreq, MPIR_Request * requests[], MPI_Status statuses[],
                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPI_Status status_static_array[MPIC_REQUEST_PTR_ARRAY_SIZE];
    MPI_Status *status_array = statuses;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIC_WAITALL);
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIC_WAITALL);

    MPL_DBG_MSG_S(MPIR_DBG_PT2PT, TYPICAL, "IN: errflag = %s", *errflag ? "TRUE" : "FALSE");

    if (statuses == MPI_STATUSES_IGNORE) {
        status_array = status_static_array;
    }

    if (numreq > MPIC_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC(status_array, MPI_Status *, numreq * sizeof(MPI_Status), mpi_errno,
                            "status objects", MPL_MEM_BUFFER);
    }

    for (i = 0; i < numreq; ++i) {
        /* The MPI_TAG field is not set for send operations, so if we want
         * to check for the error bit in the tag below, we should initialize all
         * tag fields here. */
        status_array[i].MPI_TAG = 0;
        status_array[i].MPI_SOURCE = MPI_PROC_NULL;
    }

    mpi_errno = MPIR_Waitall_impl(numreq, requests, status_array);
    if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
        goto fn_fail;
    mpi_errno = MPIR_Waitall_post(numreq, NULL, requests, status_array);
    if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
        goto fn_fail;

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIC_WAITALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
