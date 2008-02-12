/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

/*
 * This file contains the dispatch routine called by the ch3 progress 
 * engine to process messages.  
 *
 * This file is in transistion
 *
 * Where possible, the routines that create and send all packets of
 * a particular type are in the same file that contains the implementation 
 * of the handlers for that packet type (for example, the CancelSend 
 * packets are created and processed by routines in ch3/src/mpid_cancel_send.c)
 * This makes is easier to replace or modify functionality within 
 * the ch3 device.
 */

#define set_request_info(rreq_, pkt_, msg_type_)		\
{								\
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.rank;		\
    (rreq_)->status.MPI_TAG = (pkt_)->match.tag;		\
    (rreq_)->status.count = (pkt_)->data_sz;			\
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;		\
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;		\
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);		\
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));		\
}

#ifdef MPIDI_CH3_CHANNEL_RNDV
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_rndv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_rndv_pkt(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, 
				    MPID_Request ** rreqp, int *foundp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *rreq = NULL;
    MPIDI_CH3_Pkt_rndv_req_to_send_t * rts_pkt = &pkt->rndv_req_to_send;

    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&rts_pkt->match, foundp);
    if (rreq == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomemreq");
    }

    set_request_info(rreq, rts_pkt, MPIDI_REQUEST_RNDV_MSG);

    if (!*foundp)
    {
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"unexpected request allocated");

	/*
	 * An MPID_Probe() may be waiting for the request we just inserted, 
	 * so we need to tell the progress engine to exit.
	 *
	 * FIXME: This will cause MPID_Progress_wait() to return to the MPI 
	 * layer each time an unexpected RTS packet is
	 * received.  MPID_Probe() should atomically increment a counter and 
	 * MPIDI_CH3_Progress_signal_completion()
	 * should only be called if that counter is greater than zero.
	 */
	MPIDI_CH3_Progress_signal_completion();
    }

    /* return the request */
    *rreqp = rreq;

 fn_fail:
    return mpi_errno;
}
#endif

/*
 * MPIDI_CH3U_Handle_recv_pkt()
 *
 * NOTE: Multiple threads may NOT simultaneously call this routine with the 
 * same VC.  This constraint eliminates the need to
 * lock the VC.  If simultaneous upcalls are a possible, the calling routine 
 * for serializing the calls.
 */

/* This code and definition is used to allow us to provide a routine
   that handles packets that are received out-of-order.  However, we 
   currently do not support that in the CH3 device. */
#define MPIDI_CH3U_Handle_ordered_recv_pkt MPIDI_CH3U_Handle_recv_pkt 

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_ordered_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_ordered_recv_pkt(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt, 
				       MPIDI_msg_sz_t *buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    static MPIDI_CH3_PktHandler_Fcn *pktArray[MPIDI_CH3_PKT_END_CH3+1];
    static int needsInit = 1;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);

    MPIU_DBG_STMT(CH3_OTHER,VERBOSE,MPIDI_DBG_Print_packet(pkt));

    /* FIXME: We can turn this into something like

       MPIU_Assert(pkt->type >= 0 && pkt->type <= MAX_PACKET_TYPE);
       mpi_errno = MPIDI_CH3_ProgressFunctions[pkt->type](vc,pkt,rreqp);
       
       in the progress engine itself.  Then this routine is not necessary.
    */

    if (needsInit) {
	MPIDI_CH3_PktHandler_Init( pktArray, MPIDI_CH3_PKT_END_CH3 );
	needsInit = 0;
    }
    MPIU_Assert(pkt->type  >= 0 && pkt->type <= MPIDI_CH3_PKT_END_CH3);
    mpi_errno = pktArray[pkt->type](vc, pkt, buflen, rreqp);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT);
    return mpi_errno;
}

