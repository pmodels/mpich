/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

/* FIXME: This should call a channel-provided routine to deliver the 
   cancel message, once the code decides that the request can still
   be cancelled */

int MPID_Cancel_send(MPIR_Request * sreq)
{
    MPIDI_VC_t * vc;
    int proto;
    int flag;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CANCEL_SEND);
    
    MPIR_Assert(sreq->kind == MPIR_REQUEST_KIND__SEND);

    MPIDI_Request_cancel_pending(sreq, &flag);
    if (flag)
    {
	goto fn_exit;
    }

    /*
     * FIXME: user requests returned by MPI_Ibsend() have a NULL comm pointer
     * and no pointer to the underlying communication
     * request.  For now, we simply fail to cancel the request.  In the future,
     * we should add a new request kind to indicate that
     * the request is a BSEND.  Then we can properly cancel the request, much 
     * in the way we do persistent requests.
     */
    if (sreq->comm == NULL)
    {
	goto fn_exit;
    }

    MPIDI_Comm_get_vc_set_active(sreq->comm, sreq->dev.match.parts.rank, &vc);

    proto = MPIDI_Request_get_msg_type(sreq);

    if (proto == MPIDI_REQUEST_SELF_MSG)
    {
	MPIR_Request * rreq;
	
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,
		     "attempting to cancel message sent to self");
	
	MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
	rreq = MPIDI_CH3U_Recvq_FDU(sreq->handle, &sreq->dev.match);
	MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
	if (rreq)
	{
	    MPIR_Assert(rreq->dev.partner_request == sreq);
	    
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
             "send-to-self cancellation successful, sreq=0x%08x, rreq=0x%08x",
						sreq->handle, rreq->handle));

            /* Pull the message out of the unexpected queue since it's
             * being cancelled.  The below request release drops one
             * reference.  We explicitly drop a second reference,
             * because the receive request will never be visible to
             * the user. */
            MPIR_Request_free(rreq);
            MPIR_Request_free(rreq);

	    MPIR_STATUS_SET_CANCEL_BIT(sreq->status, TRUE);
            mpi_errno = MPID_Request_complete(sreq);
            MPIR_ERR_CHECK(mpi_errno);
	}
	else
	{
	    MPIR_STATUS_SET_CANCEL_BIT(sreq->status, FALSE);
	    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
               "send-to-self cancellation failed, sreq=0x%08x, rreq=0x%08x",
						sreq->handle, rreq->handle));
	}
	
	goto fn_exit;
    }

    /* If the message went over a netmod and it provides a cancel_send
       function, call it here. */
#ifdef ENABLE_COMM_OVERRIDES
    if (vc->comm_ops && vc->comm_ops->cancel_send)
    {
        mpi_errno = vc->comm_ops->cancel_send(vc, sreq);
        goto fn_exit;
    }
#endif

    /* Check to see if the send is still in the send queue.  If so, remove it, 
       mark the request and cancelled and complete, and
       release the device's reference to the request object.  
    */
    {
	int cancelled;
	
	if (proto == MPIDI_REQUEST_RNDV_MSG)
	{
	    MPIR_Request * rts_sreq;
	    /* The cancellation of the RTS request needs to be atomic through 
	       the destruction of the RTS request to avoid
               conflict with release of the RTS request if the CTS is received
	       (see handling of a rendezvous CTS packet in
               MPIDI_CH3U_Handle_recv_pkt()).  
	       MPID_Request_fetch_and_clear_rts_sreq() is used to guarantee 
	       that atomicity. */
	    MPIDI_Request_fetch_and_clear_rts_sreq(sreq, &rts_sreq);
	    if (rts_sreq != NULL) 
	    {
		cancelled = FALSE;
		
		/* since we attempted to cancel a RTS request, then we are 
		   responsible for releasing that request */
		MPIR_Request_free(rts_sreq);

		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
						     "**ch3|cancelrndv", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */
		
		if (cancelled)
		{
		    MPIR_STATUS_SET_CANCEL_BIT(sreq->status, TRUE);
		    /* no other thread should be waiting on sreq, so it is 
		       safe to reset ref_count and cc */
                    MPIR_cc_set(&sreq->cc, 0);
                    /* FIXME should be a decr and assert, not a set */
		    MPIR_Object_set_ref(sreq, 1);
		    goto fn_exit;
		}
	    }
	}
	else
	{
	    cancelled = FALSE;
	    if (cancelled)
	    {
		MPIR_STATUS_SET_CANCEL_BIT(sreq->status, TRUE);
		/* no other thread should be waiting on sreq, so it is safe to 
		   reset ref_count and cc */
                MPIR_cc_set(&sreq->cc, 0);
                /* FIXME should be a decr and assert, not a set */
		MPIR_Object_set_ref(sreq, 1);
		goto fn_exit;
	    }
	}
    }

    /* Part or all of the message has already been sent, so we need to send a 
       cancellation request to the receiver in an attempt
       to catch the message before it is matched. */
    {
	int was_incomplete;
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_cancel_send_req_t * const csr_pkt = &upkt.cancel_send_req;
	MPIR_Request * csr_sreq;
	
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
              "sending cancel request to %d for 0x%08x", 
	      sreq->dev.match.parts.rank, sreq->handle));
	
	/* The completion counter and reference count are incremented to keep 
	   the request around long enough to receive a
	   response regardless of what the user does (free the request before 
	   waiting, etc.). */
	MPIDI_CH3U_Request_increment_cc(sreq, &was_incomplete);
	if (!was_incomplete)
	{
	    /* The reference count is incremented only if the request was 
	       complete before the increment. */
	    MPIR_Request_add_ref( sreq );
	}

	MPIDI_Pkt_init(csr_pkt, MPIDI_CH3_PKT_CANCEL_SEND_REQ);
	csr_pkt->match.parts.rank = sreq->comm->rank;
	csr_pkt->match.parts.tag = sreq->dev.match.parts.tag;
	csr_pkt->match.parts.context_id = sreq->dev.match.parts.context_id;
	csr_pkt->sender_req_id = sreq->handle;
	
	MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
	mpi_errno = MPIDI_CH3_iStartMsg(vc, csr_pkt, sizeof(*csr_pkt), &csr_sreq);
	MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|cancelreq");
	}
	if (csr_sreq != NULL)
	{
	    MPIR_Request_free(csr_sreq);
	}
    }
    
    /* FIXME: if send cancellation packets are allowed to arrive out-of-order 
       with respect to send packets, then we need to
       timestamp send and cancel packets to insure that a cancellation request 
       does not bypass the send packet to be cancelled
       and erroneously cancel a previously sent message with the same request 
       handle. */
    /* FIXME: A timestamp is more than is necessary; a message sequence number
       should be adequate. */
 fn_fail:
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CANCEL_SEND);
    return mpi_errno;
}

