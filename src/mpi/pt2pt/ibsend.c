/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ibsend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibsend = PMPI_Ibsend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibsend  MPI_Ibsend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibsend as PMPI_Ibsend
#endif
/* -- End Profiling Symbol Block */

typedef struct {
    MPID_Request *req;
    int           cancelled;
} ibsend_req_info;

PMPI_LOCAL int MPIR_Ibsend_query( void *extra, MPI_Status *status );
PMPI_LOCAL int MPIR_Ibsend_free( void *extra );
PMPI_LOCAL int MPIR_Ibsend_cancel( void *extra, int complete );

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibsend
#define MPI_Ibsend PMPI_Ibsend

PMPI_LOCAL int MPIR_Ibsend_query( void *extra, MPI_Status *status )
{
    ibsend_req_info *ibsend_info = (ibsend_req_info *)extra;
    status->cancelled = ibsend_info->cancelled;
    return MPI_SUCCESS;
}
PMPI_LOCAL int MPIR_Ibsend_free( void *extra )
{
    ibsend_req_info *ibsend_info = (ibsend_req_info *)extra;

    /* Release the MPID_Request (there is still another ref pending
     within the bsendutil functions) */
    /* XXX DJG FIXME-MT should we be checking this? */
    if (MPIU_Object_get_ref(ibsend_info->req) > 1) {
        int inuse;
	/* Note that this should mean that the request was 
	   cancelled (that would have decremented the ref count)
	 */
        MPIR_Request_release_ref( ibsend_info->req, &inuse );
    }

    MPIU_Free( ibsend_info );
    return MPI_SUCCESS;
}
#undef FUNCNAME
#define FUNCNAME MPIR_Ibsend_cancel
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
PMPI_LOCAL int MPIR_Ibsend_cancel( void *extra, int complete )
{
    int mpi_errno = MPI_SUCCESS;
    ibsend_req_info *ibsend_info = (ibsend_req_info *)extra;
    MPI_Status status;
    MPID_Request *req = ibsend_info->req;

    /* FIXME: There should be no unreferenced args! */
    /* Note that this value should always be 1 because 
       Grequest_complete is called on this request when it is
       created */
    MPIU_UNREFERENCED_ARG(complete);


    /* Try to cancel the underlying request */
    mpi_errno = MPIR_Cancel_impl(req);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Wait_impl( &req->handle, &status );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPIR_Test_cancelled_impl( &status, &ibsend_info->cancelled );
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibsend_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibsend_impl(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                     MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr, *new_request_ptr;
    ibsend_req_info *ibinfo=0;
        
    /* We don't try tbsend in for MPI_Ibsend because we must create a
       request even if we can send the message */

    mpi_errno = MPIR_Bsend_isend( buf, count, datatype, dest, tag, comm_ptr, 
				  IBSEND, &request_ptr );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* FIXME: use the memory management macros */
    ibinfo = (ibsend_req_info *)MPIU_Malloc( sizeof(ibsend_req_info) );
    ibinfo->req       = request_ptr;
    ibinfo->cancelled = 0;
    mpi_errno = MPIR_Grequest_start_impl( MPIR_Ibsend_query, MPIR_Ibsend_free,
                                          MPIR_Ibsend_cancel, ibinfo, &new_request_ptr );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    /* The request is immediately complete because the MPIR_Bsend_isend has
       already moved the data out of the user's buffer */
    MPIR_Request_add_ref( request_ptr );
    /* Request count is now 2 (set to 1 in Grequest_start) */
    MPIR_Grequest_complete_impl(new_request_ptr);
    MPIU_OBJ_PUBLISH_HANDLE(*request, new_request_ptr->handle);
  
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Ibsend
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
    MPI_Ibsend - Starts a nonblocking buffered send

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Output Parameter:
. request - communication request (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_BUFFER

@*/
int MPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	       MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IBSEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_IBSEND);

    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count,mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr) {
		MPIR_ERRTEST_SEND_TAG(tag,mpi_errno);
		MPIR_ERRTEST_SEND_RANK(comm_ptr,dest,mpi_errno)
	    }

	    /* Validate datatype handle */
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (mpi_errno) goto fn_fail;

	    /* Validate datatype object */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype *datatype_ptr = NULL;

		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno) goto fn_fail;
	    }
	    
	    /* Validate buffer */
	    MPIR_ERRTEST_USERBUFFER(buf,count,datatype,mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ibsend_impl(buf, count, datatype, dest, tag, comm_ptr, request);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IBSEND);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    /* FIXME: should we be setting the request at all in the case of an error? */
    *request = MPI_REQUEST_NULL; 
#   ifdef HAVE_ERROR_REPORTING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_ibsend",
	    "**mpi_ibsend %p %d %D %i %t %C %p", 
	    buf, count, datatype, dest, tag, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