/* 
 * This function is used to receive data from the receive buffer to
 * the user buffer.  If all data for this message has not been
 * received, the request is set up to receive the next data to arrive.
 * In turn, this request is attached to a virtual connection.
 *
 * buflen is an I/O parameter.  The length of the received data is
 * passed in.  The function returns the number of bytes actually
 * processed by this function.
 *
 * complete is an OUTPUT variable.  It is set to TRUE iff all of the
 * data for the request has been received.  This function does not
 * actually complete the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Receive_data_found
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Receive_data_found(MPID_Request *rreq, char *buf, MPIDI_msg_sz_t *buflen, int *complete)
{
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype * dt_ptr = NULL;
    MPIDI_msg_sz_t data_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECEIVE_DATA_FOUND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECEIVE_DATA_FOUND);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"posted request found");
	
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, 
			    dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
		
    if (rreq->dev.recv_data_sz <= userbuf_sz) {
	data_sz = rreq->dev.recv_data_sz;
    }
    else {
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
               "receive buffer too small; message truncated, msg_sz=" MPIDI_MSG_SZ_FMT ", userbuf_sz="
					    MPIDI_MSG_SZ_FMT,
				 rreq->dev.recv_data_sz, userbuf_sz));
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
                     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
		     "**truncate", "**truncate %d %d %d %d", 
		     rreq->status.MPI_SOURCE, rreq->status.MPI_TAG, 
		     rreq->dev.recv_data_sz, userbuf_sz );
	rreq->status.count = userbuf_sz;
	data_sz = userbuf_sz;
    }

    if (dt_contig && data_sz == rreq->dev.recv_data_sz)
    {
	/* user buffer is contiguous and large enough to store the
	   entire message.  However, we haven't yet *read* the data 
	   (this code describes how to read the data into the destination) */

        /* if all of the data has already been received, unpack it
           now, otherwise build an iov and let the channel unpack */
        if (*buflen >= data_sz)
        {
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"Copying contiguous data to user buffer");
            /* copy data out of the receive buffer */
            memcpy((char*)(rreq->dev.user_buf) + dt_true_lb, buf, data_sz);
            *buflen = data_sz;
            *complete = TRUE;
        }
        else
        {
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for contiguous read");
            
            rreq->dev.iov[0].MPID_IOV_BUF = 
                (MPID_IOV_BUF_CAST)((char*)(rreq->dev.user_buf) + dt_true_lb);
            rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
            rreq->dev.iov_count = 1;
            *buflen = 0;
            *complete = FALSE;
        }
        
	/* FIXME: We want to set the OnDataAvail to the appropriate 
	   function, which depends on whether this is an RMA 
	   request or a pt-to-pt request. */
	rreq->dev.OnDataAvail = 0;
    }
    else {
	/* user buffer is not contiguous or is too small to hold
	   the entire message */
        
	rreq->dev.segment_ptr = MPID_Segment_alloc( );
	/* if (!rreq->dev.segment_ptr) { MPIU_ERR_POP(); } */
 	MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, 
			  rreq->dev.datatype, rreq->dev.segment_ptr, 0);
	rreq->dev.segment_first = 0;
	rreq->dev.segment_size  = data_sz;

        /* if all of the data has already been received, and the
           message is not truncated, unpack it now, otherwise build an
           iov and let the channel unpack */
        if (data_sz == rreq->dev.recv_data_sz && *buflen >= data_sz)
        {
            MPIDI_msg_sz_t last;
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"Copying noncontiguous data to user buffer");
            last = data_sz;
            MPID_Segment_unpack(rreq->dev.segment_ptr, rreq->dev.segment_first, 
				&last, buf);
            /* --BEGIN ERROR HANDLING-- */
            if (last != data_sz)
            {
                /* If the data can't be unpacked, the we have a
                   mismatch between the datatype and the amount of
                   data received.  Throw away received data. */
                MPIU_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
                rreq->status.count = (int)rreq->dev.segment_first;
                *buflen = data_sz;
                *complete = TRUE;
		/* FIXME: Set OnDataAvail to 0?  If not, why not? */
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            *buflen = data_sz;
            rreq->dev.OnDataAvail = 0;
            *complete = TRUE;
        }
        else
        {   
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for non-contiguous read");

            mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,
                                         "**ch3|loadrecviov");
            }
            *buflen = 0;
            *complete = FALSE;
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECEIVE_DATA_FOUND);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Receive_data_unexpected
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Receive_data_unexpected(MPID_Request * rreq, char *buf, MPIDI_msg_sz_t *buflen, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_RECEIVE_DATA_UNEXPECTED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_RECEIVE_DATA_UNEXPECTED);

    /* FIXME: to improve performance, allocate temporary buffer from a 
       specialized buffer pool. */
    /* FIXME: to avoid memory exhaustion, integrate buffer pool management
       with flow control */
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"unexpected request allocated");
    
    rreq->dev.tmpbuf = MPIU_Malloc(rreq->dev.recv_data_sz);
    if (!rreq->dev.tmpbuf) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
    }
    rreq->dev.tmpbuf_sz = rreq->dev.recv_data_sz;
    
    /* if all of the data has already been received, copy it
       now, otherwise build an iov and let the channel copy it */
    if (rreq->dev.recv_data_sz <= *buflen)
    {
        memcpy(rreq->dev.tmpbuf, buf, rreq->dev.recv_data_sz);
        *buflen = rreq->dev.recv_data_sz;
        rreq->dev.recv_pending_count = 1;
        *complete = TRUE;
    }
    else
    {
        rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *)rreq->dev.tmpbuf);
        rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.recv_data_sz;
        rreq->dev.iov_count = 1;
        rreq->dev.recv_pending_count = 2;
        *buflen = 0;
        *complete = FALSE;
    }

    rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_UnpackUEBufComplete;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_RECEIVE_DATA_UNEXPECTED);
    return mpi_errno;
}

