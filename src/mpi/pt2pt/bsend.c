/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Bsend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bsend = PMPI_Bsend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bsend  MPI_Bsend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bsend as PMPI_Bsend
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Bsend
#define MPI_Bsend PMPI_Bsend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Bsend

/*@
    MPI_Bsend - Basic send with user-provided buffering

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (nonnegative integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
This send is provided as a convenience function; it allows the user to 
send messages without worring about where they are buffered (because the
user `must` have provided buffer space with 'MPI_Buffer_attach').  

In deciding how much buffer space to allocate, remember that the buffer space 
is not available for reuse by subsequent 'MPI_Bsend's unless you are certain 
that the message
has been received (not just that it should have been received).  For example,
this code does not allocate enough buffer space
.vb
    MPI_Buffer_attach( b, n*sizeof(double) + MPI_BSEND_OVERHEAD );
    for (i=0; i<m; i++) {
        MPI_Bsend( buf, n, MPI_DOUBLE, ... );
    }
.ve
because only enough buffer space is provided for a single send, and the
loop may start a second 'MPI_Bsend' before the first is done making use of the
buffer.  

In C, you can 
force the messages to be delivered by 
.vb
    MPI_Buffer_detach( &b, &n );
    MPI_Buffer_attach( b, n );
.ve
(The 'MPI_Buffer_detach' will not complete until all buffered messages are 
delivered.)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_RANK
.N MPI_ERR_TAG

.seealso: MPI_Buffer_attach, MPI_Ibsend, MPI_Bsend_init
@*/
int MPI_Bsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	      MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Bsend";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BSEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_BSEND);
    
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

    /* Validate object pointers if error checking is enabled */
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
    
#   ifdef MPID_HAS_TBSEND
    {
	mpi_errno = MPID_tBsend( buf, count, datatype, dest, tag, comm_ptr, 0 );
	if (mpi_errno == MPI_SUCCESS)
	{
	    goto fn_exit;
	}
	/* FIXME: Check for MPID_WOULD_BLOCK? */
    }
#   endif
    
    mpi_errno = MPIR_Bsend_isend( buf, count, datatype, dest, tag, comm_ptr, 
				  BSEND, &request_ptr );
    /* Note that we can ignore the request_ptr because it is handled internally
       by the bsend util routines */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
	
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_bsend",
	    "**mpi_bsend %p %d %D %i %t %C", buf, count, datatype, dest, tag, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
