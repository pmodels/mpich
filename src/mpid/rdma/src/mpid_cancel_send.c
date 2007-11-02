/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Cancel_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Cancel_send(MPID_Request * sreq)
{
    MPIDI_VC * vc;
    int proto;
    int flag;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CANCEL_SEND);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    assert(sreq->kind == MPID_REQUEST_SEND);

    MPIDI_Request_cancel_pending(sreq, &flag);
    if (flag)
    {
	goto fn_exit;
    }
    
    vc = sreq->comm->vcr[sreq->dev.match.rank];

    proto = MPIDI_Request_get_msg_type(sreq);

    if (proto == MPIDI_REQUEST_SELF_MSG)
    {
	MPID_Request * rreq;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "attempting to cancel message sent to self"));
	
	rreq = MPIDI_CH3U_Recvq_FDU(sreq->handle, &sreq->dev.match);
	if (rreq)
	{
	    assert(rreq->partner_request == sreq);
	    
	    MPIDI_DBG_PRINTF((15, FCNAME, "send-to-self cancellation successful, sreq=0x%08x, rreq=0x%08x",
			      sreq->handle, rreq->handle));
	    
	    MPIU_Object_set_ref(rreq, 0);
	    MPIDI_CH3_Request_destroy(rreq);
	    
	    sreq->status.cancelled = TRUE;
	    /* no other thread should be waiting on sreq, so it is safe to reset ref_count and cc */
	    sreq->cc = 0;
	    MPIU_Object_set_ref(sreq, 1);
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "send-to-self cancellation failed, sreq=0x%08x, rreq=0x%08x",
			      sreq->handle, rreq->handle));
	}
	
	goto fn_exit;
    }

    /* Check to see if the send is still in the send queue.  If so, remove it, mark the request and cancelled and complete, and
       release the device's reference to the request object.  QUESTION: what is the right interface for MPIDI_CH3_Send_cancel()?
       It needs to be able to cancel requests to send a RTS packet for this request.  Perhaps we can use the partner request
       field to track RTS requests. */
    {
	int cancelled;
	
	if (proto == MPIDI_REQUEST_RNDV_MSG)
	{
	    MPID_Request * rts_sreq;
	    /* The cancellation of the RTS request needs to be atomic through the destruction of the RTS request to avoid
               conflict with release of the RTS request if the CTS is received (see handling of a rendezvous CTS packet in
               MPIDI_CH3U_Handle_recv_pkt()).  MPID_Request_fetch_and_clear_rts_sreq() is used to gurantee that atomicity. */
	    MPIDI_Request_fetch_and_clear_rts_sreq(sreq, &rts_sreq);
	    if (rts_sreq != NULL) 
	    {
		mpi_errno = MPIDI_CH3_Cancel_send(vc, rts_sreq, &cancelled);
		
		/* since we attempted to cancel a RTS request, then we are responsible for releasing that request */
		MPID_Request_release(rts_sreq);
		
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|cancelrndv", 0);
		    goto fn_exit;
		}
		
		if (cancelled)
		{
		    sreq->status.cancelled = TRUE;
		    /* no other thread should be waiting on sreq, so it is safe to reset ref_count and cc */
		    sreq->cc = 0;
		    MPIU_Object_set_ref(sreq, 1);
		    goto fn_exit;
		}
	    }
	}
	else
	{
	    mpi_errno = MPIDI_CH3_Cancel_send(vc, sreq, &cancelled);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|canceleager", 0);
		goto fn_exit;
	    }
	    if (cancelled)
	    {
		sreq->status.cancelled = TRUE;
		/* no other thread should be waiting on sreq, so it is safe to reset ref_count and cc */
		sreq->cc = 0;
		MPIU_Object_set_ref(sreq, 1);
		goto fn_exit;
	    }
	}
    }

    /* Part or all of the message has already been sent, so we need to send a cancellation request to the receiver in an attempt
       to catch the message before it is matched. */
    {
	int was_incomplete;
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_cancel_send_req_t * const csr_pkt = &upkt.cancel_send_req;
	MPID_Request * csr_sreq;
	
	MPIDI_DBG_PRINTF((15, FCNAME, "sending cancel request to %d for 0x%08x", sreq->dev.match.rank, sreq->handle));
	
	/* The completion counter and reference count are incremented to keep the request around long enough to receive a
	   response regardless of what the user does (free the request before waiting, etc.). */
	MPIDI_CH3U_Request_increment_cc(sreq, &was_incomplete);
	if (!was_incomplete)
	{
	    /* The reference count is incremented only if the request was complete before the increment. */
	    MPIDI_CH3_Request_add_ref(sreq);
	}

	csr_pkt->type = MPIDI_CH3_PKT_CANCEL_SEND_REQ;
	csr_pkt->match.rank = sreq->comm->rank;
	csr_pkt->match.tag = sreq->dev.match.tag;
	csr_pkt->match.context_id = sreq->dev.match.context_id;
	csr_pkt->sender_req_id = sreq->handle;
	
	mpi_errno = MPIDI_CH3_iStartMsg(vc, csr_pkt, sizeof(*csr_pkt), &csr_sreq);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|cancelreq", 0);
	    goto fn_exit;
	}
	if (csr_sreq != NULL)
	{
	    MPID_Request_release(csr_sreq);
	}
    }
    
    /* FIXME: if send cancellation packets are allowed to arrive out-of-order with respect to send packets, then we need to
       timestamp send and cancel packets to insure that a cancellation request does not bypass the send packet to be cancelled
       and erroneously cancel a previously sent message with the same request handle. */

  fn_exit:
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CANCEL_SEND);
    return mpi_errno;
}
