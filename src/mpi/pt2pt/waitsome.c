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

/* -- Begin Profiling Symbol Block for routine MPI_Waitsome */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitsome = PMPI_Waitsome
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitsome  MPI_Waitsome
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitsome as PMPI_Waitsome
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Waitsome
#define MPI_Waitsome PMPI_Waitsome

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitsome

/*@
    MPI_Waitsome - Waits for some given MPI Requests to complete

Input Parameters:
+ incount - length of array_of_requests (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ outcount - number of completed requests (integer) 
. array_of_indices - array of indices of operations that 
completed (array of integers) 
- array_of_statuses - array of status objects for 
    operations that completed (array of Status).  May be 'MPI_STATUSES_IGNORE'.

Notes:
  The array of indicies are in the range '0' to 'incount - 1' for C and 
in the range '1' to 'incount' for Fortran.  

Null requests are ignored; if all requests are null, then the routine
returns with 'outcount' set to 'MPI_UNDEFINED'.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

'MPI_Waitsome' provides an interface much like the Unix 'select' or 'poll' 
calls and, in a high qualilty implementation, indicates all of the requests
that have completed when 'MPI_Waitsome' is called.  
However, 'MPI_Waitsome' only guarantees that at least one
request has completed; there is no guarantee that `all` completed requests 
will be returned, or that the entries in 'array_of_indices' will be in 
increasing order. Also, requests that are completed while 'MPI_Waitsome' is
executing may or may not be returned, depending on the timing of the 
completion of the message.  

.N waitstatus

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], 
		 int *outcount, int array_of_indices[],
		 MPI_Status array_of_statuses[])
{
    static const char FCNAME[] = "MPI_Waitsome";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = request_ptr_array;
    MPI_Status * status_ptr;
    MPID_Progress_state progress_state;
    int i;
    int n_active;
    int n_inactive;
    int active_flag;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITSOME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITSOME);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(incount, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (incount != 0) {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
		MPIR_ERRTEST_ARGNULL(array_of_indices, "array_of_indices", mpi_errno);
		/* NOTE: MPI_STATUSES_IGNORE != NULL */
		MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses", mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(outcount, "outcount", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    for (i = 0; i < incount; i++)
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
    
    *outcount = 0;
    
    /* Convert MPI request handles to a request object pointers */
    if (incount > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPID_Request **, incount * sizeof(MPID_Request *), mpi_errno, "request pointers");
    }
    
    n_inactive = 0;
    for (i = 0; i < incount; i++)
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
		    if (mpi_errno != MPI_SUCCESS)
		    {
			goto fn_fail;
		    }
		    
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif	    
	}
	else
	{
	    n_inactive += 1;
	    request_ptrs[i] = NULL;
	} 
    }

    if (n_inactive == incount)
    {
	*outcount = MPI_UNDEFINED;
	goto fn_exit;
    }
    
    /* Bill Gropp says MPI_Waitsome() is expected to try to make
       progress even if some requests have already completed;  
       therefore, we kick the pipes once and then fall into a loop
       checking for completion and waiting for progress. */ 
    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    n_active = 0;
    MPID_Progress_start(&progress_state);
    for(;;)
    {
	mpi_errno = MPIR_Grequest_progress_poke(incount, 
			request_ptrs, array_of_statuses);
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	for (i = 0; i < incount; i++)
	{
            if (request_ptrs[i] != NULL && MPID_Request_is_complete(request_ptrs[i]))
	    {
		status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[n_active] : MPI_STATUS_IGNORE;
		rc = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status_ptr, &active_flag);
		if (active_flag)
		{
		    array_of_indices[n_active] = i;
		    n_active += 1;
		    
		    if (rc == MPI_SUCCESS)
		    { 
			request_ptrs[i] = NULL;
		    }
		    else
		    {
			mpi_errno = MPI_ERR_IN_STATUS;
			if (status_ptr != MPI_STATUS_IGNORE)
			{
			    status_ptr->MPI_ERROR = rc;
			}
		    }
		}
		else
		{
		    request_ptrs[i] = NULL;
		    n_inactive += 1;
		}
	    }
	}

	if (mpi_errno == MPI_ERR_IN_STATUS)
	{
	    if (array_of_statuses != MPI_STATUSES_IGNORE)
	    { 
		for (i = 0; i < n_active; i++)
		{
		    if (request_ptrs[array_of_indices[i]] == NULL)
		    { 
			array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
		    }
		}
	    }
	    *outcount = n_active;
	    break;
	}
	else if (n_active > 0)
	{
	    *outcount = n_active;
	    break;
	}
	else if (n_inactive == incount)
	{
	    *outcount = MPI_UNDEFINED;
	    break;
	}

	mpi_errno = MPID_Progress_wait(&progress_state);
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPID_Progress_end(&progress_state);
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}
    }
    MPID_Progress_end(&progress_state);

    /* ... end of body of routine ... */
    
  fn_exit:
    if (incount > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_FREEALL();
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITSOME);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(
	mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	"**mpi_waitsome", "**mpi_waitsome %d %p %p %p %p",
	incount, array_of_requests, outcount, array_of_indices, 
	array_of_statuses);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
