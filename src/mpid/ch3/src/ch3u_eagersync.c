
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * Send a synchronous eager message.  This is an optimization that you
 * may want to use for programs that make extensive use of MPI_Ssend and
 * MPI_Issend for short messages.
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_EagerSyncNoncontigSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* MPIDI_CH3_EagerSyncNoncontigSend - Eagerly send noncontiguous data in
   synchronous mode.

   Some implementations may choose to use Rendezvous sends (see ch3u_rndv.c)
   for all Synchronous sends (MPI_Issend and MPI_Ssend).  An eager 
   synchronous send eliminates one of the handshake messages, but 
   most application codes should not be using synchronous sends in
   performance-critical operations.
*/
int MPIDI_CH3_EagerSyncNoncontigSend( MPIR_Request **sreq_p,
				      const void * buf, int count, 
				      MPI_Datatype datatype, intptr_t data_sz,
				      int dt_contig, MPI_Aint dt_true_lb,
				      int rank, 
				      int tag, MPIR_Comm * comm,
				      int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
    MPIDI_VC_t * vc;
    MPIR_Request *sreq = *sreq_p;

    /* MT FIXME what are the two operations we are waiting for?  the send and
     * the sync response? */
    MPIR_cc_set(&sreq->cc, 2);
    sreq->dev.OnDataAvail = 0;
    sreq->dev.OnFinal = 0;

    MPIDI_Pkt_init(es_pkt, MPIDI_CH3_PKT_EAGER_SYNC_SEND);
    es_pkt->match.parts.rank = comm->rank;
    es_pkt->match.parts.tag = tag;
    es_pkt->match.parts.context_id = comm->context_id + context_offset;
    es_pkt->sender_req_id = sreq->handle;
    es_pkt->data_sz = data_sz;

    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    
    MPL_DBG_MSGPKT(vc,tag,es_pkt->match.parts.context_id,rank,data_sz,"EagerSync");

    if (dt_contig)
    {
        MPL_IOV iov[2];
	MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
                                            "sending contiguous sync eager message, data_sz=%" PRIdPTR,
					    data_sz));
	
        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)es_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*es_pkt);
	iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *)buf + dt_true_lb);
	iov[1].MPL_IOV_LEN = data_sz;	
	
	MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
	mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, 2);
	MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
        /* Make sure to destroy the request before setting the pointer to
         * NULL, otherwise we lose the handle on the request */
            MPIR_Request_free(sreq);
	    *sreq_p = NULL;
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|eagermsg");
	}
	/* --END ERROR HANDLING-- */
    }
    else
    {
	MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,VERBOSE,
		       "sending non-contiguous sync eager message, data_sz=%" PRIdPTR,
		       data_sz);
	
	sreq->dev.segment_ptr = MPIR_Segment_alloc( );
        MPIR_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");

	MPIR_Segment_init(buf, count, datatype, sreq->dev.segment_ptr, 0);
	sreq->dev.segment_first = 0;
	sreq->dev.segment_size = data_sz;
	
	MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = vc->sendNoncontig_fn(vc, sreq, es_pkt, sizeof(MPIDI_CH3_Pkt_eager_sync_send_t));
	MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    *sreq_p = NULL;
    goto fn_exit;
}

/* Send a zero-sized message with eager synchronous.  This is a temporary
   routine, as we may want to replace this with a counterpart to the
   Eager Short message */
int MPIDI_CH3_EagerSyncZero(MPIR_Request **sreq_p, int rank, int tag,
			    MPIR_Comm * comm, int context_offset )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_send_t * const es_pkt = &upkt.eager_sync_send;
    MPIDI_VC_t * vc;
    MPIR_Request *sreq = *sreq_p;
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending zero length message");
    
    /* MT FIXME what are the two operations we are waiting for?  the send and
     * the sync response? */
    MPIR_cc_set(&sreq->cc, 2);
    MPIDI_Request_set_msg_type(sreq, MPIDI_REQUEST_EAGER_MSG);
    sreq->dev.OnDataAvail = 0;
    
    MPIDI_Pkt_init(es_pkt, MPIDI_CH3_PKT_EAGER_SYNC_SEND);
    es_pkt->match.parts.rank = comm->rank;
    es_pkt->match.parts.tag = tag;
    es_pkt->match.parts.context_id = comm->context_id + context_offset;
    es_pkt->sender_req_id = sreq->handle;
    es_pkt->data_sz = 0;
    
    MPIDI_Comm_get_vc_set_active(comm, rank, &vc);
    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(es_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);
    
    MPL_DBG_MSGPKT(vc,tag,es_pkt->match.parts.context_id,rank,(intptr_t)0,"EagerSync0");
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iSend(vc, sreq, es_pkt, sizeof(*es_pkt));
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIR_Request_free(sreq);
	*sreq_p = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|eagermsg");
    }
    /* --END ERROR HANDLING-- */

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* 
 * These routines are called when a receive matches an eager sync send 
 */