/* 
 * This function is used to post a receive operation on a request for the 
 * next data to arrive.  In turn, this request is attached to a virtual
 * connection.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Post_data_receive_found
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Post_data_receive_found(MPID_Request * rreq)
{
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype * dt_ptr = NULL;
    MPIDI_msg_sz_t data_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_POST_DATA_RECEIVE_FOUND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_POST_DATA_RECEIVE_FOUND);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"posted request found");
	
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, 
			    dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
		
    if (rreq->dev.recv_data_sz <= userbuf_sz) {
	data_sz = rreq->dev.recv_data_sz;
    }
    else {
	MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST,
               "receive buffer too small; message truncated, msg_sz=" MPIDI_MSG_SZ_FMT ", userbuf_sz="
					    MPIDI_MSG_SZ_FMT,
				 rreq->dev.recv_data_sz, userbuf_sz));
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, 
                     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
		     "**truncate", "**truncate %d %d %d %d", 
		     rreq->status.MPI_SOURCE, rreq->status.MPI_TAG, 
		     rreq->dev.recv_data_sz, userbuf_sz );
	rreq->status.count = userbuf_sz;
	data_sz = userbuf_sz;
    }

    if (dt_contig && data_sz == rreq->dev.recv_data_sz)
    {
	/* user buffer is contiguous and large enough to store the
	   entire message.  However, we haven't yet *read* the data 
	   (this code describes how to read the data into the destination) */
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for contiguous read");
	rreq->dev.iov[0].MPID_IOV_BUF = 
	    (MPID_IOV_BUF_CAST)((char*)(rreq->dev.user_buf) + dt_true_lb);
	rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	rreq->dev.iov_count = 1;
	/* FIXME: We want to set the OnDataAvail to the appropriate 
	   function, which depends on whether this is an RMA 
	   request or a pt-to-pt request. */
	rreq->dev.OnDataAvail = 0;
    }
    else {
	/* user buffer is not contiguous or is too small to hold
	   the entire message */
	int mpi_errno;
	
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for non-contiguous read");
	rreq->dev.segment_ptr = MPID_Segment_alloc( );
	/* if (!rreq->dev.segment_ptr) { MPIU_ERR_POP(); } */
	MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, 
			  rreq->dev.datatype, rreq->dev.segment_ptr, 0);
	rreq->dev.segment_first = 0;
	rreq->dev.segment_size = data_sz;
	mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,
				     "**ch3|loadrecviov");
	}
    }

fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_POST_DATA_RECEIVE_FOUND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Post_data_receive_unexpected
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Post_data_receive_unexpected(MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_POST_DATA_RECEIVE_UNEXPECTED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_POST_DATA_RECEIVE_UNEXPECTED);

    /* FIXME: to improve performance, allocate temporary buffer from a 
       specialized buffer pool. */
    /* FIXME: to avoid memory exhaustion, integrate buffer pool management
       with flow control */
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"unexpected request allocated");
    
    rreq->dev.tmpbuf = MPIU_Malloc(rreq->dev.recv_data_sz);
    if (!rreq->dev.tmpbuf) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
    }
    rreq->dev.tmpbuf_sz = rreq->dev.recv_data_sz;
    
    rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rreq->dev.tmpbuf;
    rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.recv_data_sz;
    rreq->dev.iov_count = 1;
    rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_UnpackUEBufComplete;
    rreq->dev.recv_pending_count = 2;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_POST_DATA_RECEIVE_UNEXPECTED);
    return mpi_errno;
}


/* Check if requested lock can be granted. If it can, set 
   win_ptr->current_lock_type to the new lock type and return 1. Else return 0.

   FIXME: MT: This function must be atomic because two threads could be trying 
   to do the same thing, e.g., the main thread in MPI_Win_lock(source=target) 
   and another thread in the progress engine.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Try_acquire_win_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Try_acquire_win_lock(MPID_Win *win_ptr, int requested_lock)
{
    int existing_lock;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_TRY_ACQUIRE_WIN_LOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_TRY_ACQUIRE_WIN_LOCK);

    existing_lock = win_ptr->current_lock_type;

    /* Locking Rules:
       
    Requested          Existing             Action
    --------           --------             ------
    Shared             Exclusive            Queue it
    Shared             NoLock/Shared        Grant it
    Exclusive          NoLock               Grant it
    Exclusive          Exclusive/Shared     Queue it
    */

    if ( ( (requested_lock == MPI_LOCK_SHARED) && 
           ((existing_lock == MPID_LOCK_NONE) ||
            (existing_lock == MPI_LOCK_SHARED) ) )
         || 
         ( (requested_lock == MPI_LOCK_EXCLUSIVE) &&
           (existing_lock == MPID_LOCK_NONE) ) ) {

        /* grant lock.  set new lock type on window */
        win_ptr->current_lock_type = requested_lock;

        /* if shared lock, incr. ref. count */
        if (requested_lock == MPI_LOCK_SHARED)
            win_ptr->shared_lock_ref_cnt++;

	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_TRY_ACQUIRE_WIN_LOCK);
        return 1;
    }
    else {
        /* do not grant lock */
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_TRY_ACQUIRE_WIN_LOCK);
        return 0;
    }
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_granted_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Send_lock_granted_pkt(MPIDI_VC_t *vc, MPI_Win source_win_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_granted_t *lock_granted_pkt = &upkt.lock_granted;
    MPID_Request *req = NULL;
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);

    /* send lock granted packet */
    MPIDI_Pkt_init(lock_granted_pkt, MPIDI_CH3_PKT_LOCK_GRANTED);
    lock_granted_pkt->source_win_handle = source_win_handle;
        
    mpi_errno = MPIU_CALL(MPIDI_CH3,iStartMsg(vc, lock_granted_pkt,
				      sizeof(*lock_granted_pkt), &req));
    if (mpi_errno) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
    }

    if (req != NULL)
    {
        MPID_Request_release(req);
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);

    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/* Here are the functions that implement the packet actions.  They'll be moved
 * to more modular places where it will be easier to replace subsets of the
 * in order to experiement with alternative data transfer methods, such as
 * sending some data with a rendezvous request or including data within
 * an eager message.                                                        
 *
 * The convention for the names of routines that handle packets is
 *   MPIDI_CH3_PktHandler_<type>( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt )
 * as in
 *   MPIDI_CH3_PktHandler_EagerSend
 *
 * Each packet type also has a routine that understands how to print that
 * packet type, this routine is
 *   MPIDI_CH3_PktPrint_<type>( FILE *, MPIDI_CH3_Pkt_t * )
 *                                                                          */
