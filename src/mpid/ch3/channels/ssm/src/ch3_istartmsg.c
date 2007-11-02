/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* We use this routine to create and enqueue a send request.  It's ok
   to use a routine here because messages are normally dispatched without
   a request.  Because it may be used to queue a partial packet, we include
   a "bytes-sent-so-far" (nb) parameter.
*/
#undef FUNCNAME
#define FUNCNAME ssm_ch3istartmsg_createRequest
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int createRequest( void *pkt, MPIDI_msg_sz_t pkt_sz, int nb,
			  MPID_Request **sreq_ptr )
{
    MPID_Request * sreq;
    int            mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_AND_ENQUEUE_REQUEST);
    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_AND_ENQUEUE_REQUEST);
    sreq = MPID_Request_create();
    if (sreq == NULL) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem",
			     "**nomem %s","MPID_Request");
    }
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind   = MPID_REQUEST_SEND;
    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) pkt;
    sreq->dev.iov[0].MPID_IOV_BUF = 
	(MPID_IOV_BUF_CAST)((char *) &sreq->dev.pending_pkt + nb);
    sreq->dev.iov[0].MPID_IOV_LEN = pkt_sz - nb;
    sreq->dev.iov_count   = 1;
    sreq->dev.iov_offset  = 0;
    sreq->dev.OnDataAvail = 0;
    *sreq_ptr = sreq;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_AND_ENQUEUE_REQUEST);
    return mpi_errno;
}

/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  
 * If the entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the header is copied into the
 * request, and a pointer to the request is returned.
 * An error condition also results in a request be allocated and the error 
 * being returned in the status field of the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void * pkt, MPIDI_msg_sz_t pkt_sz, 
			MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIU_Assert(pkt_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* The ssm channel uses a fixed length header, the size of which is the 
       maximum of all possible packet headers */
    pkt_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)pkt);
    
    if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send 
	   data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vcch)) /* MT */
	{
	    int nb;

	    /* MT - need some signalling to lock down our right to use the 
	       channel, thus insuring that the progress engine does
               not also try to write */
	    if (vcch->bShm) {
		mpi_errno = MPIDI_CH3I_SHM_write(vc, pkt, pkt_sz, &nb);
	    }
	    else {
		MPIDU_Sock_size_t snb;
		mpi_errno = MPIDU_Sock_write(vcch->sock, pkt, pkt_sz, &snb);
		nb = snb;
	    }
	    if (mpi_errno == MPI_SUCCESS) {
		if (nb == pkt_sz) {
		    MPIDI_DBG_PRINTF((55, FCNAME, "entire write complete, %d bytes", nb));
		    /* done.  get us out of here as quickly as possible. */
		    goto fn_exit;
		}
		else
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "partial write of %d bytes, request enqueued at head", nb));
		    mpi_errno = createRequest( pkt, pkt_sz, nb, &sreq );
		    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
		    MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
		    if (vcch->bShm) {
			vcch->send_active = sreq;
		    }
		    else {
			/*MPIDI_CH3I_SSM_VC_post_write(vc, sreq);*/
			MPIDI_DBG_PRINTF((55, FCNAME, "posting write, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
			vcch->conn->send_active = sreq;
			/* We have to post this one since we're adding it
			   to the head of the queue */
			mpi_errno = MPIDU_Sock_post_write(vcch->conn->sock, 
			    sreq->dev.iov[0].MPID_IOV_BUF,
			    sreq->dev.iov[0].MPID_IOV_LEN, 
                            sreq->dev.iov[0].MPID_IOV_LEN, NULL);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
				"**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
				sreq, vcch->conn, vc);
			    goto fn_exit;
			}
		    }
		}
	    }
	    else
	    {
		sreq = MPID_Request_create();
		if (sreq == NULL)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		    goto fn_exit;
		}
		sreq->kind = MPID_REQUEST_SEND;
		sreq->cc = 0;
		/* TODO: Create an appropriate error message based on the return value */
		sreq->status.MPI_ERROR = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ssmwrite", 0);
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "send in progress, request enqueued"));
	    mpi_errno = createRequest( pkt, pkt_sz, 0, &sreq );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	}
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTING) /* MT */
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "connecting.  enqueuing request"));
	
	/* queue the data so it can be sent after the connection is formed */
	mpi_errno = createRequest( pkt, pkt_sz, 0, &sreq );
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_UNCONNECTED) /* MT */
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  posting connect and enqueuing request"));
	
	/* Form a new connection */

	/* queue the data so it can be sent after the connection is formed */
	mpi_errno = createRequest( pkt, pkt_sz, 0, &sreq );
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	MPIDI_CH3I_VC_post_connect(vc);
    }
    else if (vcch->state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIDI_DBG_PRINTF((55, FCNAME, "forming connection, request enqueued"));
	mpi_errno = createRequest( pkt, pkt_sz, 0, &sreq );
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    else
    {
	/* Connection failed, so allocate a request and return an error. */
	MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - connection failed"));
	sreq = MPID_Request_create();
	if (sreq == NULL) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem",
				 "**nomem %s","MPID_Request");
	}
	sreq->kind = MPID_REQUEST_SEND;
	sreq->cc = 0;
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
    }

 fn_fail:
 fn_exit:
    *sreq_ptr = sreq;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
}
