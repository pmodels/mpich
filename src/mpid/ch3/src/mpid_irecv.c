/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Irecv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Irecv(void * buf, int count, MPI_Datatype datatype, int rank, int tag,
	       MPID_Comm * comm, int context_offset,
               MPID_Request ** request)
{
    MPID_Request * rreq;
    int found;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_IRECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_IRECV);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
			"rank=%d, tag=%d, context=%d", 
			rank, tag, comm->recvcontext_id + context_offset));
    
    if (rank == MPI_PROC_NULL)
    {
	rreq = MPID_Request_create();
	if (rreq != NULL)
	{
	    MPIU_Object_set_ref(rreq, 1);
	    rreq->cc = 0;
	    rreq->kind = MPID_REQUEST_RECV;
	    MPIR_Status_set_procnull(&rreq->status);
	}
	else
	{
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_NO_MEM, "**nomem");
	}
	goto fn_exit;
    }

    rreq = MPIDI_CH3U_Recvq_FDU_or_AEP(
	rank, tag, comm->recvcontext_id + context_offset, &found);
    if (rreq == NULL)
    {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_NO_MEM, "**nomem");
    }

    rreq->comm		 = comm;
    MPIR_Comm_add_ref(comm);
    rreq->dev.user_buf	 = buf;
    rreq->dev.user_count = count;
    rreq->dev.datatype	 = datatype;

    if (found)
    {
	MPIDI_VC_t * vc;
	
	/* Message was found in the unexepected queue */
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"request found in unexpected queue");

	if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
	{
	    int recv_pending;
	    
	    /* This is an eager message */
	    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"eager message in the request");
	    
	    /* If this is a eager synchronous message, then we need to send an 
	       acknowledgement back to the sender. */
	    if (MPIDI_Request_get_sync_send_flag(rreq))
	    {
		MPIDI_Comm_get_vc(comm, rreq->dev.match.rank, &vc);
		mpi_errno = MPIDI_CH3_EagerSyncAck( vc, rreq );
		if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    }

            MPIDI_Request_recv_pending(rreq, &recv_pending);
	    if (!recv_pending)
	    {
		/* All of the data has arrived, we need to copy the data and 
		   then free the buffer. */
		/* FIXME: if the user buffer is contiguous, just move the
		   data without using a separate routine call */
		if (rreq->dev.recv_data_sz > 0)
		{
		    MPIDI_CH3U_Request_unpack_uebuf(rreq);
		    MPIU_Free(rreq->dev.tmpbuf);
		}

		mpi_errno = rreq->status.MPI_ERROR;
		goto fn_exit;
	    }
	    else
	    {
		/* The data is still being transfered across the net.  We'll 
		   leave it to the progress engine to handle once the
		   entire message has arrived. */
		if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
		{
		    MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
		    MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
		}
	    
	    }
	}
	else if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_RNDV_MSG)
	{
	    MPIDI_Comm_get_vc(comm, rreq->dev.match.rank, &vc);
	
	    mpi_errno = vc->rndvRecv_fn( vc, rreq );
	    if (mpi_errno) MPIU_ERR_POP( mpi_errno );
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
		MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
	    }
	}
	else if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_SELF_MSG)
	{
	    mpi_errno = MPIDI_CH3_RecvFromSelf( rreq, buf, count, datatype );
	    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	}
	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPID_Request_release(rreq);
	    rreq = NULL;
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_INTERN, "**ch3|badmsgtype",
		      "**ch3|badmsgtype %d", MPIDI_Request_get_msg_type(rreq));
	    /* --END ERROR HANDLING-- */
	}
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the 
	   list of posted receive requests and populated with
           information supplied in the arguments. */
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"request allocated in posted queue");
	
	if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	{
	    MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
	    MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
	}

	rreq->dev.recv_pending_count = 1;
    }

  fn_exit:
    *request = rreq;
    MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,"request allocated, handle=0x%08x", 
		   rreq->handle);

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_IRECV);
    return mpi_errno;
}
