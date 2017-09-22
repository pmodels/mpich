/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * This file contains the implementation of the rendezvous protocol
 * for MPI point-to-point messaging.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_RndvSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDI_CH3_RndvSend - Send a request to perform a rendezvous send */
int MPIDI_CH3_RndvSend( MPIR_Request **sreq_p, const void * buf, MPI_Aint count,
			MPI_Datatype datatype, int dt_contig, intptr_t data_sz,
			MPI_Aint dt_true_lb,
			int rank, 
			int tag, MPIR_Comm * comm, int context_offset )
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_rndv_req_to_send_t * const rts_pkt = &upkt.rndv_req_to_send;
    MPIDI_VC_t * vc;
    MPIR_Request * rts_sreq;
    MPIR_Request *sreq =*sreq_p;
    int          mpi_errno = MPI_SUCCESS;
	
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,VERBOSE,
		   "sending rndv RTS, data_sz=%" PRIdPTR, data_sz);

    sreq->dev.OnDataAvail = 0;
    
    sreq->dev.partner_request = NULL;
	
    MPIDI_Pkt_init(rts_pkt, MPIDI_CH3_PKT_RNDV_REQ_TO_SEND);
    rts_pkt->match.parts.rank	      = comm->rank;
    rts_pkt->match.parts.tag	      = tag;
    rts_pkt->match.parts.context_id = comm->context_id + context_offset;
    rts_pkt->sender_req_id    = sreq->handle;
    rts_pkt->data_sz	      = data_sz;

    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(rts_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);

    MPL_DBG_MSGPKT(vc,tag,rts_pkt->match.parts.context_id,rank,data_sz,"Rndv");

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, rts_pkt, sizeof(*rts_pkt), &rts_sreq);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        MPIR_Request_free(sreq);
	*sreq_p = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rtspkt");
    }
    /* --END ERROR HANDLING-- */
    if (rts_sreq != NULL)
    {
	if (rts_sreq->status.MPI_ERROR != MPI_SUCCESS)
	{
            MPIR_Request_free(sreq);
	    *sreq_p = NULL;
            mpi_errno = rts_sreq->status.MPI_ERROR;
            MPIR_Request_free(rts_sreq);
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rtspkt");
	}
	MPIR_Request_free(rts_sreq);
    }

    /* FIXME: fill temporary IOV or pack temporary buffer after send to hide 
       some latency.  This requires synchronization
       because the CTS packet could arrive and be processed before the above 
       iStartmsg completes (depending on the progress
       engine, threads, etc.). */

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* 
 * Here are the routines that are called by the progress engine to handle
 * the various rendezvous message requests (cancel of sends is in 
 * mpid_cancel_send.c).
 */    

