/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Recv(void * buf, int count, MPI_Datatype datatype, int rank, int tag, 
	      MPID_Comm * comm, int context_offset,
	      MPI_Status * status, MPID_Request ** request)
{
    /* FIXME: in the common case, we want to simply complete the message
       and make as few updates as possible.
       Note in addition that this routine is used only by MPI_Recv (a
       blocking routine; the intent of the interface (which returns 
       a request) was to simplify the handling of the case where the
       message was not found in the unexpected queue. */

    int mpi_errno = MPI_SUCCESS;
    MPID_Request * rreq;
    int found;
    MPIDI_STATE_DECL(MPID_STATE_MPID_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_RECV);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
                      "rank=%d, tag=%d, context=%d", rank, tag,
		      comm->recvcontext_id + context_offset));
    
    if (rank == MPI_PROC_NULL)
    {
	MPIR_Status_set_procnull(status);
	rreq = NULL;
	goto fn_exit;
    }

    MPIU_THREAD_CS_ENTER(MSGQUEUE,);
    rreq = MPIDI_CH3U_Recvq_FDU_or_AEP(rank, tag, 
				       comm->recvcontext_id + context_offset,
                                       comm, buf, count, datatype, &found);
    if (rreq == NULL) {
	MPIU_THREAD_CS_EXIT(MSGQUEUE,);
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomemreq");
    }

    if (found)
    {
	MPIDI_VC_t * vc;

	/* Message was found in the unexepected queue */
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"request found in unexpected queue");

	/* Release the message queue - we've removed this request from 
	   the queue already */
	MPIU_THREAD_CS_EXIT(MSGQUEUE,);
	if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
	{
	    int recv_pending;
	    
	    /* This is an eager message. */
	    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"eager message in the request");

	    if (MPIDI_Request_get_sync_send_flag(rreq))
	    {
		MPIDI_Comm_get_vc_set_active(comm, rreq->dev.match.parts.rank, &vc);
		mpi_errno = MPIDI_CH3_EagerSyncAck( vc, rreq );
		if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    }
	    
            /* the request was found in the unexpected queue, so it has a
               recv_pending_count of at least 1, corresponding to this matching */
            MPIDI_Request_decr_pending(rreq);
            MPIDI_Request_check_pending(rreq, &recv_pending);

            if (MPID_Request_is_complete(rreq)) {
                /* is it ever possible to have (cc==0 && recv_pending>0) ? */
                MPIU_Assert(!recv_pending);

                /* All of the data has arrived, we need to unpack the data and 
                   then free the buffer and the request. */
                if (rreq->dev.recv_data_sz > 0)
                {
                    MPIDI_CH3U_Request_unpack_uebuf(rreq);
                    MPIU_Free(rreq->dev.tmpbuf);
                }

                mpi_errno = rreq->status.MPI_ERROR;
                if (status != MPI_STATUS_IGNORE)
                {
                    *status = rreq->status;
                }

                MPID_Request_release(rreq);
                rreq = NULL;

                goto fn_exit;
            }
	    else
	    {
                /* there should never be outstanding completion events for an unexpected
                 * recv without also having a "pending recv" */
                MPIU_Assert(recv_pending);

		/* The data is still being transfered across the net.  
		   We'll leave it to the progress engine to handle once the
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
	    MPIDI_Comm_get_vc_set_active(comm, rreq->dev.match.parts.rank, &vc);
	    mpi_errno = vc->rndvRecv_fn( vc, rreq );
	    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
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
	    if (status != MPI_STATUS_IGNORE)
	    {
		*status = rreq->status;
	    }
	}
	else
	{
	    /* --BEGIN ERROR HANDLING-- */
            int msg_type = MPIDI_Request_get_msg_type(rreq);
            MPID_Request_release(rreq);
	    rreq = NULL;
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_INTERN, "**ch3|badmsgtype",
                                 "**ch3|badmsgtype %d", msg_type);
	    /* --END ERROR HANDLING-- */
	}
    }
    else
    {
	/* Message has yet to arrived.  The request has been placed on the 
	   list of posted receive requests and populated with
           information supplied in the arguments. */
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"request allocated in posted queue");

	/* FIXME: We do not need to add a datatype reference if
	   the request is blocking.  This is currently added because
	   of the actions that are taken when a request is freed. 
	   (specifically, the datatype and comm both have their refs
	   decremented, and are freed if the refs are zero) */
	if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	{
	    MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
	    MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
	}

	rreq->dev.recv_pending_count = 1;
	/* We must wait until here to exit the msgqueue critical section
	   on this request (we needed to set the recv_pending_count
	   and the datatype pointer) */
	MPIU_THREAD_CS_EXIT(MSGQUEUE,rreq);
    }

  fn_exit:
    *request = rreq;
    MPIU_DBG_STMT(CH3_OTHER,VERBOSE,
    if (rreq)
    {
	MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
		       "request allocated, handle=0x%08x", rreq->handle);
    }
    else
    {
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,
		     "operation complete, no requests allocated");
    });

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_RECV);
    return mpi_errno;
}
