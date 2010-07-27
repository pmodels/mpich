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

/* -- Begin Profiling Symbol Block for routine MPI_Waitany */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitany = PMPI_Waitany
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitany  MPI_Waitany
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitany as PMPI_Waitany
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Waitany
#define MPI_Waitany PMPI_Waitany

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitany

/*@
    MPI_Waitany - Waits for any specified MPI Request to complete

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ index - index of handle for operation that completed (integer).  In the
range '0' to 'count-1'.  In Fortran, the range is '1' to 'count'.
- status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

Notes:
If all of the requests are 'MPI_REQUEST_NULL', then 'index' is returned as 
'MPI_UNDEFINED', and 'status' is returned as an empty status.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N waitstatus

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Waitany(int count, MPI_Request array_of_requests[], int *index, 
		MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Waitany";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = request_ptr_array;
    MPID_Progress_state progress_state;
    int i;
    int n_inactive;
    int active_flag;
    int init_req_array;
    int found_nonnull_req;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITANY);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITANY);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (count != 0) {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
		/* NOTE: MPI_STATUS_IGNORE != NULL */
		MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(index, "index", mpi_errno);
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
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
    init_req_array = TRUE;
    found_nonnull_req = FALSE;
    
    MPID_Progress_start(&progress_state);
    for(;;)
    {
	for (i = 0; i < count; i++)
	{
            if (init_req_array)
            {   
#ifdef HAVE_ERROR_CHECKING
                MPID_BEGIN_ERROR_CHECKS;
                {
                    MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], 
						      i, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) goto fn_progress_end_fail;
                }
                MPID_END_ERROR_CHECKS;
#endif /* HAVE_ERROR_CHECKING */
                if (array_of_requests[i] != MPI_REQUEST_NULL)
                {
                    MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
                    /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
                    {
                        MPID_BEGIN_ERROR_CHECKS;
                        {
                            MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
                            if (mpi_errno != MPI_SUCCESS) goto fn_progress_end_fail;
                        }
                        MPID_END_ERROR_CHECKS;
                    }
#endif	    
                }
                else
                {
                    request_ptrs[i] = NULL;
                    ++n_inactive;
                }
            }
            if (request_ptrs[i] == NULL)
                continue;
            /* we found at least one non-null request */
            found_nonnull_req = TRUE;
            
	    if (request_ptrs[i]->kind == MPID_UREQUEST && request_ptrs[i]->poll_fn != NULL)
	    {
                /* this is a generalized request; make progress on it */
		mpi_errno = (request_ptrs[i]->poll_fn)(request_ptrs[i]->grequest_extra_state, status);
		if (mpi_errno != MPI_SUCCESS) goto fn_progress_end_fail;
	    }
            if (MPID_Request_is_complete(request_ptrs[i]))
	    {
		mpi_errno = MPIR_Request_complete(&array_of_requests[i], 
						  request_ptrs[i], status, 
						  &active_flag);
		if (active_flag)
		{
		    *index = i;
		    goto break_l1;
		}
		else
		{
		    ++n_inactive;
		    request_ptrs[i] = NULL;

		    if (n_inactive == count)
		    {
			*index = MPI_UNDEFINED;
			/* status is set to empty by MPIR_Request_complete */
			goto break_l1;
		    }
		}
	    }
	}
        init_req_array = FALSE;

        if (!found_nonnull_req)
        {
            /* all requests were NULL */
            *index = MPI_UNDEFINED;
            if (status != NULL)    /* could be null if count=0 */
                MPIR_Status_set_empty(status);
            goto break_l1;
        }

	mpi_errno = MPID_Progress_wait(&progress_state);
	if (mpi_errno != MPI_SUCCESS) goto fn_progress_end_fail;
    }
  break_l1:
    MPID_Progress_end(&progress_state);

    /* ... end of body of routine ... */
    
  fn_exit:
    if (count > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_FREEALL();
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITANY);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_progress_end_fail:
    MPID_Progress_end(&progress_state);

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_waitany", 
				     "**mpi_waitany %d %p %p %p", 
				     count, array_of_requests, index, status);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