/* ------------------------------------------------------------------------ */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Put( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
			      MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_put_t * put_pkt = &pkt->put;
    MPID_Request *req = NULL;
    int predefined;
    int type_size;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received put pkt");

    if (put_pkt->count == 0)
    {
	MPID_Win *win_ptr;
	
	/* it's a 0-byte message sent just to decrement the
	   completion counter. This happens only in
	   post/start/complete/wait sync model; therefore, no need
	   to check lock queue. */
	if (put_pkt->target_win_handle != MPI_WIN_NULL) {
	    MPID_Win_get_ptr(put_pkt->target_win_handle, win_ptr);
	    /* FIXME: MT: this has to be done atomically */
	    win_ptr->my_counter -= 1;
	}
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
	MPIDI_CH3_Progress_signal_completion();	
	*rreqp = NULL;
        goto fn_exit;
    }
        
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
                
    req->dev.user_buf = put_pkt->addr;
    req->dev.user_count = put_pkt->count;
    req->dev.target_win_handle = put_pkt->target_win_handle;
    req->dev.source_win_handle = put_pkt->source_win_handle;
	
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(put_pkt->datatype, predefined);
    if (predefined)
    {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
        req->dev.datatype = put_pkt->datatype;
	    
        MPID_Datatype_get_size_macro(put_pkt->datatype,
                                     type_size);
        req->dev.recv_data_sz = type_size * put_pkt->count;
		    
        if (req->dev.recv_data_sz == 0) {
            MPIDI_CH3U_Request_complete( req );
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            *rreqp = NULL;
            goto fn_exit;
        }

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                                  &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
        /* FIXME:  Only change the handling of completion if
           post_data_receive reset the handler.  There should
           be a cleaner way to do this */
        if (!req->dev.OnDataAvail) {
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutAccumRespComplete;
        }
        
        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t) + data_len;

        if (complete) 
        {
            mpi_errno = MPIDI_CH3_ReqHandler_PutAccumRespComplete(vc, req, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
            if (complete)
            {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
    }
    else
    {
        /* derived datatype */
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT);
        req->dev.datatype = MPI_DATATYPE_NULL;
	    
        req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
            MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
        if (! req->dev.dtype_info) {
            MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
        }

        req->dev.dataloop = MPIU_Malloc(put_pkt->dataloop_size);
        if (! req->dev.dataloop) {
            MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
        }

        /* if we received all of the dtype_info and dataloop, copy it
           now and call the handler, otherwise set the iov and let the
           channel copy it */
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + put_pkt->dataloop_size)
        {
            /* copy all of dtype_info and dataloop */
            memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info), put_pkt->dataloop_size);

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + put_pkt->dataloop_size;
          
            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_PutRespDerivedDTComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT"); 
            if (complete)
            {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
        else
        {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *)req->dev.dtype_info);
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = put_pkt->dataloop_size;
            req->dev.iov_count = 2;

            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutRespDerivedDTComplete;
        }
        
    }
	
    *rreqp = req;

    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER,"**ch3|postrecv",
                      "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
    }
    

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Get( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
			      MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_get_t * get_pkt = &pkt->get;
    MPID_Request *req = NULL;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int predefined;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    int type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received get pkt");
    
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    req = MPID_Request_create();
    req->dev.target_win_handle = get_pkt->target_win_handle;
    req->dev.source_win_handle = get_pkt->source_win_handle;
    
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(get_pkt->datatype, predefined);
    if (predefined)
    {
	/* basic datatype. send the data. */
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
	
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP); 
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->dev.OnFinal     = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->kind = MPID_REQUEST_SEND;
	
	MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
	get_resp_pkt->request_handle = get_pkt->request_handle;
	
	iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
	
	iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)get_pkt->addr;
	MPID_Datatype_get_size_macro(get_pkt->datatype, type_size);
	iov[1].MPID_IOV_LEN = get_pkt->count * type_size;
	
	mpi_errno = MPIU_CALL(MPIDI_CH3,iSendv(vc, req, iov, 2));
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(req, 0);
	    MPIDI_CH3_Request_destroy(req);
	    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
	}
	/* --END ERROR HANDLING-- */
	
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
	*rreqp = NULL;
    }
    else
    {
	/* derived datatype. first get the dtype_info and dataloop. */
	
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetRespDerivedDTComplete;
	req->dev.OnFinal     = 0;
	req->dev.user_buf = get_pkt->addr;
	req->dev.user_count = get_pkt->count;
	req->dev.datatype = MPI_DATATYPE_NULL;
	req->dev.request_handle = get_pkt->request_handle;
	
	req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
	    MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
	if (! req->dev.dtype_info) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	
	req->dev.dataloop = MPIU_Malloc(get_pkt->dataloop_size);
	if (! req->dev.dataloop) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	
        /* if we received all of the dtype_info and dataloop, copy it
           now and call the handler, otherwise set the iov and let the
           channel copy it */
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + get_pkt->dataloop_size)
        {
            /* copy all of dtype_info and dataloop */
            memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info), get_pkt->dataloop_size);

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + get_pkt->dataloop_size;
          
            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_GetRespDerivedDTComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET"); 
            if (complete)
                *rreqp = NULL;
        }
        else
        {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dtype_info;
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = get_pkt->dataloop_size;
            req->dev.iov_count = 2;
	
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            *rreqp = req;
        }
        
    }
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);
    return mpi_errno;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_GetResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_GetResp( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				  MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &pkt->get_resp;
    MPID_Request *req;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    int type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received get response pkt");

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    MPID_Request_get_ptr(get_resp_pkt->request_handle, req);
    
    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;
    
    /* FIXME: It is likely that this cannot happen (never perform
       a get with a 0-sized item).  In that case, change this
       to an MPIU_Assert (and do the same for accumulate and put) */
    if (req->dev.recv_data_sz == 0) {
	MPIDI_CH3U_Request_complete( req );
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
	*rreqp = NULL;
    }
    else {
	*rreqp = req;
        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf,
                                                  &data_len, &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv", "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_RESP");
        if (complete) 
        {
            MPIDI_CH3U_Request_complete(req);
            *rreqp = NULL;
        }
        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Accumulate( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				     MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_accum_t * accum_pkt = &pkt->accum;
    MPID_Request *req;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    int predefined;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    int type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received accumulate pkt");
    
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    *rreqp = req;
    
    req->dev.user_count = accum_pkt->count;
    req->dev.op = accum_pkt->op;
    req->dev.real_user_buf = accum_pkt->addr;
    req->dev.target_win_handle = accum_pkt->target_win_handle;
    req->dev.source_win_handle = accum_pkt->source_win_handle;

    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(accum_pkt->datatype, predefined);
    if (predefined)
    {
	MPIU_THREADPRIV_DECL;
	MPIU_THREADPRIV_GET;
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP);
	req->dev.datatype = accum_pkt->datatype;

	MPIR_Nest_incr();
	mpi_errno = NMPI_Type_get_true_extent(accum_pkt->datatype, 
					      &true_lb, &true_extent);
	MPIR_Nest_decr();
	if (mpi_errno) {
	    MPIU_ERR_POP(mpi_errno);
	}

	MPID_Datatype_get_extent_macro(accum_pkt->datatype, extent); 
	tmp_buf = MPIU_Malloc(accum_pkt->count * 
			      (MPIR_MAX(extent,true_extent)));
	if (!tmp_buf) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
	
	/* adjust for potential negative lower bound in datatype */
	tmp_buf = (void *)((char*)tmp_buf - true_lb);
	
	req->dev.user_buf = tmp_buf;
	
	MPID_Datatype_get_size_macro(accum_pkt->datatype, type_size);
	req->dev.recv_data_sz = type_size * accum_pkt->count;
              
	if (req->dev.recv_data_sz == 0) {
	    MPIDI_CH3U_Request_complete(req);
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
	    *rreqp = NULL;
	}
	else {
            mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                                      &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
	    /* FIXME:  Only change the handling of completion if
	       post_data_receive reset the handler.  There should
	       be a cleaner way to do this */
	    if (!req->dev.OnDataAvail) {
		req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutAccumRespComplete;
	    }
            /* return the number of bytes processed in this function */
            *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
            
            if (complete) 
            {
                mpi_errno = MPIDI_CH3_ReqHandler_PutAccumRespComplete(vc, req, &complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                if (complete)
                {
                    *rreqp = NULL;
                    goto fn_exit;
                }
            }
	}
    }
    else
    {
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumRespDerivedDTComplete;
	req->dev.datatype = MPI_DATATYPE_NULL;
                
	req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
	    MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
	if (! req->dev.dtype_info) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	
	req->dev.dataloop = MPIU_Malloc(accum_pkt->dataloop_size);
	if (! req->dev.dataloop) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + accum_pkt->dataloop_size)
        {
            /* copy all of dtype_info and dataloop */
            memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info), accum_pkt->dataloop_size);

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + accum_pkt->dataloop_size;
          
            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_AccumRespDerivedDTComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_ACCUMULATE"); 
            if (complete)
            {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
        else
        {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dtype_info;
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = accum_pkt->dataloop_size;
            req->dev.iov_count = 2;
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
        }
        
    }

    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**ch3|postrecv",
			     "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Lock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
			       MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_t * lock_pkt = &pkt->lock;
    MPID_Win *win_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock pkt");
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_pkt->target_win_handle, win_ptr);
    
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, 
					lock_pkt->lock_type) == 1)
    {
	/* send lock granted packet. */
	mpi_errno = MPIDI_CH3I_Send_lock_granted_pkt(vc,
					     lock_pkt->source_win_handle);
    }

    else {
	/* queue the lock information */
	MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;
	
	/* FIXME: MT: This may need to be done atomically. */
	
	curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	prev_ptr = curr_ptr;
	while (curr_ptr != NULL)
	{
	    prev_ptr = curr_ptr;
	    curr_ptr = curr_ptr->next;
	}
	
	new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
	if (!new_ptr) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	if (prev_ptr != NULL)
	    prev_ptr->next = new_ptr;
	else 
	    win_ptr->lock_queue = new_ptr;
        
	new_ptr->next = NULL;  
	new_ptr->lock_type = lock_pkt->lock_type;
	new_ptr->source_win_handle = lock_pkt->source_win_handle;
	new_ptr->vc = vc;
	new_ptr->pt_single_op = NULL;
    }
    
    *rreqp = NULL;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockGranted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockGranted( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				      MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_granted_t * lock_granted_pkt = &pkt->lock_granted;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock granted pkt");
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_granted_pkt->source_win_handle, win_ptr);
    /* set the lock_granted flag in the window */
    win_ptr->lock_granted = 1;
    
    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();	

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_PtRMADone
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_PtRMADone( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
				    MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_pt_rma_done_t * pt_rma_done_pkt = &pkt->pt_rma_done;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_PTRMADONE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_PTRMADONE);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received shared lock ops done pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(pt_rma_done_pkt->source_win_handle, win_ptr);
    /* reset the lock_granted flag in the window */
    win_ptr->lock_granted = 0;

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();	

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_PTRMADONE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockPutUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockPutUnlock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
					MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_put_unlock_t * lock_put_unlock_pkt = 
	&pkt->lock_put_unlock;
    MPID_Win *win_ptr = NULL;
    MPID_Request *req = NULL;
    int type_size;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock_put_unlock pkt");
    
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    
    req->dev.datatype = lock_put_unlock_pkt->datatype;
    MPID_Datatype_get_size_macro(lock_put_unlock_pkt->datatype, type_size);
    req->dev.recv_data_sz = type_size * lock_put_unlock_pkt->count;
    req->dev.user_count = lock_put_unlock_pkt->count;
    req->dev.target_win_handle = lock_put_unlock_pkt->target_win_handle;
    
    MPID_Win_get_ptr(lock_put_unlock_pkt->target_win_handle, win_ptr);
    
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, 
                                        lock_put_unlock_pkt->lock_type) == 1)
    {
	/* do the put. for this optimization, only basic datatypes supported. */
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutAccumRespComplete;
	req->dev.user_buf = lock_put_unlock_pkt->addr;
	req->dev.source_win_handle = lock_put_unlock_pkt->source_win_handle;
	req->dev.single_op_opt = 1;
    }
    
    else {
	/* queue the information */
	MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;
	
	new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
	if (!new_ptr) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	
	new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
	if (new_ptr->pt_single_op == NULL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	
	/* FIXME: MT: The queuing may need to be done atomically. */

	curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	prev_ptr = curr_ptr;
	while (curr_ptr != NULL)
	{
	    prev_ptr = curr_ptr;
	    curr_ptr = curr_ptr->next;
	}
	
	if (prev_ptr != NULL)
	    prev_ptr->next = new_ptr;
	else 
	    win_ptr->lock_queue = new_ptr;
        
	new_ptr->next = NULL;  
	new_ptr->lock_type = lock_put_unlock_pkt->lock_type;
	new_ptr->source_win_handle = lock_put_unlock_pkt->source_win_handle;
	new_ptr->vc = vc;
	
	new_ptr->pt_single_op->type = MPIDI_RMA_PUT;
	new_ptr->pt_single_op->addr = lock_put_unlock_pkt->addr;
	new_ptr->pt_single_op->count = lock_put_unlock_pkt->count;
	new_ptr->pt_single_op->datatype = lock_put_unlock_pkt->datatype;
	/* allocate memory to receive the data */
	new_ptr->pt_single_op->data = MPIU_Malloc(req->dev.recv_data_sz);
	if (new_ptr->pt_single_op->data == NULL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}

	new_ptr->pt_single_op->data_recd = 0;

	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PT_SINGLE_PUT);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SinglePutAccumComplete;
	req->dev.user_buf = new_ptr->pt_single_op->data;
	req->dev.lock_queue_entry = new_ptr;
    }
    
    if (req->dev.recv_data_sz == 0) {
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
	MPIDI_CH3U_Request_complete(req);
	*rreqp = NULL;
    }
    else {
	int (*fcn)( MPIDI_VC_t *, struct MPID_Request *, int * );
	fcn = req->dev.OnDataAvail;
        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                                  &complete);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_SETFATALANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
                                      "**ch3|postrecv", "**ch3|postrecv %s", 
                                      "MPIDI_CH3_PKT_LOCK_PUT_UNLOCK");
        }
	req->dev.OnDataAvail = fcn; 
	*rreqp = req;

        if (complete) 
        {
            mpi_errno = fcn(vc, req, &complete);
            if (complete)
            {
                *rreqp = NULL;
            }
        }
        
         /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
   }
    
    
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETFATALANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
				  "**ch3|postrecv", "**ch3|postrecv %s", 
				  "MPIDI_CH3_PKT_LOCK_PUT_UNLOCK");
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockGetUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockGetUnlock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_get_unlock_t * lock_get_unlock_pkt = 
	&pkt->lock_get_unlock;
    MPID_Win *win_ptr = NULL;
    int type_size;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock_get_unlock pkt");
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_get_unlock_pkt->target_win_handle, win_ptr);
    
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, 
                                        lock_get_unlock_pkt->lock_type) == 1)
    {
	/* do the get. for this optimization, only basic datatypes supported. */
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
	MPID_Request *req;
	MPID_IOV iov[MPID_IOV_LIMIT];
	
	req = MPID_Request_create();
	req->dev.target_win_handle = lock_get_unlock_pkt->target_win_handle;
	req->dev.source_win_handle = lock_get_unlock_pkt->source_win_handle;
	req->dev.single_op_opt = 1;
	
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP); 
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->dev.OnFinal     = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->kind = MPID_REQUEST_SEND;
	
	MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
	get_resp_pkt->request_handle = lock_get_unlock_pkt->request_handle;
	
	iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
	
	iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)lock_get_unlock_pkt->addr;
	MPID_Datatype_get_size_macro(lock_get_unlock_pkt->datatype, type_size);
	iov[1].MPID_IOV_LEN = lock_get_unlock_pkt->count * type_size;
	
	mpi_errno = MPIU_CALL(MPIDI_CH3,iSendv(vc, req, iov, 2));
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(req, 0);
	    MPIDI_CH3_Request_destroy(req);
	    MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
	}
	/* --END ERROR HANDLING-- */
    }

    else {
	/* queue the information */
	MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;
	
	/* FIXME: MT: This may need to be done atomically. */
	
	curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	prev_ptr = curr_ptr;
	while (curr_ptr != NULL)
	{
	    prev_ptr = curr_ptr;
	    curr_ptr = curr_ptr->next;
	}
	
	new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
	if (!new_ptr) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
	if (new_ptr->pt_single_op == NULL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
	}
	
	if (prev_ptr != NULL)
	    prev_ptr->next = new_ptr;
	else 
	    win_ptr->lock_queue = new_ptr;
        
	new_ptr->next = NULL;  
	new_ptr->lock_type = lock_get_unlock_pkt->lock_type;
	new_ptr->source_win_handle = lock_get_unlock_pkt->source_win_handle;
	new_ptr->vc = vc;
	
	new_ptr->pt_single_op->type = MPIDI_RMA_GET;
	new_ptr->pt_single_op->addr = lock_get_unlock_pkt->addr;
	new_ptr->pt_single_op->count = lock_get_unlock_pkt->count;
	new_ptr->pt_single_op->datatype = lock_get_unlock_pkt->datatype;
	new_ptr->pt_single_op->data = NULL;
	new_ptr->pt_single_op->request_handle = lock_get_unlock_pkt->request_handle;
	new_ptr->pt_single_op->data_recd = 1;
    }
    
    *rreqp = NULL;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockAccumUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockAccumUnlock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					  MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_accum_unlock_t * lock_accum_unlock_pkt = 
	&pkt->lock_accum_unlock;
    MPID_Request *req = NULL;
    MPID_Win *win_ptr = NULL;
    MPIDI_Win_lock_queue *curr_ptr = NULL, *prev_ptr = NULL, *new_ptr = NULL;
    int type_size;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock_accum_unlock pkt");
    
    /* no need to acquire the lock here because we need to receive the 
       data into a temporary buffer first */
    
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    
    req->dev.datatype = lock_accum_unlock_pkt->datatype;
    MPID_Datatype_get_size_macro(lock_accum_unlock_pkt->datatype, type_size);
    req->dev.recv_data_sz = type_size * lock_accum_unlock_pkt->count;
    req->dev.user_count = lock_accum_unlock_pkt->count;
    req->dev.target_win_handle = lock_accum_unlock_pkt->target_win_handle;
    
    /* queue the information */
    
    new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
    if (!new_ptr) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
    }
    
    new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
    if (new_ptr->pt_single_op == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
    }
    
    MPID_Win_get_ptr(lock_accum_unlock_pkt->target_win_handle, win_ptr);
    
    /* FIXME: MT: The queuing may need to be done atomically. */
    
    curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
    prev_ptr = curr_ptr;
    while (curr_ptr != NULL)
    {
	prev_ptr = curr_ptr;
	curr_ptr = curr_ptr->next;
    }
    
    if (prev_ptr != NULL)
	prev_ptr->next = new_ptr;
    else 
	win_ptr->lock_queue = new_ptr;
    
    new_ptr->next = NULL;  
    new_ptr->lock_type = lock_accum_unlock_pkt->lock_type;
    new_ptr->source_win_handle = lock_accum_unlock_pkt->source_win_handle;
    new_ptr->vc = vc;
    
    new_ptr->pt_single_op->type = MPIDI_RMA_ACCUMULATE;
    new_ptr->pt_single_op->addr = lock_accum_unlock_pkt->addr;
    new_ptr->pt_single_op->count = lock_accum_unlock_pkt->count;
    new_ptr->pt_single_op->datatype = lock_accum_unlock_pkt->datatype;
    new_ptr->pt_single_op->op = lock_accum_unlock_pkt->op;
    /* allocate memory to receive the data */
    new_ptr->pt_single_op->data = MPIU_Malloc(req->dev.recv_data_sz);
    if (new_ptr->pt_single_op->data == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem" );
    }
    
    new_ptr->pt_single_op->data_recd = 0;
    
    MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PT_SINGLE_ACCUM);
    req->dev.user_buf = new_ptr->pt_single_op->data;
    req->dev.lock_queue_entry = new_ptr;
    
    *rreqp = req;
    if (req->dev.recv_data_sz == 0) {
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
	MPIDI_CH3U_Request_complete(req);
	*rreqp = NULL;
    }
    else {
        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                                  &complete);
	/* FIXME:  Only change the handling of completion if
	   post_data_receive reset the handler.  There should
	   be a cleaner way to do this */
	if (!req->dev.OnDataAvail) {
	    req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SinglePutAccumComplete;
	}
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER,"**ch3|postrecv", 
		  "**ch3|postrecv %s", "MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK");
	}
        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

        if (complete) 
        {
            mpi_errno = MPIDI_CH3_ReqHandler_SinglePutAccumComplete(vc, req, &complete);
            if (complete)
            {
                *rreqp = NULL;
            }
        }
    }
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);
    return mpi_errno;
}

