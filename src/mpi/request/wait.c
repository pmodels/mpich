/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Wait */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wait = PMPI_Wait
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wait  MPI_Wait
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wait as PMPI_Wait
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Wait(MPI_Request * request, MPI_Status * status) __attribute__ ((weak, alias("PMPI_Wait")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Wait
#define MPI_Wait PMPI_Wait

/* MPID_Wait call MPIR_Wait_state with initialized progress state */
int MPIR_Wait_state(MPIR_Request * request_ptr, MPI_Status * status, MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    while (!MPIR_Request_is_complete(request_ptr)) {
        mpi_errno = MPID_Progress_wait(state);
        MPIR_ERR_CHECK(mpi_errno);

        if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptr))) {
            mpi_errno = MPIR_Request_handle_proc_failed(request_ptr);
            goto fn_fail;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* legacy interface (for ch3) */
int MPIR_Wait_impl(MPIR_Request * request_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;

    MPIR_Assert(request_ptr != NULL);
    MPID_Progress_start(&progress_state);
    mpi_errno = MPIR_Wait_state(request_ptr, status, &progress_state);
    MPID_Progress_end(&progress_state);

    return mpi_errno;
}

int MPIR_Wait(MPI_Request * request, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int active_flag;
    MPIR_Request *request_ptr = NULL;

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL) {
        MPIR_Status_set_empty(status);
        goto fn_exit;
    }

    MPIR_Request_get_ptr(*request, request_ptr);
    MPIR_Assert(request_ptr != NULL);

    if (!MPIR_Request_is_complete(request_ptr)) {
        /* If this is an anysource request including a communicator with
         * anysource disabled, convert the call to an MPI_Test instead so we
         * don't get stuck in the progress engine. */
        if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptr))) {
            mpi_errno = MPIR_Test(request, &active_flag, status);
            goto fn_exit;
        }

        if (MPIR_Request_has_poll_fn(request_ptr)) {
            while (!MPIR_Request_is_complete(request_ptr)) {
                mpi_errno = MPIR_Grequest_poll(request_ptr, status);
                MPIR_ERR_CHECK(mpi_errno);

                /* Avoid blocking other threads since I am inside an infinite loop */
                MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            }
        } else {
            mpi_errno = MPID_Wait(request_ptr, status);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    mpi_errno = MPIR_Request_completion_processing(request_ptr, status);
    if (!MPIR_Request_is_persistent(request_ptr)) {
        MPIR_Request_free(request_ptr);
        *request = MPI_REQUEST_NULL;
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

/*@
    MPI_Wait - Waits for an MPI request to complete

Input Parameters:
. request - request (handle)

Output Parameters:
. status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

.N waitstatus

.N ThreadSafe

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Wait(MPI_Request * request, MPI_Status * status)
{
    MPIR_Request *request_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_WAIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_REQUEST_ENTER(MPID_STATE_MPI_WAIT);

    /* Check the arguments */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* NOTE: MPI_STATUS_IGNORE != NULL */
            MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
            MPIR_ERRTEST_REQUEST_OR_NULL(*request, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL) {
        MPIR_Status_set_empty(status);
        goto fn_exit;
    }

    /* Convert MPI request handle to a request object pointer */
    MPIR_Request_get_ptr(*request, request_ptr);

    /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Request_valid_ptr(request_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    /* save copy of comm because request will be freed */
    if (request_ptr)
        comm_ptr = request_ptr->comm;
    mpi_errno = MPIR_Wait(request, status);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_REQUEST_EXIT(MPID_STATE_MPI_WAIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                     __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_wait", "**mpi_wait %p %p", request, status);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
