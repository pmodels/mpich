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
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) __attribute__((weak,alias("PMPI_Test")));
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
int MPIR_Test_impl(MPI_Request *request, int *flag, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    int active_flag;
    MPID_Request *request_ptr = NULL;

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL) {
	MPIR_Status_set_empty(status);
	*flag = TRUE;
	goto fn_exit;
    }
    
    *flag = FALSE;

    MPID_Request_get_ptr( *request, request_ptr );

    /* If the request is already completed AND we want to avoid calling
     the progress engine, we could make the call to MPID_Progress_test
     conditional on the request not being completed. */
    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (request_ptr->kind == MPID_UREQUEST &&
        request_ptr->greq_fns != NULL &&
        request_ptr->greq_fns->poll_fn != NULL)
    {
        mpi_errno = (request_ptr->greq_fns->poll_fn)(request_ptr->greq_fns->grequest_extra_state, status);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    if (MPID_Request_is_complete(request_ptr)) {
	mpi_errno = MPIR_Request_complete(request, request_ptr, status,
					  &active_flag);
	*flag = TRUE;
	if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* Fall through to the exit */
    } else if (unlikely(
                MPIR_CVAR_ENABLE_FT &&
                MPID_Request_is_anysource(request_ptr) &&
                !MPID_Comm_AS_enabled(request_ptr->comm))) {
        MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
        if (status != MPI_STATUS_IGNORE) status->MPI_ERROR = mpi_errno;
        goto fn_fail;
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
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr = NULL;
   MPID_MPI_STATE_DECL(MPID_STATE_MPI_TEST);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TEST);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
	    MPIR_ERRTEST_REQUEST_OR_NULL(*request, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    MPID_Request_get_ptr( *request, request_ptr );
    
   /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if (*request != MPI_REQUEST_NULL)
	    {
		/* Validate request_ptr */
		MPID_Request_valid_ptr( request_ptr, mpi_errno );
                if (mpi_errno) goto fn_fail;
	    }
	    
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Test_impl(request, flag, status);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */
    
  fn_exit:
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST);
	MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
	return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_test",
	    "**mpi_test %p %p %p", request, flag, status);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(request_ptr ? request_ptr->comm : NULL, 
				     FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
