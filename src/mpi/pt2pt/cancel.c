/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cancel */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cancel = PMPI_Cancel
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cancel  MPI_Cancel
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cancel as PMPI_Cancel
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cancel
#define MPI_Cancel PMPI_Cancel


#undef FUNCNAME
#define FUNCNAME MPIR_Cancel_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Cancel_impl(MPID_Request *request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
        
    switch (request_ptr->kind)
    {
	case MPID_REQUEST_SEND:
	{
	    mpi_errno = MPID_Cancel_send(request_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    break;
	}

	case MPID_REQUEST_RECV:
	{
	    mpi_errno = MPID_Cancel_recv(request_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    break;
	}

	case MPID_PREQUEST_SEND:
	{
	    if (request_ptr->partner_request != NULL) {
		if (request_ptr->partner_request->kind != MPID_UREQUEST) {
		    mpi_errno = MPID_Cancel_send(request_ptr->partner_request);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		} else {
		    /* This is needed for persistent Bsend requests */
                    /* FIXME why do we directly access the partner request's
                     * cc field?  shouldn't our cc_ptr be set to the address
                     * of the partner req's cc field? */
                    mpi_errno = MPIR_Grequest_cancel(request_ptr->partner_request,
                                                     MPID_cc_is_complete(&request_ptr->partner_request->cc));
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
		}
	    } else {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_REQUEST,"**requestpersistactive");
	    }
	    break;
	}

	case MPID_PREQUEST_RECV:
	{
	    if (request_ptr->partner_request != NULL) {
		mpi_errno = MPID_Cancel_recv(request_ptr->partner_request);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    } else {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_REQUEST,"**requestpersistactive");
	    }
	    break;
	}

	case MPID_UREQUEST:
	{
            mpi_errno = MPIR_Grequest_cancel(request_ptr, MPID_cc_is_complete(&request_ptr->cc));
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    break;
	}

	/* --BEGIN ERROR HANDLING-- */
	default:
	{
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,"**cancelunknown");
	}
	/* --END ERROR HANDLING-- */
    }

 fn_exit:
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}
#endif


#undef FUNCNAME
#define FUNCNAME MPI_Cancel
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
    MPI_Cancel - Cancels a communication request

Input Parameter:
. request - communication request (handle) 

Notes:
The primary expected use of 'MPI_Cancel' is in multi-buffering
schemes, where speculative 'MPI_Irecvs' are made.  When the computation 
completes, some of these receive requests may remain; using 'MPI_Cancel' allows
the user to cancel these unsatisfied requests.  

Cancelling a send operation is much more difficult, in large part because the 
send will usually be at least partially complete (the information on the tag,
size, and source are usually sent immediately to the destination).  
Users are
advised that cancelling a send, while a local operation (as defined by the MPI
standard), is likely to be expensive (usually generating one or more internal
messages). 

.N ThreadSafe

.N Fortran

.N NULL

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Cancel(MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * request_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CANCEL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_CANCEL);
    
    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( *request, request_ptr );
    
    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate request_ptr */
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Cancel_impl(request_ptr);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
	    FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_cancel",
	    "**mpi_cancel %p", request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
