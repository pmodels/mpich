/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME update_request
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void update_request(MPID_Request * sreq, void * hdr, 
			   MPIDI_msg_sz_t hdr_sz, MPIU_Size_t nb)
{
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST);
    MPIU_Assert(hdr_sz == sizeof(MPIDI_CH3_Pkt_t));
    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) hdr;
    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) &sreq->dev.pending_pkt + nb);
    sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz - nb;
    sreq->dev.iov_count = 1;
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSend(MPIDI_VC_t * vc, MPID_Request * sreq, void * hdr, 
		    MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISEND);

    MPIU_Assert( hdr_sz <= sizeof(MPIDI_CH3_Pkt_t) );

    /* The sock channel uses a fixed length header, the size of which is the 
       maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIU_DBG_STMT(CH3_CHANNEL,VERBOSE,
		  MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr));

    if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send 
	   data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vcch)) /* MT */
	{
	    MPIU_Size_t nb;
	    int rc;

	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
			 "send queue empty, attempting to write");
	    MPIU_DBG_PKT(vcch->conn,hdr,"isend");
	    /* MT: need some signalling to lock down our right to use the 
	       channel, thus insuring that the progress engine does
               also try to write */
	    rc = MPIDU_Sock_write(vcch->sock, hdr, hdr_sz, &nb);
	    if (rc == MPI_SUCCESS)
	    {
		MPIU_DBG_MSG_D(CH3_CHANNEL,VERBOSE,
			       "wrote %ld bytes", (unsigned long) nb);
		
		if (nb == hdr_sz)
		{
		    MPIU_DBG_MSG_D(CH3_CHANNEL,VERBOSE,
                     "write complete " MPIDI_MSG_SZ_FMT " bytes, calling OnDataAvail fcn", nb);
		    reqFn = sreq->dev.OnDataAvail;
		    if (!reqFn) {
			MPIU_Assert(MPIDI_Request_get_type(sreq)!=MPIDI_REQUEST_TYPE_GET_RESP);
			MPIDI_CH3U_Request_complete(sreq);
		    }
		    else {
			int complete;
			mpi_errno = reqFn( vc, sreq, &complete );
			if (mpi_errno) MPIU_ERR_POP(mpi_errno);
			if (!complete) {
			    MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
			    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
					     (MPIU_DBG_FDEST,
                    "posting writev, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
			    vcch->conn->send_active = sreq;
			    mpi_errno = MPIDU_Sock_post_writev(
				vcch->conn->sock, sreq->dev.iov, 
				sreq->dev.iov_count, NULL);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
								 sreq, vcch->conn, vc);
			    }
			    /* --END ERROR HANDLING-- */
			}
		    }
		}
		else
		{
		    MPIU_DBG_MSG_D(CH3_CHANNEL,VERBOSE,
                     "partial write of " MPIDI_MSG_SZ_FMT " bytes, request enqueued at head", nb);
		    update_request(sreq, hdr, hdr_sz, nb);
		    MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
		    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
     (MPIU_DBG_FDEST,"posting write, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
		    vcch->conn->send_active = sreq;
		    mpi_errno = MPIDU_Sock_post_write(vcch->conn->sock, 
					  sreq->dev.iov[0].MPID_IOV_BUF,
				          sreq->dev.iov[0].MPID_IOV_LEN, 
					  sreq->dev.iov[0].MPID_IOV_LEN, NULL);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
							 sreq, vcch->conn, vc);
		    }
		    /* --END ERROR HANDLING-- */
		}
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else if (MPIR_ERR_GET_CLASS(rc) == MPIDU_SOCK_ERR_NOMEM)
	    {
		MPIU_DBG_MSG(CH3_CHANNEL,TYPICAL,
			     "MPIDU_Sock_write failed, out of memory");
		sreq->status.MPI_ERROR = MPIR_ERR_MEMALLOCFAILED;
	    }
	    else
	    {
		MPIU_DBG_MSG_D(CH3_CHANNEL,TYPICAL,
			       "MPIDU_Sock_write failed, rc=%d", rc);
		/* Connection just failed. Mark the request complete and 
		   return an error. */
		MPIU_DBG_VCCHSTATECHANGE(vc,VC_STATE_FAILED);
		/* FIXME: Shouldn't the vc->state also change? */
		vcch->state = MPIDI_CH3I_VC_STATE_FAILED;
		sreq->status.MPI_ERROR = MPIR_Err_create_code( rc,
			       MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
			       MPI_ERR_INTERN, "**ch3|sock|writefailed", 
			       "**ch3|sock|writefailed %d", rc );
		 /* MT -CH3U_Request_complete() performs write barrier */
		MPIDI_CH3U_Request_complete(sreq);
		/* Make sure that the caller sees this error */
		mpi_errno = sreq->status.MPI_ERROR;
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"send queue not empty, enqueuing");
	    update_request(sreq, hdr, hdr_sz, 0);
	    MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	}
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTING) /* MT */
    {
	/* queuing the data so it can be sent later. */
	MPIU_DBG_VCUSE(vc,"connecting.  enqueuing request");
	update_request(sreq, hdr, hdr_sz, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_UNCONNECTED) /* MT */
    {
	/* Form a new connection, queuing the data so it can be sent later. */
	MPIU_DBG_VCUSE(vc,"unconnected.  enqueuing request");
	update_request(sreq, hdr, hdr_sz, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	mpi_errno = MPIDI_CH3I_VC_post_connect(vc);
	if (mpi_errno) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }
    else if (vcch->state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIU_DBG_VCUSE(vc,"still connecting. Enqueuing request");
	update_request(sreq, hdr, hdr_sz, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	/* Connection failed.  Mark the request complete and return an error. */
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	/* MT - CH3U_Request_complete() performs write barrier */
	MPIDI_CH3U_Request_complete(sreq);
    }
    /* --END ERROR HANDLING-- */

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
    return mpi_errno;
}

