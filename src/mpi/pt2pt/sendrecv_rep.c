/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Sendrecv_replace */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Sendrecv_replace = PMPI_Sendrecv_replace
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Sendrecv_replace  MPI_Sendrecv_replace
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Sendrecv_replace as PMPI_Sendrecv_replace
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Sendrecv_replace
#define MPI_Sendrecv_replace PMPI_Sendrecv_replace

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Sendrecv_replace

/*@
    MPI_Sendrecv_replace - Sends and receives using a single buffer

Input Parameters:
+ count - number of elements in send and receive buffer (integer) 
. datatype - type of elements in send and receive buffer (handle) 
. dest - rank of destination (integer) 
. sendtag - send message tag (integer) 
. source - rank of source (integer) 
. recvtag - receive message tag (integer) 
- comm - communicator (handle) 

Output Parameters:
+ buf - initial address of send and receive buffer (choice) 
- status - status object (Status) 

.N ThreadSafe

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_TRUNCATE
.N MPI_ERR_EXHAUSTED

@*/
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, 
			 int dest, int sendtag, int source, int recvtag,
			 MPI_Comm comm, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Sendrecv_replace";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
#ifdef MPID_LOG_ARROWS
    /* This isn't the right test, but it is close enough for now */
    int sendcount = count, recvcount = count;
#endif
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SENDRECV_REPLACE);
    
    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_PT2PT_FUNC_ENTER_BOTH(MPID_STATE_MPI_SENDRECV_REPLACE);

    /* Convert handles to MPI objects. */
    MPID_Comm_get_ptr(comm, comm_ptr);
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate communicator */
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            if (mpi_errno) goto fn_fail;
	    
	    /* Validate count */
	    MPIR_ERRTEST_COUNT(count, mpi_errno);

	    /* Validate status (status_ignore is not the same as null) */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);

	    /* Validate tags */
	    MPIR_ERRTEST_SEND_TAG(sendtag, mpi_errno);
	    MPIR_ERRTEST_RECV_TAG(recvtag, mpi_errno);

	    /* Validate source and destination */
	    MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
	    MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
            if (mpi_errno) goto fn_fail;

	    /* Validate datatype handle */
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    
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

    /* ... body of routine ... */
    
#   if defined(MPID_Sendrecv_replace)
    {
	mpi_errno = MPID_Sendrecv_replace(buf, count, datatype, dest,
					  sendtag, source, recvtag, comm_ptr, 
					  status)
    }
#   else
    {
	MPID_Request * sreq;
	MPID_Request * rreq;
	void * tmpbuf = NULL;
	int tmpbuf_size = 0;
	int tmpbuf_count = 0;

	if (count > 0 && dest != MPI_PROC_NULL)
	{
	    MPIR_Pack_size_impl(count, datatype, &tmpbuf_size);

	    MPIU_CHKLMEM_MALLOC_ORJUMP(tmpbuf, void *, tmpbuf_size, mpi_errno, "temporary send buffer");

	    mpi_errno = MPIR_Pack_impl(buf, count, datatype, tmpbuf, tmpbuf_size, &tmpbuf_count);
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
	
	mpi_errno = MPID_Irecv(buf, count, datatype, source, recvtag, 
			       comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &rreq);
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	mpi_errno = MPID_Isend(tmpbuf, tmpbuf_count, MPI_PACKED, dest, 
			       sendtag, comm_ptr, MPID_CONTEXT_INTRA_PT2PT, 
			       &sreq);
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    /* FIXME: should we cancel the pending (possibly completed) receive request or wait for it to complete? */
	    MPID_Request_release(rreq);
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}
	
        if (!MPID_Request_is_complete(sreq) || !MPID_Request_is_complete(rreq))
	{
	    MPID_Progress_state progress_state;
	
	    MPID_Progress_start(&progress_state);
            while (!MPID_Request_is_complete(sreq) || !MPID_Request_is_complete(rreq))
	    {
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

	if (status != MPI_STATUS_IGNORE)
	{
	    *status = rreq->status;
	}

	if (mpi_errno == MPI_SUCCESS)
	{
	    mpi_errno = rreq->status.MPI_ERROR;

	    if (mpi_errno == MPI_SUCCESS)
	    {
		mpi_errno = sreq->status.MPI_ERROR;
	    }
	}
    
	MPID_Request_release(sreq);
	MPID_Request_release(rreq);
    }
#   endif

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPID_MPI_PT2PT_FUNC_EXIT_BOTH(MPID_STATE_MPI_SENDRECV_REPLACE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_sendrecv_replace",
	    "**mpi_sendrecv_replace %p %d %D %i %t %i %t %C %p", buf, count, datatype, dest, sendtag,
	    source, recvtag, comm, status);
    }
#   endif
    mpi_errno =  MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
