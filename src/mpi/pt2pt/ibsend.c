/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ibsend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibsend = PMPI_Ibsend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibsend  MPI_Ibsend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibsend as PMPI_Ibsend
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request * request) __attribute__ ((weak, alias("PMPI_Ibsend")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibsend
#define MPI_Ibsend PMPI_Ibsend


int MPIR_Ibsend_impl(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                     MPIR_Comm * comm_ptr, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bsend_isend(buf, count, datatype, dest, tag, comm_ptr, NULL);
    MPIR_ERR_CHECK(mpi_errno);

    /* Ibsend is local-complete */
    MPIR_Request *req = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
    *request = req->handle;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#endif

/*@
    MPI_Ibsend - Starts a nonblocking buffered send

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_BUFFER

@*/
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IBSEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER_FRONT(MPID_STATE_MPI_IBSEND);

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

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            if (comm_ptr) {
                MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);
                MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno)
            }

            /* Validate datatype handle */
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* Validate datatype object */
            if (!HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }

            /* Validate buffer */
            MPIR_ERRTEST_USERBUFFER(buf, count, datatype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Create a completed request and return immediately for dummy process */
    if (unlikely(dest == MPI_PROC_NULL)) {
        MPIR_Request *request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(request_ptr == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        *request = request_ptr->handle;
        goto fn_exit;
    }

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ibsend_impl(buf, count, datatype, dest, tag, comm_ptr, request);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPI_IBSEND);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    /* FIXME: should we be setting the request at all in the case of an error? */
    *request = MPI_REQUEST_NULL;
#ifdef HAVE_ERROR_REPORTING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ibsend", "**mpi_ibsend %p %d %D %i %t %C %p", buf, count,
                                 datatype, dest, tag, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
