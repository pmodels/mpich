/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME create_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static MPID_Request * create_request(MPL_IOV * iov, int iov_count, 
				     int iov_offset, MPIU_Size_t nb)
{
    MPID_Request * sreq;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST);
    
    sreq = MPID_Request_create();
    /* --BEGIN ERROR HANDLING-- */
    if (sreq == NULL)
	return NULL;
    /* --END ERROR HANDLING-- */
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    
    for (i = 0; i < iov_count; i++)
    {
	sreq->dev.iov[i] = iov[i];
    }
    if (iov_offset == 0)
    {
	MPIU_Assert(iov[0].MPL_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t));
	sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPL_IOV_BUF;
	sreq->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) &sreq->dev.pending_pkt;
    }
    sreq->dev.iov[iov_offset].MPL_IOV_BUF = (MPL_IOV_BUF_CAST)((char *) sreq->dev.iov[iov_offset].MPL_IOV_BUF + nb);
    sreq->dev.iov[iov_offset].MPL_IOV_LEN -= nb;
    sreq->dev.iov_count = iov_count;
    sreq->dev.OnDataAvail = 0;

    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST);
    return sreq;
}

/*
 * MPIDI_CH3_iStartMsgv() attempts to send the message immediately.  If the 
 * entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the iovec and the first buffer 
 * pointed to by the iovec (which is assumed to be a
 * MPIDI_CH3_Pkt_t) are copied into the request, and a pointer to the request 
 * is returned.  An error condition also results in a
 * request be allocated and the errror being returned in the status field of 
 * the request.
 */

/* XXX - What do we do if MPID_Request_create() returns NULL???  
   If MPIDI_CH3_iStartMsgv() returns NULL, the calling code
   assumes the request completely successfully, but the reality is that we 
   couldn't allocate the memory for a request.  This
   seems like a flaw in the CH3 API. */

/* NOTE - The completion action associated with a request created by 
   CH3_iStartMsgv() is alway MPIDI_CH3_CA_COMPLETE.  This
   implies that CH3_iStartMsgv() can only be used when the entire message can 
   be described by a single iovec of size
   MPL_IOV_LIMIT. */
    
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsgv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsgv(MPIDI_VC_t * vc, MPL_IOV * iov, int n_iov, 
			 MPID_Request ** sreq_ptr)
{
    MPID_Request * sreq = NULL;
    MPIDI_CH3I_VC *vcch = &vc->ch;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIU_Assert( n_iov <= MPL_IOV_LIMIT);

    /* The SOCK channel uses a fixed length header, the size of which is the 
       maximum of all possible packet headers */
    iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIU_DBG_STMT(CH3_CHANNEL,VERBOSE,
	   MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].MPL_IOV_BUF));
    
    if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send 
	   data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vcch)) /* MT */
	{
	    int rc;
	    MPIU_Size_t nb;

	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
			 "send queue empty, attempting to write");
	    MPIU_DBG_PKT(vcch->conn,(MPIDI_CH3_Pkt_t*)iov[0].MPL_IOV_BUF,"isend");
	    
	    /* MT - need some signalling to lock down our right to use the 
	       channel, thus insuring that the progress engine does
               also try to write */
	    rc = MPIDU_Sock_writev(vcch->sock, iov, n_iov, &nb);
	    if (rc == MPI_SUCCESS)
	    {
		int offset = 0;
    
		MPIU_DBG_MSG_D(CH3_CHANNEL,VERBOSE,
			       "wrote %ld bytes", (unsigned long) nb);
		
		while (offset < n_iov)
		{
		    if (nb >= (int)iov[offset].MPL_IOV_LEN)
		    {
			nb -= iov[offset].MPL_IOV_LEN;
			offset++;
		    }
		    else
		    {
			MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
			     "partial write, request enqueued at head");
			sreq = create_request(iov, n_iov, offset, nb);
			if (sreq == NULL) {
			    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
			}
			MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
			MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
					 (MPIU_DBG_FDEST,"posting writev, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
			vcch->conn->send_active = sreq;
			mpi_errno = MPIDU_Sock_post_writev(vcch->conn->sock, sreq->dev.iov + offset,
							   sreq->dev.iov_count - offset, NULL);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
							     sreq, vcch->conn, vc);
			}
			/* --END ERROR HANDLING-- */
			break;
		    }
		}

		if (offset == n_iov)
		{
		    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,"entire write complete");
		}
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else
	    {
		MPIU_DBG_MSG_D(CH3_CHANNEL,TYPICAL,
			       "ERROR - MPIDU_Sock_writev failed, rc=%d", rc);
		sreq = MPID_Request_create();
		if (sreq == NULL) {
		    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
		}
		sreq->kind = MPID_REQUEST_SEND;
        MPID_cc_set(&(sreq->cc), 0);
		sreq->status.MPI_ERROR = MPIR_Err_create_code( rc,
			       MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
			       MPI_ERR_INTERN, "**ch3|sock|writefailed", 
			       "**ch3|sock|writefailed %d", rc );
		/* Make sure that the caller sees this error */
		mpi_errno = sreq->status.MPI_ERROR;
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    MPIU_DBG_MSG(CH3_CHANNEL,VERBOSE,
			 "send in progress, request enqueued");
	    sreq = create_request(iov, n_iov, 0, 0);
	    if (sreq == NULL) {
		MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	    }
	    MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	}
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTING)
    {
	MPIU_DBG_VCUSE(vc,
		       "connecting.  enqueuing request");
	
	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(iov, n_iov, 0, 0);
	if (sreq == NULL) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	MPIU_DBG_VCUSE(vc,
		       "unconnected.  posting connect and enqueuing request");
	
	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(iov, n_iov, 0, 0);
	if (sreq == NULL) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	
	/* Form a new connection */
	MPIDI_CH3I_VC_post_connect(vc);
    }
    else if (vcch->state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIU_DBG_VCUSE(vc,"forming connection, request enqueued");
	sreq = create_request(iov, n_iov, 0, 0);
	if (sreq == NULL) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	/* Connection failed, so allocate a request and return an error. */
	MPIU_DBG_VCUSE(vc,"ERROR - connection failed");
	sreq = MPID_Request_create();
	if (sreq == NULL) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
	sreq->kind = MPID_REQUEST_SEND;
    MPID_cc_set(&(sreq->cc), 0);
	sreq->status.MPI_ERROR = MPIR_Err_create_code( MPI_SUCCESS,
		       MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
		       MPI_ERR_INTERN, "**ch3|sock|connectionfailed",0 );
	/* Make sure that the caller sees this error */
	mpi_errno = sreq->status.MPI_ERROR;
    }
    /* --END ERROR HANDLING-- */

  fn_fail:
    *sreq_ptr = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    return mpi_errno;
}
