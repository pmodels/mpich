/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Grequest_complete */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Grequest_complete = PMPI_Grequest_complete
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Grequest_complete  MPI_Grequest_complete
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Grequest_complete as PMPI_Grequest_complete
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Grequest_complete(MPI_Request request) __attribute__((weak,alias("PMPI_Grequest_complete")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Grequest_complete
#define MPI_Grequest_complete PMPI_Grequest_complete

/* Any internal routines can go here.  Make them static if possible.  If they
   are used by both the MPI and PMPI versions, use PMPI_LOCAL instead of 
   static; this macro expands into "static" if weak symbols are supported and
   into nothing otherwise. */
#undef FUNCNAME
#define FUNCNAME MPIR_Grequest_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Grequest_complete_impl(MPID_Request *request_ptr)
{
    /* Set the request as completed.  This does not change the
       reference count on the generalized request */
    MPID_Request_complete( request_ptr );

    /* The request release comes with the wait/test, not this complete
       routine, so we don't call the MPID_Request_release routine */
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Grequest_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Grequest_complete - Notify MPI that a user-defined request is complete

Input Parameters:
.  request - Generalized request to mark as complete

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso: MPI_Grequest_start
@*/
int MPI_Grequest_complete( MPI_Request request )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GREQUEST_COMPLETE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GREQUEST_COMPLETE);
    
    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_REQUEST(request, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( request, request_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Request_valid_ptr(request_ptr,mpi_errno);
	    if (request_ptr && request_ptr->kind != MPID_UREQUEST) {
 	        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, 
						  "**notgenreq", 0 );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Grequest_complete_impl(request_ptr);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GREQUEST_COMPLETE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_grequest_complete",
	    "**mpi_grequest_complete %R", request);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
