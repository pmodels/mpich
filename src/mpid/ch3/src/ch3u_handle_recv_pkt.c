/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"
#include "mpidi_recvq_statistics.h"

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
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.parts.rank;	\
    (rreq_)->status.MPI_TAG = (pkt_)->match.parts.tag;		\
    MPIR_STATUS_SET_COUNT((rreq_)->status, (pkt_)->data_sz);		\
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;		\
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;		\
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);		\
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));		\
}


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
#define FCNAME MPL_QUOTE(FUNCNAME)
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

       MPIU_Assert(pkt->type <= MAX_PACKET_TYPE);
       mpi_errno = MPIDI_CH3_ProgressFunctions[pkt->type](vc,pkt,rreqp);
       
       in the progress engine itself.  Then this routine is not necessary.
    */

    if (needsInit) {
	MPIDI_CH3_PktHandler_Init( pktArray, MPIDI_CH3_PKT_END_CH3 );
	needsInit = 0;
    }
    /* Packet type is an enum and hence >= 0 */
    MPIU_Assert(pkt->type <= MPIDI_CH3_PKT_END_CH3);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
	MPIR_STATUS_SET_COUNT(rreq->status, userbuf_sz);
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
            if (rreq->dev.drop_data == FALSE) {
                MPIU_Memcpy((char*)(rreq->dev.user_buf) + dt_true_lb, buf, data_sz);
            }
            *buflen = data_sz;
            *complete = TRUE;
        }
        else
        {
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for contiguous read");
            
            rreq->dev.iov[0].MPL_IOV_BUF = 
                (MPL_IOV_BUF_CAST)((char*)(rreq->dev.user_buf) + dt_true_lb);
            rreq->dev.iov[0].MPL_IOV_LEN = data_sz;
            rreq->dev.iov_count = 1;
            *buflen = 0;
            *complete = FALSE;
        }
        
        /* Trigger OnFinal when receiving the last segment */
        rreq->dev.OnDataAvail = rreq->dev.OnFinal;
    }
    else {
	/* user buffer is not contiguous or is too small to hold
	   the entire message */
        
	rreq->dev.segment_ptr = MPID_Segment_alloc( );
        MPIR_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

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
                MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
                MPIR_STATUS_SET_COUNT(rreq->status, rreq->dev.segment_first);
                *buflen = data_sz;
                *complete = TRUE;
		/* FIXME: Set OnDataAvail to 0?  If not, why not? */
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            *buflen = data_sz;
            /* Trigger OnFinal when receiving the last segment */
            rreq->dev.OnDataAvail = rreq->dev.OnFinal;
            *complete = TRUE;
        }
        else
        {   
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for non-contiguous read");

            mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
	MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
			     rreq->dev.recv_data_sz);
    }
    rreq->dev.tmpbuf_sz = rreq->dev.recv_data_sz;
    
    /* if all of the data has already been received, copy it
       now, otherwise build an iov and let the channel copy it */
    if (rreq->dev.recv_data_sz <= *buflen)
    {
        MPIU_Memcpy(rreq->dev.tmpbuf, buf, rreq->dev.recv_data_sz);
        *buflen = rreq->dev.recv_data_sz;
        rreq->dev.recv_pending_count = 1;
        *complete = TRUE;
    }
    else
    {
        rreq->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)((char *)rreq->dev.tmpbuf);
        rreq->dev.iov[0].MPL_IOV_LEN = rreq->dev.recv_data_sz;
        rreq->dev.iov_count = 1;
        rreq->dev.recv_pending_count = 2;
        *buflen = 0;
        *complete = FALSE;
    }

    if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
        MPIR_T_PVAR_LEVEL_INC(RECVQ, unexpected_recvq_buffer_size, rreq->dev.tmpbuf_sz);

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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Post_data_receive_found(MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;	
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype * dt_ptr = NULL;
    MPIDI_msg_sz_t data_sz;
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
	MPIR_STATUS_SET_COUNT(rreq->status, userbuf_sz);
	data_sz = userbuf_sz;
    }

    if (dt_contig && data_sz == rreq->dev.recv_data_sz)
    {
	/* user buffer is contiguous and large enough to store the
	   entire message.  However, we haven't yet *read* the data 
	   (this code describes how to read the data into the destination) */
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for contiguous read");
	rreq->dev.iov[0].MPL_IOV_BUF = 
	    (MPL_IOV_BUF_CAST)((char*)(rreq->dev.user_buf) + dt_true_lb);
	rreq->dev.iov[0].MPL_IOV_LEN = data_sz;
	rreq->dev.iov_count = 1;
	/* FIXME: We want to set the OnDataAvail to the appropriate 
	   function, which depends on whether this is an RMA 
	   request or a pt-to-pt request. */
	rreq->dev.OnDataAvail = 0;
    }
    else {
	/* user buffer is not contiguous or is too small to hold
	   the entire message */
	MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"IOV loaded for non-contiguous read");
	rreq->dev.segment_ptr = MPID_Segment_alloc( );
        MPIR_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
	MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, 
			  rreq->dev.datatype, rreq->dev.segment_ptr, 0);
	rreq->dev.segment_first = 0;
	rreq->dev.segment_size = data_sz;
	mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIR_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,
				     "**ch3|loadrecviov");
	}
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_POST_DATA_RECEIVE_FOUND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Post_data_receive_unexpected
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
	MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
			     rreq->dev.recv_data_sz);
    }
    rreq->dev.tmpbuf_sz = rreq->dev.recv_data_sz;
    
    rreq->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)rreq->dev.tmpbuf;
    rreq->dev.iov[0].MPL_IOV_LEN = rreq->dev.recv_data_sz;
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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


