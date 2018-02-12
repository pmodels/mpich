/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Test */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Test = PMPI_Test
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Test  MPI_Test
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Test as PMPI_Test
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_Test")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Test
#define MPI_Test PMPI_Test

#undef FUNCNAME
#define FUNCNAME MPIR_Test_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Test_impl(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    *flag = FALSE;

    /* If the request is already completed AND we want to avoid calling
     * the progress engine, we could make the call to MPID_Progress_test
     * conditional on the request not being completed. */
    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    if (request_ptr->kind == MPIR_REQUEST_KIND__GREQUEST &&
        request_ptr->u.ureq.greq_fns != NULL && request_ptr->u.ureq.greq_fns->poll_fn != NULL) {
        mpi_errno =
            (request_ptr->u.ureq.greq_fns->poll_fn) (request_ptr->u.ureq.
                                                     greq_fns->grequest_extra_state, status);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (MPIR_Request_is_complete(request_ptr)) {
        *flag = TRUE;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Test  - Tests for the completion of a request

Input Parameters:
. request - MPI request (handle)

Output Parameters:
+ flag - true if operation completed (logical)
- status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

.N ThreadSafe

.N waitstatus

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int active_flag;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TEST);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER(MPID_STATE_MPI_TEST);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            MPIR_ERRTEST_REQUEST_OR_NULL(*request, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Request_get_ptr(*request, request_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (*request != MPI_REQUEST_NULL) {
                /* Validate request_ptr */
                MPIR_Request_valid_ptr(request_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
            /* NOTE: MPI_STATUS_IGNORE != NULL */
            MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL) {
        MPIR_Status_set_empty(status);
        *flag = TRUE;
        goto fn_exit;
    }

    /* ... body of routine ...  */

    mpi_errno = MPIR_Test_impl(request_ptr, flag, status);
    if (mpi_errno)
        goto fn_fail;

    if (*flag != FALSE) {
        mpi_errno = MPIR_Request_completion_processing(request_ptr, status,
                                                       &active_flag);
        if (!MPIR_Request_is_persistent(request_ptr)) {
            MPIR_Request_free(request_ptr);
            *request = MPI_REQUEST_NULL;
        }
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* Fall through to the exit */
    } else if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        MPID_Request_is_anysource(request_ptr) &&
                        !MPID_Comm_AS_enabled(request_ptr->comm))) {
        MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
        if (status != MPI_STATUS_IGNORE)
            status->MPI_ERROR = mpi_errno;
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPI_TEST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_test", "**mpi_test %p %p %p", request, flag, status);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(request_ptr ? request_ptr->comm : NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