int MPIDI_CH3_EagerSyncAck( MPIDI_VC_t *vc, MPIR_Request *rreq )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
    MPIR_Request * esa_req;
    
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending eager sync ack");
    MPIDI_Pkt_init(esa_pkt, MPIDI_CH3_PKT_EAGER_SYNC_ACK);
    esa_pkt->sender_req_id = rreq->dev.sender_req_id;
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, esa_pkt, sizeof(*esa_pkt), &esa_req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno != MPI_SUCCESS) {
	MPIR_ERR_POP(mpi_errno);
    }
    if (esa_req != NULL)
    {
	MPIR_Request_free(esa_req);
    }
 fn_fail:
    return mpi_errno;
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
#define FUNCNAME MPIDI_CH3_PktHandler_EagerSyncSend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_EagerSyncSend( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data,
					intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_eager_send_t * es_pkt = &pkt->eager_send;
    MPIR_Request * rreq;
    int found;
    int complete;
    intptr_t data_len;
    int mpi_errno = MPI_SUCCESS;
    
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,VERBOSE,(MPL_DBG_FDEST,
     "received eager sync send pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d",
	      es_pkt->sender_req_id, es_pkt->match.parts.rank, 
	      es_pkt->match.parts.tag, 
              es_pkt->match.parts.context_id));
    MPL_DBG_MSGPKT(vc,es_pkt->match.parts.tag,es_pkt->match.parts.context_id,
		    es_pkt->match.parts.rank,es_pkt->data_sz,
		    "ReceivedEagerSync");
	    
    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&es_pkt->match, &found);
    MPIR_ERR_CHKANDJUMP1(!rreq, mpi_errno,MPI_ERR_OTHER, "**nomemreq", "**nomemuereq %d", MPIDI_CH3U_Recvq_count_unexp());

    /* If the completion counter is 0, that means that the communicator to
     * which this message is being sent has been revoked and we shouldn't
     * bother finishing this. */
    if (!found && MPIR_cc_get(rreq->cc) == 0) {
        *rreqp = NULL;
        goto fn_fail;
    }
    
    set_request_info(rreq, es_pkt, MPIDI_REQUEST_EAGER_MSG);

    data_len = ((*buflen >= rreq->dev.recv_data_sz)
                ? rreq->dev.recv_data_sz : *buflen);
    
    if (found)
    {
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_eager_sync_ack_t * const esa_pkt = &upkt.eager_sync_ack;
	MPIR_Request * esa_req;

	if (rreq->dev.recv_data_sz == 0) {
            *buflen = 0;
            mpi_errno = MPID_Request_complete(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
	    *rreqp = NULL;
	}
	else {
	    mpi_errno = MPIDI_CH3U_Receive_data_found( rreq, data,
                                                       &data_len, &complete );
	    if (mpi_errno != MPI_SUCCESS) {
		MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**ch3|postrecv",
		    "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGER_SYNC_SEND");
	    }

            *buflen = data_len;

            if (complete) 
            {
                mpi_errno = MPID_Request_complete(rreq);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
                *rreqp = NULL;
            }
            else
            {
                *rreqp = rreq;
            }
	}
	MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"sending eager sync ack");
	
	MPIDI_Pkt_init(esa_pkt, MPIDI_CH3_PKT_EAGER_SYNC_ACK);
	esa_pkt->sender_req_id = rreq->dev.sender_req_id;
	/* Because this is a packet handler, it is already within a CH3 CS */
	/* MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex); */
	mpi_errno = MPIDI_CH3_iStartMsg(vc, esa_pkt, sizeof(*esa_pkt), &esa_req);
	/* MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex); */
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|syncack");
	}
	if (esa_req != NULL) {
	    MPIR_Request_free(esa_req);
	}
    }
    else
    {
	if (rreq->dev.recv_data_sz == 0) {
            *buflen = 0;
            mpi_errno = MPID_Request_complete(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
	    *rreqp = NULL;
	}
	else {
	    mpi_errno = MPIDI_CH3U_Receive_data_unexpected( rreq, data,
                                                            &data_len, &complete );
	    if (mpi_errno != MPI_SUCCESS) {
		MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**ch3|postrecv",
		    "**ch3|postrecv %s", "MPIDI_CH3_PKT_EAGER_SYNC_SEND");
	    }

            *buflen = data_len;

            if (complete) 
            {
                mpi_errno = MPID_Request_complete(rreq);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
                *rreqp = NULL;
            }
            else
            {
                *rreqp = rreq;
            }
	}
	MPIDI_Request_set_sync_send_flag(rreq, TRUE);
    }
 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_EagerSyncAck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_EagerSyncAck( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, void *data ATTRIBUTE((unused)),
				       intptr_t *buflen, MPIR_Request **rreqp )
{
    MPIDI_CH3_Pkt_eager_sync_ack_t * esa_pkt = &pkt->eager_sync_ack;
    MPIR_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    
    MPL_DBG_MSG_P(MPIDI_CH3_DBG_OTHER,VERBOSE,
	   "received eager sync ack pkt, sreq=0x%08x", esa_pkt->sender_req_id);
	    
    MPIR_Request_get_ptr(esa_pkt->sender_req_id, sreq);
    /* decrement CC (but don't mark data transfer as complete since the 
       transfer could still be in progress) */

    /* FIXME: This sometimes segfaults */
    mpi_errno = MPID_Request_complete(sreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
    
    *buflen = 0;
    *rreqp = NULL;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_EagerSyncSend( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... EAGER_SYNC_SEND\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->eager_sync_send.sender_req_id));
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," context_id ... %d\n", pkt->eager_sync_send.match.parts.context_id);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," tag .......... %d\n", pkt->eager_sync_send.match.parts.tag);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," rank ......... %d\n", pkt->eager_sync_send.match.parts.rank);
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," data_sz ...... %d\n", pkt->eager_sync_send.data_sz);
#ifdef MPID_USE_SEQUENCE_NUMBERS
    MPL_DBG_MSG_D(MPIDI_CH3_DBG_OTHER,TERSE," seqnum ....... %d\n", pkt->eager_sync_send.seqnum);
#endif
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_EagerSyncAck( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,TERSE," type ......... EAGER_SYNC_ACK\n");
    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER,TERSE,(MPL_DBG_FDEST," sender_reqid . 0x%08X\n", pkt->eager_sync_ack.sender_req_id));
    return MPI_SUCCESS;
}
#endif
