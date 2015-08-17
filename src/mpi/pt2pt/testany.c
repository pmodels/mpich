/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if !defined(MPID_REQUEST_PTR_ARRAY_SIZE)
#define MPID_REQUEST_PTR_ARRAY_SIZE 16
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Testany */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Testany = PMPI_Testany
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Testany  MPI_Testany
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Testany as PMPI_Testany
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag,
                MPI_Status *status) __attribute__((weak,alias("PMPI_Testany")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Testany
#define MPI_Testany PMPI_Testany

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Testany

/*@
    MPI_Testany - Tests for completion of any previdously initiated 
                  requests

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ indx - index of operation that completed, or 'MPI_UNDEFINED'  if none
  completed (integer) 
. flag - true if one of the operations is complete (logical) 
- status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

Notes:

While it is possible to list a request handle more than once in the
'array_of_requests', such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N ThreadSafe

.N waitstatus

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Testany(int count, MPI_Request array_of_requests[], int *indx,
		int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Testany";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = request_ptr_array;
    int i;
    int n_inactive;
    int active_flag;
    int last_disabled_anysource = -1;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TESTANY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TESTANY);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);

	    if (count != 0) {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
		/* NOTE: MPI_STATUS_IGNORE != NULL */
		MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(indx, "indx", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	    
	    for (i = 0; i < count; i++) {
		MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], i, mpi_errno);
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Convert MPI request handles to a request object pointers */
    if (count > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPID_Request **, count * sizeof(MPID_Request *), mpi_errno, "request pointers");
    }

    n_inactive = 0;
    for (i = 0; i < count; i++)
    {
	if (array_of_requests[i] != MPI_REQUEST_NULL)
	{
	    MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
	    /* Validate object pointers if error checking is enabled */
#           ifdef HAVE_ERROR_CHECKING
	    {
		MPID_BEGIN_ERROR_CHECKS;
		{
		    MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
		    if (mpi_errno) goto fn_fail;
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif	    
	}
	else
	{
	    request_ptrs[i] = NULL;
	    n_inactive += 1;
	}
    }

    if (n_inactive == count)
    {
	*flag = TRUE;
	*indx = MPI_UNDEFINED;
	if (status != NULL)  /* could be null if count=0 */
	    MPIR_Status_set_empty(status);
	goto fn_exit;
    }
    
    *flag = FALSE;
    *indx = MPI_UNDEFINED;
    
    mpi_errno = MPID_Progress_test();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
	
    for (i = 0; i < count; i++)
    {
	if (request_ptrs[i] != NULL && 
            request_ptrs[i]->kind == MPID_UREQUEST &&
            request_ptrs[i]->greq_fns->poll_fn != NULL)
	{
            mpi_errno = (request_ptrs[i]->greq_fns->poll_fn)(request_ptrs[i]->greq_fns->grequest_extra_state,
                                                             status);
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
        if (request_ptrs[i] != NULL)
        {
            if (MPID_Request_is_complete(request_ptrs[i]))
            {
                mpi_errno = MPIR_Request_complete(&array_of_requests[i],
                        request_ptrs[i],
                        status, &active_flag);
                if (active_flag)
                {
                    *flag = TRUE;
                    *indx = i;
                    goto fn_exit;
                }
                else
                {
                    n_inactive += 1;
                }
            } else if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        MPID_Request_is_anysource(request_ptrs[i]) &&
                        !MPID_Comm_AS_enabled(request_ptrs[i]->comm)))
            {
                last_disabled_anysource = i;
            }
        }
    }

    /* If none of the requests completed, mark the last anysource request as
     * pending failure. */
    if (unlikely(last_disabled_anysource != -1))
    {
        MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
        if (status != MPI_STATUS_IGNORE) status->MPI_ERROR = mpi_errno;
        *flag = TRUE;
        goto fn_fail;
    }
    
    if (n_inactive == count)
    {
	*flag = TRUE;
	*indx = MPI_UNDEFINED;
	/* status set to empty by MPIR_Request_complete() */
    }
    
    /* ... end of body of routine ... */
    
  fn_exit:
    if (count > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_FREEALL();
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TESTANY);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_testany",
	    "**mpi_testany %d %p %p %p %p", count, array_of_requests, indx, flag, status);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