/* FIXME: we still need to implement flow control */
int MPIDI_CH3_PktHandler_FlowCntlUpdate( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					 MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_EndCH3
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_EndCH3( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				 MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ENDCH3);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ENDCH3);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_ENDCH3);

    return MPI_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* This routine may be called within a channel to initialize an 
   array of packet handler functions, indexed by packet type.

   This function initializes an array so that the array may be private 
   to the file that contains the progress function, if this is 
   appropriate (this allows the compiler to reduce the cost in 
   accessing the elements of the array in some cases).
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Init( MPIDI_CH3_PktHandler_Fcn *pktArray[], 
			       int arraySize  )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_INIT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_INIT);

    /* Check that the array is large enough */
    if (arraySize < MPIDI_CH3_PKT_END_CH3) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN,
				 "**ch3|pktarraytoosmall");
    }
    pktArray[MPIDI_CH3_PKT_EAGER_SEND] = 
	MPIDI_CH3_PktHandler_EagerSend;
#ifdef USE_EAGER_SHORT
    pktArray[MPIDI_CH3_PKT_EAGERSHORT_SEND] = 
	MPIDI_CH3_PktHandler_EagerShortSend;
#endif
    pktArray[MPIDI_CH3_PKT_READY_SEND] = 
	MPIDI_CH3_PktHandler_ReadySend;
    pktArray[MPIDI_CH3_PKT_EAGER_SYNC_SEND] = 
	MPIDI_CH3_PktHandler_EagerSyncSend;
    pktArray[MPIDI_CH3_PKT_EAGER_SYNC_ACK] = 
	MPIDI_CH3_PktHandler_EagerSyncAck;
    pktArray[MPIDI_CH3_PKT_RNDV_REQ_TO_SEND] =
	MPIDI_CH3_PktHandler_RndvReqToSend;
    pktArray[MPIDI_CH3_PKT_RNDV_CLR_TO_SEND] = 
	MPIDI_CH3_PktHandler_RndvClrToSend;
    pktArray[MPIDI_CH3_PKT_RNDV_SEND] = 
	MPIDI_CH3_PktHandler_RndvSend;
    pktArray[MPIDI_CH3_PKT_CANCEL_SEND_REQ] = 
	MPIDI_CH3_PktHandler_CancelSendReq;
    pktArray[MPIDI_CH3_PKT_CANCEL_SEND_RESP] = 
	MPIDI_CH3_PktHandler_CancelSendResp;
    pktArray[MPIDI_CH3_PKT_PUT] = 
	MPIDI_CH3_PktHandler_Put;
    pktArray[MPIDI_CH3_PKT_ACCUMULATE] = 
	MPIDI_CH3_PktHandler_Accumulate;
    pktArray[MPIDI_CH3_PKT_GET] = 
	MPIDI_CH3_PktHandler_Get;
    pktArray[MPIDI_CH3_PKT_GET_RESP] = 
	MPIDI_CH3_PktHandler_GetResp;
    pktArray[MPIDI_CH3_PKT_LOCK] =
	MPIDI_CH3_PktHandler_Lock;
    pktArray[MPIDI_CH3_PKT_LOCK_GRANTED] =
	MPIDI_CH3_PktHandler_LockGranted;
    pktArray[MPIDI_CH3_PKT_PT_RMA_DONE] = 
	MPIDI_CH3_PktHandler_PtRMADone;
    pktArray[MPIDI_CH3_PKT_LOCK_PUT_UNLOCK] = 
	MPIDI_CH3_PktHandler_LockPutUnlock;
    pktArray[MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK] =
	MPIDI_CH3_PktHandler_LockAccumUnlock;
    pktArray[MPIDI_CH3_PKT_LOCK_GET_UNLOCK] = 
	MPIDI_CH3_PktHandler_LockGetUnlock;
    pktArray[MPIDI_CH3_PKT_CLOSE] =
	MPIDI_CH3_PktHandler_Close;
    pktArray[MPIDI_CH3_PKT_FLOW_CNTL_UPDATE] = 0;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_INIT);
    return mpi_errno;
}
    
