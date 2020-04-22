/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Sendrecv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Sendrecv = PMPI_Sendrecv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Sendrecv  MPI_Sendrecv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Sendrecv as PMPI_Sendrecv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                 int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_Sendrecv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Sendrecv
#define MPI_Sendrecv PMPI_Sendrecv

#endif

/*@
    MPI_Sendrecv - Sends and receives a message

Input Parameters:
+ sendbuf - initial address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - type of elements in send buffer (handle)
. dest - rank of destination (integer)
. sendtag - send tag (integer)
. recvcount - number of elements in receive buffer (integer)
. recvtype - type of elements in receive buffer (handle)
. source - rank of source (integer)
. recvtag - receive tag (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - initial address of receive buffer (choice)
- status - status object (Status).  This refers to the receive operation.

.N ThreadSafe

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 int dest, int sendtag,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *sreq = NULL;
    MPIR_Request *rreq = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_SENDRECV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER_BOTH(MPID_STATE_MPI_SENDRECV);

    /* Validate handle parameters needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert handles to MPI objects. */
    MPIR_Comm_get_ptr(comm, comm_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate communicator */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno)
                goto fn_fail;

            /* Validate count */
            MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
            MPIR_ERRTEST_COUNT(recvcount, mpi_errno);

            /* Validate status (status_ignore is not the same as null) */
            MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);

            /* Validate tags */
            MPIR_ERRTEST_SEND_TAG(sendtag, mpi_errno);
            MPIR_ERRTEST_RECV_TAG(recvtag, mpi_errno);

            /* Validate source and destination */
            if (comm_ptr) {
                MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
                MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
            }

            /* Validate datatype handles */
            MPIR_ERRTEST_DATATYPE(sendtype, "datatype", mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "datatype", mpi_errno);

            /* Validate datatype objects */
            if (!HANDLE_IS_BUILTIN(sendtype)) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(sendtype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }
            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(recvtype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }

            /* Validate buffers */
            MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* FIXME - Performance for small messages might be better if MPID_Send() were used here instead of MPID_Isend() */
    /* If source is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(source == MPI_PROC_NULL)) {
        rreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__RECV);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Status_set_procnull(&rreq->status);
    } else {
        mpi_errno =
            MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag, comm_ptr,
                       MPIR_CONTEXT_INTRA_PT2PT, &rreq);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
    }

    /* If dest is MPI_PROC_NULL, create a completed request and return. */
    if (unlikely(dest == MPI_PROC_NULL)) {
        sreq = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        mpi_errno =
            MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag, comm_ptr,
                       MPIR_CONTEXT_INTRA_PT2PT, &sreq);
        if (mpi_errno != MPI_SUCCESS) {
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPIX_ERR_NOREQ)
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
            /* FIXME: should we cancel the pending (possibly completed) receive request or wait for it to complete? */
            MPIR_Request_free(rreq);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
    }

    if (!MPIR_Request_is_complete(sreq) || !MPIR_Request_is_complete(rreq)) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (!MPIR_Request_is_complete(sreq) || !MPIR_Request_is_complete(rreq)) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno != MPI_SUCCESS) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }

            if (unlikely(MPIR_Request_is_anysrc_mismatched(rreq))) {
                /* --BEGIN ERROR HANDLING-- */
                mpi_errno = MPIR_Request_handle_proc_failed(rreq);
                if (!MPIR_Request_is_complete(sreq)) {
                    MPID_Cancel_send(sreq);
                    MPIR_STATUS_SET_CANCEL_BIT(sreq->status, FALSE);
                }
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }
        }
        MPID_Progress_end(&progress_state);
    }

    mpi_errno = rreq->status.MPI_ERROR;
    MPIR_Request_extract_status(rreq, status);
    MPIR_Request_free(rreq);

    if (mpi_errno == MPI_SUCCESS) {
        mpi_errno = sreq->status.MPI_ERROR;
    }
    MPIR_Request_free(sreq);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT_BOTH(MPID_STATE_MPI_SENDRECV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_sendrecv",
                                 "**mpi_sendrecv %p %d %D %i %t %p %d %D %i %t %C %p", sendbuf,
                                 sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype,
                                 source, recvtag, comm, status);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
