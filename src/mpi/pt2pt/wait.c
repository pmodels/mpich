/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Wait */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wait = PMPI_Wait
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wait  MPI_Wait
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wait as PMPI_Wait
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Wait
#define MPI_Wait PMPI_Wait

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Wait

/*@
    MPI_Wait - Waits for an MPI request to complete

Input Parameter:
. request - request (handle) 

Output Parameter:
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
int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Wait";
    MPID_Request * request_ptr = NULL;
    int active_flag;
    int mpi_errno = MPI_SUCCESS;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAIT);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    if (mpi_errno) goto fn_fail;

	    MPIR_ERRTEST_REQUEST_OR_NULL(*request, mpi_errno);
	    if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL)
    {
	MPIR_Status_set_empty(status);
	goto fn_exit;
    }
    
    /* Convert MPI request handle to a request object pointer */
    MPID_Request_get_ptr(*request, request_ptr);
    
    /* Validate object pointers if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    
    if ((*(request_ptr)->cc_ptr) != 0)
    {
	MPID_Progress_state progress_state;
	    
	MPID_Progress_start(&progress_state);
	while((*(request_ptr)->cc_ptr) != 0)
	{
	    mpi_errno = MPIR_Grequest_progress_poke(1, &request_ptr, status);
	    if (request_ptr->kind == MPID_UREQUEST && 
			    request_ptr->wait_fn != NULL) 
	    {
		if (mpi_errno != MPI_SUCCESS)
		{
		    /* --BEGIN ERROR HANDLING-- */ 
		    MPID_Progress_end(&progress_state);
		    goto fn_fail;
		    /* --END ERROR HANDLING-- */
		}
		continue; /* treating UREQUEST like normal request means we'll
			     poll indefinitely. skip over progress_wait */ 
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
    }

    mpi_errno = MPIR_Request_complete(request, request_ptr, status, 
				      &active_flag);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAIT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
	
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_wait", "**mpi_wait %p %p", 
				     request, status);
#endif
    mpi_errno = MPIR_Err_return_comm((request_ptr != NULL) ? request_ptr->comm : NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