/* FIXME: we still need to implement flow control.  As a reminder, 
   we don't mark these parameters as unused, because a full implementation
   of this routine will need to make use of all 4 parameters */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_FlowCntlUpdate
#undef FCNAME
int MPIDI_CH3_PktHandler_FlowCntlUpdate( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					 MPIDI_msg_sz_t *buflen, MPID_Request **rreqp)
{
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    return MPI_SUCCESS;
}

/* This is a dummy handler*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_EndCH3
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_EndCH3( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
				 MPIDI_CH3_Pkt_t *pkt ATTRIBUTE((unused)),
				 MPIDI_msg_sz_t *buflen ATTRIBUTE((unused)), 
				 MPID_Request **rreqp ATTRIBUTE((unused)) )
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Init( MPIDI_CH3_PktHandler_Fcn *pktArray[], 
			       int arraySize  )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_INIT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_INIT);

    /* Check that the array is large enough */
    if (arraySize < MPIDI_CH3_PKT_END_CH3) {
	MPIR_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN,
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

    /* Connection Management */
    pktArray[MPIDI_CH3_PKT_CLOSE] =
	MPIDI_CH3_PktHandler_Close;

    /* Provision for flow control */
    pktArray[MPIDI_CH3_PKT_FLOW_CNTL_UPDATE] = 0;


    /* Default RMA operations */
    /* FIXME: This should be initialized by a separate, RMA routine.
       That would allow different RMA implementations.
       We could even do lazy initialization (make this part of win_create) */
    pktArray[MPIDI_CH3_PKT_PUT] = 
	MPIDI_CH3_PktHandler_Put;
    pktArray[MPIDI_CH3_PKT_PUT_IMMED] =
	MPIDI_CH3_PktHandler_Put;
    pktArray[MPIDI_CH3_PKT_ACCUMULATE] = 
	MPIDI_CH3_PktHandler_Accumulate;
    pktArray[MPIDI_CH3_PKT_ACCUMULATE_IMMED] =
	MPIDI_CH3_PktHandler_Accumulate;
    pktArray[MPIDI_CH3_PKT_GET] = 
	MPIDI_CH3_PktHandler_Get;
    pktArray[MPIDI_CH3_PKT_GET_RESP] = 
	MPIDI_CH3_PktHandler_GetResp;
    pktArray[MPIDI_CH3_PKT_GET_RESP_IMMED] =
	MPIDI_CH3_PktHandler_GetResp;
    pktArray[MPIDI_CH3_PKT_LOCK] =
	MPIDI_CH3_PktHandler_Lock;
    pktArray[MPIDI_CH3_PKT_LOCK_ACK] =
	MPIDI_CH3_PktHandler_LockAck;
    pktArray[MPIDI_CH3_PKT_LOCK_OP_ACK] =
	MPIDI_CH3_PktHandler_LockOpAck;
    pktArray[MPIDI_CH3_PKT_UNLOCK] =
        MPIDI_CH3_PktHandler_Unlock;
    pktArray[MPIDI_CH3_PKT_FLUSH] =
        MPIDI_CH3_PktHandler_Flush;
    pktArray[MPIDI_CH3_PKT_ACK] =
	MPIDI_CH3_PktHandler_Ack;
    pktArray[MPIDI_CH3_PKT_DECR_AT_COUNTER] =
        MPIDI_CH3_PktHandler_DecrAtCnt;
    pktArray[MPIDI_CH3_PKT_CAS_IMMED] =
        MPIDI_CH3_PktHandler_CAS;
    pktArray[MPIDI_CH3_PKT_CAS_RESP_IMMED] =
        MPIDI_CH3_PktHandler_CASResp;
    pktArray[MPIDI_CH3_PKT_FOP] =
        MPIDI_CH3_PktHandler_FOP;
    pktArray[MPIDI_CH3_PKT_FOP_IMMED] =
        MPIDI_CH3_PktHandler_FOP;
    pktArray[MPIDI_CH3_PKT_FOP_RESP] =
        MPIDI_CH3_PktHandler_FOPResp;
    pktArray[MPIDI_CH3_PKT_FOP_RESP_IMMED] =
        MPIDI_CH3_PktHandler_FOPResp;
    pktArray[MPIDI_CH3_PKT_GET_ACCUM] =
        MPIDI_CH3_PktHandler_GetAccumulate;
    pktArray[MPIDI_CH3_PKT_GET_ACCUM_IMMED] =
        MPIDI_CH3_PktHandler_GetAccumulate;
    pktArray[MPIDI_CH3_PKT_GET_ACCUM_RESP] =
        MPIDI_CH3_PktHandler_Get_AccumResp;
    pktArray[MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED] =
        MPIDI_CH3_PktHandler_Get_AccumResp;
    /* End of default RMA operations */

    /* Fault tolerance */
    pktArray[MPIDI_CH3_PKT_REVOKE] =
        MPIDI_CH3_PktHandler_Revoke;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_INIT);
    return mpi_errno;
}
    