/*
 * Handler routines called when cancel send packets arrive
 */

int MPIDI_CH3_PktHandler_CancelSendReq( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
					intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_cancel_send_req_t * req_pkt = &pkt->cancel_send_req;
    MPIR_Request * rreq;
    int ack;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_cancel_send_resp_t * resp_pkt = &upkt.cancel_send_resp;
    MPIR_Request * resp_sreq;
    int mpi_errno = MPI_SUCCESS;
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
      "received cancel send req pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
		      req_pkt->sender_req_id, req_pkt->match.parts.rank, 
		      req_pkt->match.parts.tag, req_pkt->match.parts.context_id));
	    
    *buflen = 0;
    /* FIXME: Note that this routine is called from within the packet handler. 
       If the message queue mutex is different from the progress mutex, this 
       must be protected within a message-queue mutex */
    rreq = MPIDI_CH3U_Recvq_FDU(req_pkt->sender_req_id, &req_pkt->match);
    if (rreq != NULL)
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TYPICAL,"message cancelled");
	if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG && rreq->dev.recv_data_sz > 0)
	{
	    MPL_free(rreq->dev.tmpbuf);
	}
	if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_RNDV_MSG)
	{
	    MPIR_Request_free(rreq);
	}
	MPIR_Request_free(rreq);
	ack = TRUE;
    }
    else
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TYPICAL,"unable to cancel message");
	ack = FALSE;
    }
    
    MPIDI_Pkt_init(resp_pkt, MPIDI_CH3_PKT_CANCEL_SEND_RESP);
    resp_pkt->sender_req_id = req_pkt->sender_req_id;
    resp_pkt->ack = ack;
    /* FIXME: This is called within the packet handler */
    /* MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex); */
    mpi_errno = MPIDI_CH3_iStartMsg(vc, resp_pkt, sizeof(*resp_pkt), &resp_sreq);
    /* MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex); */
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
			    "**ch3|cancelresp");
    }
    if (resp_sreq != NULL)
    {
	MPIR_Request_free(resp_sreq);
    }
    
    *rreqp = NULL;

 fn_fail:
    return mpi_errno;
}

int MPIDI_CH3_PktHandler_CancelSendResp( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
					 MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
					 intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_cancel_send_resp_t * resp_pkt = &pkt->cancel_send_resp;
    MPIR_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
			"received cancel send resp pkt, sreq=0x%08x, ack=%d",
			resp_pkt->sender_req_id, resp_pkt->ack));
	    
    *buflen = 0;

    MPIR_Request_get_ptr(resp_pkt->sender_req_id, sreq);
    
    if (resp_pkt->ack)
    {
	MPIR_STATUS_SET_CANCEL_BIT(sreq->status, TRUE);
	
	if (MPIDI_Request_get_msg_type(sreq) == MPIDI_REQUEST_RNDV_MSG ||
	    MPIDI_Request_get_type(sreq) == MPIDI_REQUEST_TYPE_SSEND)
	{
	    /* decrement the CC one additional time for the CTS/sync ack that 
	       is never going to arrive */
	    MPIDI_CH3U_Request_dec_cc(sreq);
	}
		
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TYPICAL,"message cancelled");
    }
    else
    {
	MPIR_STATUS_SET_CANCEL_BIT(sreq->status, FALSE);
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TYPICAL,"unable to cancel message");
    }
    
    mpi_errno = MPID_Request_complete(sreq);
    MPIR_ERR_CHECK(mpi_errno);

    *rreqp = NULL;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
 * Define the routines that can print out the cancel packets if 
 * debugging is enabled.
 */
#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_CancelSendReq( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... CANCEL_SEND\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," context_id ... %d\n", pkt->cancel_send_req.match.parts.context_id);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," tag .......... %d\n", pkt->cancel_send_req.match.parts.tag);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," rank ......... %d\n", pkt->cancel_send_req.match.parts.rank);

    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_CancelSendResp( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... CANCEL_SEND_RESP\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," ack .......... %d\n", pkt->cancel_send_resp.ack);
    
    return MPI_SUCCESS;
}
#endif