#define set_request_info(rreq_, pkt_, msg_type_)		\
{								\
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.parts.rank;	\
    (rreq_)->status.MPI_TAG = (pkt_)->match.parts.tag;		\
    MPIR_STATUS_SET_COUNT((rreq_)->status, (pkt_)->data_sz);		\
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;		\
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;		\
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);		\
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));		\
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_RndvReqToSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_RndvReqToSend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
					intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIR_Request * rreq;
    int found;
    MPIDI_CH3_Pkt_rndv_req_to_send_t * rts_pkt = &pkt->rndv_req_to_send;
    int mpi_errno = MPI_SUCCESS;
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
 "received rndv RTS pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d, data_sz=%" PRIdPTR,
	      rts_pkt->sender_req_id, rts_pkt->match.parts.rank, 
					rts_pkt->match.parts.tag, 
              rts_pkt->match.parts.context_id, rts_pkt->data_sz));
    MPL_DBG_MSGPKT(vc,rts_pkt->match.parts.tag,rts_pkt->match.parts.context_id,
		    rts_pkt->match.parts.rank,rts_pkt->data_sz,
		    "ReceivedRndv");

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&rts_pkt->match, &found);
    MPIR_ERR_CHKANDJUMP1(!rreq, mpi_errno,MPI_ERR_OTHER, "**nomemreq", "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());

    /* If the completion counter is 0, that means that the communicator to
     * which this message is being sent has been revoked and we shouldn't
     * bother finishing this. */
    if (!found && MPIR_cc_get(rreq->cc) == 0) {
        *rreqp = NULL;
        goto fn_fail;
    }
    
    set_request_info(rreq, rts_pkt, MPIDI_REQUEST_RNDV_MSG);

    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);

    *buflen = 0;
    
    if (found)
    {
	MPIR_Request * cts_req;
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &upkt.rndv_clr_to_send;
	
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"posted request found");
	
	/* FIXME: What if the receive user buffer is not big enough to
	   hold the data about to be cleared for sending? */
	
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending rndv CTS packet");
	MPIDI_Pkt_init(cts_pkt, MPIDI_CH3_PKT_RNDV_CLR_TO_SEND);
	cts_pkt->sender_req_id = rts_pkt->sender_req_id;
	cts_pkt->receiver_req_id = rreq->handle;
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
	mpi_errno = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt), &cts_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|ctspkt");
	}
	if (cts_req != NULL) {
	    MPIR_Request_free(cts_req);
	}
    }
    else
    {
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"unexpected request allocated");
	
	/*
	 * A MPID_Probe() may be waiting for the request we just 
	 * inserted, so we need to tell the progress engine to exit.
	 *
	 * FIXME: This will cause MPID_Progress_wait() to return to the
	 * MPI layer each time an unexpected RTS packet is
	 * received.  MPID_Probe() should atomically increment a
	 * counter and MPIDI_CH3_Progress_signal_completion()
	 * should only be called if that counter is greater than zero.
	 */
	MPIDI_CH3_Progress_signal_completion();
    }
    
    *rreqp = NULL;

 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_RndvClrToSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_RndvClrToSend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
					intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &pkt->rndv_clr_to_send;
    MPIR_Request * sreq;
    MPIR_Request * rts_sreq;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &upkt.rndv_send;
    int dt_contig;
    MPI_Aint dt_true_lb;
    intptr_t data_sz;
    MPIR_Datatype* dt_ptr;
    int mpi_errno = MPI_SUCCESS;
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"received rndv CTS pkt");
    
    MPIR_Request_get_ptr(cts_pkt->sender_req_id, sreq);
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST,"received cts, count=" MPI_AINT_FMT_DEC_SPEC "\n", sreq->dev.user_count));

    sreq->dev.OnDataAvail = 0;
    sreq->dev.OnFinal = 0;

    /* Release the RTS request if one exists.  
       MPID_Request_fetch_and_clear_rts_sreq() needs to be atomic to 
       prevent
       cancel send from cancelling the wrong (future) request.  
       If MPID_Request_fetch_and_clear_rts_sreq() returns a NULL
       rts_sreq, then MPID_Cancel_send() is responsible for releasing 
       the RTS request object. */
    MPIDI_Request_fetch_and_clear_rts_sreq(sreq, &rts_sreq);
    if (rts_sreq != NULL)
    {
	MPIR_Request_free(rts_sreq);
    }

    *buflen = 0;

    MPIDI_Pkt_init(rs_pkt, MPIDI_CH3_PKT_RNDV_SEND);
    rs_pkt->receiver_req_id = cts_pkt->receiver_req_id;
    
    MPIDI_Datatype_get_info(sreq->dev.user_count, sreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    
    if (dt_contig) 
    {
	MPL_IOV iov[MPL_IOV_LIMIT];

	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
		    "sending contiguous rndv data, data_sz=%" PRIdPTR,
					    data_sz));
	
	iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)rs_pkt;
	iov[0].MPL_IOV_LEN = sizeof(*rs_pkt);
	
	iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)((char *)sreq->dev.user_buf + dt_true_lb);
	iov[1].MPL_IOV_LEN = data_sz;

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
	mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, 2);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
	MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|senddata");
    }
    else
    {
	sreq->dev.segment_ptr = MPIR_Segment_alloc( );
        MPIR_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");
	MPIR_Segment_init(sreq->dev.user_buf, sreq->dev.user_count, 
			  sreq->dev.datatype, sreq->dev.segment_ptr, 0);
	sreq->dev.segment_first = 0;
	sreq->dev.segment_size = data_sz;

	MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
	mpi_errno = vc->sendNoncontig_fn(vc, sreq, rs_pkt, sizeof(*rs_pkt));
	MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
	MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|senddata");
    }    
    *rreqp = NULL;

 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_RndvSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_RndvSend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data,
				   intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_rndv_send_t * rs_pkt = &pkt->rndv_send;
    int mpi_errno = MPI_SUCCESS;
    int complete;
    intptr_t data_len;
    MPIR_Request *req;
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"received rndv send (data) pkt");

    MPIR_Request_get_ptr(rs_pkt->receiver_req_id, req);

    data_len = ((*buflen >= req->dev.recv_data_sz)
                ? req->dev.recv_data_sz : *buflen);
    
    if (req->dev.recv_data_sz == 0) {
        *buflen = 0;
        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
	*rreqp = NULL;
    }
    else {
        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data, &data_len,
                                                  &complete);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**ch3|postrecv",
			     "**ch3|postrecv %s", "MPIDI_CH3_PKT_RNDV_SEND");
	}

        *buflen = data_len;

        if (complete) 
        {
            mpi_errno = MPID_Request_complete(req);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
            *rreqp = NULL;
        }
        else
        {
            *rreqp = req;
        }
   }
	
 fn_fail:
    return mpi_errno;
}

/*
 * This routine processes a rendezvous message once the message is matched.
 * It is used in mpid_recv and mpid_irecv.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_RecvRndv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_RecvRndv( MPIDI_VC_t * vc, MPIR_Request *rreq )
{
    int mpi_errno = MPI_SUCCESS;

    /* A rendezvous request-to-send (RTS) message has arrived.  We need
       to send a CTS message to the remote process. */
    MPIR_Request * cts_req;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_rndv_clr_to_send_t * cts_pkt = &upkt.rndv_clr_to_send;
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,
		 "rndv RTS in the request, sending rndv CTS");
    
    MPIDI_Pkt_init(cts_pkt, MPIDI_CH3_PKT_RNDV_CLR_TO_SEND);
    cts_pkt->sender_req_id = rreq->dev.sender_req_id;
    cts_pkt->receiver_req_id = rreq->handle;
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cts_pkt, sizeof(*cts_pkt), &cts_req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**ch3|ctspkt");
    }
    if (cts_req != NULL)
    {
	/* FIXME: Ideally we could specify that a req not be returned.  
	   This would avoid our having to decrement the
	   reference count on a req we don't want/need. */
	MPIR_Request_free(cts_req);
    }

 fn_fail:    
    return mpi_errno;
}

/*
 * Define the routines that can print out the cancel packets if 
 * debugging is enabled.
 */
#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_RndvReqToSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... REQ_TO_SEND\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," context_id ... %d\n", pkt->rndv_req_to_send.match.parts.context_id);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," tag .......... %d\n", pkt->rndv_req_to_send.match.parts.tag);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," rank ......... %d\n", pkt->rndv_req_to_send.match.parts.rank);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz);
#ifdef MPID_USE_SEQUENCE_NUMBERS
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum);
#endif
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_RndvClrToSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... CLR_TO_SEND\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_RndvSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... RNDV_SEND\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
    return MPI_SUCCESS;
}
#endif

