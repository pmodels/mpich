/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if !defined(MPID_REQUEST_PTR_ARRAY_SIZE)
#define MPID_REQUEST_PTR_ARRAY_SIZE 16
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Testall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Testall = PMPI_Testall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Testall  MPI_Testall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Testall as PMPI_Testall
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Testall
#define MPI_Testall PMPI_Testall

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Testall

/*@
    MPI_Testall - Tests for the completion of all previously initiated
    requests

Input Parameters:
+ count - lists length (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ flag - True if all requests have completed; false otherwise (logical) 
- array_of_statuses - array of status objects (array of Status).  May be
 'MPI_STATUSES_IGNORE'.

Notes:
  'flag' is true only if all requests have completed.  Otherwise, flag is
  false and neither the 'array_of_requests' nor the 'array_of_statuses' is
  modified.

If one or more of the requests completes with an error, 'MPI_ERR_IN_STATUS' is
returned.  An error value will be present is elements of 'array_of_status'
associated with the requests.  Likewise, the 'MPI_ERROR' field in the status
elements associated with requests that have successfully completed will be
'MPI_SUCCESS'.  Finally, those requests that have not completed will have a 
value of 'MPI_ERR_PENDING'.

While it is possible to list a request handle more than once in the
'array_of_requests', such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N ThreadSafe

.N waitstatus

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_IN_STATUS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, 
		MPI_Status array_of_statuses[])
{
    static const char FCNAME[] = "MPI_Testall";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = request_ptr_array;
    MPI_Status * status_ptr;
    int i;
    int n_completed;
    int active_flag;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TESTALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TESTALL);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (count != 0) {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", 
				     mpi_errno);
		/* NOTE: MPI_STATUSES_IGNORE != NULL */
		MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses", 
				     mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    for (i = 0; i < count; i++)
	    {
		MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], 
						  i, mpi_errno);
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Convert MPI request handles to a request object pointers */
    if (count > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPID_Request **, 
		 count * sizeof(MPID_Request *), mpi_errno, "request pointers");
    }

    n_completed = 0;
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
	    n_completed += 1;
	}
    }

    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    
    for (i = 0; i < count; i++)
    {
	if (request_ptrs[i] != NULL && 
			request_ptrs[i]->kind == MPID_UREQUEST &&
			request_ptrs[i]->poll_fn != NULL)
	{
	    mpi_errno = (request_ptrs[i]->poll_fn)(request_ptrs[i]->grequest_extra_state, 
			    &(array_of_statuses[i]));
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
	if (request_ptrs[i] != NULL && MPID_Request_is_complete(request_ptrs[i]))
	{
	    n_completed++;
	    if (MPIR_Request_get_error(request_ptrs[i]) != MPI_SUCCESS)
	    {
		mpi_errno = MPI_ERR_IN_STATUS;
	    }
	}
    }
    
    if (n_completed == count || mpi_errno == MPI_ERR_IN_STATUS)
    {
	n_completed = 0;
	for (i = 0; i < count; i++)
	{
	    if (request_ptrs[i] != NULL)
	    {
                if (MPID_Request_is_complete(request_ptrs[i]))
		{
		    n_completed ++;
		    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
		    rc = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status_ptr, &active_flag);
		    if (mpi_errno == MPI_ERR_IN_STATUS && status_ptr != MPI_STATUS_IGNORE)
		    {
			if (active_flag)
			{
			    status_ptr->MPI_ERROR = rc;
			}
			else
			{
			    status_ptr->MPI_ERROR = MPI_SUCCESS;
			}
		    }
		}
		else
		{
		    if (mpi_errno == MPI_ERR_IN_STATUS && array_of_statuses != MPI_STATUSES_IGNORE)
		    { 
			array_of_statuses[i].MPI_ERROR = MPI_ERR_PENDING;
		    }
		}
	    }
	    else
	    {
		n_completed ++;
		if (array_of_statuses != MPI_STATUSES_IGNORE)
		{ 
		    MPIR_Status_set_empty(&array_of_statuses[i]);
		    if (mpi_errno == MPI_ERR_IN_STATUS)
		    {
			array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
		    }
		}
	    }
	}
    }

    *flag = (n_completed == count) ? TRUE : FALSE;

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    if (count > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_FREEALL();
    }
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TESTALL);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_testall",
	    "**mpi_testall %d %p %p %p", count, array_of_requests, flag, array_of_statuses);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
