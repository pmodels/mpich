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

/* -- Begin Profiling Symbol Block for routine MPI_Waitall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitall = PMPI_Waitall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitall  MPI_Waitall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitall as PMPI_Waitall
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Waitall
#define MPI_Waitall PMPI_Waitall

#undef FUNCNAME
#define FUNCNAME MPIR_Waitall_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Waitall_impl(int count, MPI_Request array_of_requests[],
                      MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = request_ptr_array;
    MPI_Status * status_ptr;
    MPID_Progress_state progress_state;
    int i, j;
    int n_completed;
    int active_flag;
    int rc;
    int n_greqs;
    const int ignoring_statuses = (array_of_statuses == MPI_STATUSES_IGNORE);
    MPIU_CHKLMEM_DECL(1);
    
    /* Convert MPI request handles to a request object pointers */
    if (count > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_MALLOC(request_ptrs, MPID_Request **, count * sizeof(MPID_Request *), mpi_errno, "request pointers");
    }

    n_greqs = 0;
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
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif
            if (request_ptrs[i]->kind == MPID_UREQUEST)
                ++n_greqs;
	}
	else
	{
	    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
	    MPIR_Status_set_empty(status_ptr);
	    request_ptrs[i] = NULL;
	    n_completed += 1;
	}
    }
    
    if (n_completed == count)
    {
	goto fn_exit;
    }

    /* Grequest_waitall may run the progress engine - thus, we don't 
       invoke progress_start until after running Grequest_waitall */
    /* first, complete any generalized requests */
    if (n_greqs)
    {
        mpi_errno = MPIR_Grequest_waitall(count, request_ptrs);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    
    MPID_Progress_start(&progress_state);

    for (i = 0; i < count; i++)
    {
        if (request_ptrs[i] == NULL)
        {
            if (!ignoring_statuses)
                array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
            continue;
        }
        
        /* wait for ith request to complete */
        while (!MPID_Request_is_complete(request_ptrs[i]))
        {
            /* generalized requests should already be finished */
            MPIU_Assert(request_ptrs[i]->kind != MPID_UREQUEST);
            
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno != MPI_SUCCESS) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                MPIU_ERR_POP(mpi_errno);
                /* --END ERROR HANDLING-- */
            }
        }

        /* complete the request and check the status */
        status_ptr = (ignoring_statuses) ? MPI_STATUS_IGNORE : &array_of_statuses[i];
        rc = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status_ptr, &active_flag);
        if (rc == MPI_SUCCESS)
        {
            request_ptrs[i] = NULL;
            if (!ignoring_statuses)
                status_ptr->MPI_ERROR = MPI_SUCCESS;
        }
        else
        {
            /* req completed with an error */
            mpi_errno = MPI_ERR_IN_STATUS;
            if (!ignoring_statuses)
            {
                /* set the error code for this request */
                status_ptr->MPI_ERROR = rc;

                /* set the error codes for the rest of the uncompleted requests to PENDING */
                for (j = i+1; j < count; ++j)
                {
                    if (!ignoring_statuses)
                    {
                        if (request_ptrs[j] == NULL)
                        {
                            /* either the user specified MPI_REQUEST_NULL, or this is a completed greq */
                            array_of_statuses[j].MPI_ERROR = MPI_SUCCESS;
                        }
                        else
                        {
                            array_of_statuses[j].MPI_ERROR = MPI_ERR_PENDING;
                        }
                    }
                }
            }
            break;
        }
    }
    MPID_Progress_end(&progress_state);
        
 fn_exit:
     if (count > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_FREEALL();
    }

   return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitall
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
    MPI_Waitall - Waits for all given MPI Requests to complete

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of request handles (array of handles)

Output Parameter:
. array_of_statuses - array of status objects (array of Statuses).  May be
  'MPI_STATUSES_IGNORE'.

Notes:

If one or more of the requests completes with an error, 'MPI_ERR_IN_STATUS' is
returned.  An error value will be present is elements of 'array_of_status'
associated with the requests.  Likewise, the 'MPI_ERROR' field in the status
elements associated with requests that have successfully completed will be
'MPI_SUCCESS'.  Finally, those requests that have not completed will have a 
value of 'MPI_ERR_PENDING'.

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
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitall(int count, MPI_Request array_of_requests[], 
		MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITALL);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            int i;
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (count != 0) {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
		/* NOTE: MPI_STATUSES_IGNORE != NULL */
	    
		MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses", mpi_errno);
		if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    }

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

    mpi_errno = MPIR_Waitall_impl(count, array_of_requests, array_of_statuses);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
 fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITALL);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_waitall",
				     "**mpi_waitall %d %p %p",
				     count, array_of_requests,
				     array_of_statuses);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
