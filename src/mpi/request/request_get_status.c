/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


/* -- Begin Profiling Symbol Block for routine MPI_Request_get_status */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Request_get_status = PMPI_Request_get_status
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Request_get_status  MPI_Request_get_status
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Request_get_status as PMPI_Request_get_status
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_Request_get_status")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Request_get_status
#define MPI_Request_get_status PMPI_Request_get_status

#endif

/*@
   MPI_Request_get_status - Nondestructive test for the completion of a Request

Input Parameters:
.  request - request (handle).  May be 'MPI_REQUEST_NULL'.

Output Parameters:
+  flag - true if operation has completed (logical)
-  status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

   Notes:
   Unlike 'MPI_Test', 'MPI_Request_get_status' does not deallocate or
   deactivate the request.  A call to one of the test/wait routines or
   'MPI_Request_free' should be made to release the request object.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REQUEST_GET_STATUS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_REQUEST_GET_STATUS);

    /* Check the arguments */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_REQUEST_OR_NULL(request, mpi_errno);
            MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
            /* NOTE: MPI_STATUS_IGNORE != NULL */
            MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* If this is a null request handle, then return an empty status */
    if (request == MPI_REQUEST_NULL) {
        *flag = 1;
        MPIR_Status_set_empty(status);
        /* the above macro doesn't set MPI_ERROR */
        if (status != MPI_STATUS_IGNORE)
            status->MPI_ERROR = MPI_SUCCESS;
        goto fn_exit;
    }

    /* Convert MPI object handles to object pointers */
    MPIR_Request_get_ptr(request, request_ptr);

    /* Validate parameters if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate request_ptr */
            MPIR_Request_valid_ptr(request_ptr, mpi_errno);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Request_get_status_impl(request_ptr, flag, status);
    if (mpi_errno) {
        goto fn_fail;
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_REQUEST_GET_STATUS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OTHER,
                                         "**mpi_request_get_status",
                                         "**mpi_request_get_status %R %p %p", request, flag,
                                         status);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
