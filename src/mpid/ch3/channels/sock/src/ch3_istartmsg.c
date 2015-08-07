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
static MPID_Request * create_request(void * hdr, MPIDI_msg_sz_t hdr_sz, 
				     MPIU_Size_t nb)
{
    MPID_Request * sreq;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST);

    sreq = MPID_Request_create();
    /* --BEGIN ERROR HANDLING-- */
    if (sreq == NULL)
	return NULL;
    /* --END ERROR HANDLING-- */
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    MPIU_Assert(hdr_sz == sizeof(MPIDI_CH3_Pkt_t));
    sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
    sreq->dev.iov[0].MPL_IOV_BUF = 
	(MPL_IOV_BUF_CAST)((char *) &sreq->dev.pending_pkt + nb);
    sreq->dev.iov[0].MPL_IOV_LEN = hdr_sz - nb;
    sreq->dev.iov_count = 1;
    sreq->dev.OnDataAvail = 0;
    
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST);
    return sreq;
}

/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If the 
 * entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the header is copied into the 
 * request, and a pointer to the request is returned.
 * An error condition also results in a request be allocated and the errror 
 * being returned in the status field of the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void * hdr, MPIDI_msg_sz_t hdr_sz, 
			MPID_Request ** sreq_ptr)
{
    MPID_Request * sreq = NULL;
    MPIDI_CH3I_VC *vcch = &vc->ch;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    
    MPIU_Assert( hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* The SOCK channel uses a fixed length header, the size of which is the 
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
	    
	    MPIU_DBG_PKT(vcch->conn,hdr,"istartmsg");
	    /* MT: need some signalling to lock down our right to use the 
	       channel, thus insuring that the progress engine does
               not also try to write */
	    rc = MPIDU_Sock_write(vcch->sock, hdr, hdr_sz, &nb);
	    if (rc == MPI_SUCCESS)
	    {
		MPIU_DBG_MSG_D(CH3_CHANNEL,VERBOSE,
			       "wrote %ld bytes", (unsigned long) nb);
		
		if (nb == hdr_sz)
		{ 
		    MPIU_DBG_MSG_D(CH3_CHANNEL,VERBOSE,
				   "entire write complete, " MPIDI_MSG_SZ_FMT " bytes", nb);
		    /* done.  get us out of here as quickly as possible. */
		}
		else
		{
		    MPIU_DBG_MSG_D(CH3_CHANNEL,VERBOSE,
                    "partial write of " MPIDI_MSG_SZ_FMT " bytes, request enqueued at head", nb);
		    sreq = create_request(hdr, hdr_sz, nb);
		    if (!sreq) {
			MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
		    }

		    MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
		    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
     (MPIU_DBG_FDEST,"posting write, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
		    vcch->conn->send_active = sreq;
		    mpi_errno = MPIDU_Sock_post_write(vcch->conn->sock, sreq->dev.iov[0].MPL_IOV_BUF,
						      sreq->dev.iov[0].MPL_IOV_LEN, sreq->dev.iov[0].MPL_IOV_LEN, NULL);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
							 sreq, vcch->conn, vc);
			goto fn_fail;
		    }
		    /* --END ERROR HANDLING-- */
		}
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else
	    {
		MPIU_DBG_MSG_D(CH3_CHANNEL,TYPICAL,
			       "ERROR - MPIDU_Sock_write failed, rc=%d", rc);
		sreq = MPID_Request_create();
		if (!sreq) {
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
	    sreq = create_request(hdr, hdr_sz, 0);
	    if (!sreq) {
		MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	    }
	    MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	}
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTING) /* MT */
    {
	MPIU_DBG_VCUSE(vc,
		       "connecteding. enqueuing request");
	
	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(hdr, hdr_sz, 0);
	if (!sreq) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_UNCONNECTED) /* MT */
    {
	MPIU_DBG_VCUSE(vc,
		       "unconnected.  posting connect and enqueuing request");
	
	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(hdr, hdr_sz, 0);
	if (!sreq) {
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
	sreq = create_request(hdr, hdr_sz, 0);
	if (!sreq) {
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
	if (!sreq) {
	    MPIR_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	}
	sreq->kind = MPID_REQUEST_SEND;
	MPID_cc_set(&sreq->cc, 0);
	
	sreq->status.MPI_ERROR = MPIR_Err_create_code( MPI_SUCCESS,
		       MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
		       MPI_ERR_INTERN, "**ch3|sock|connectionfailed",0 );
	/* Make sure that the caller sees this error */
	mpi_errno = sreq->status.MPI_ERROR;
    }
    /* --END ERROR HANDLING-- */

  fn_fail:
    *sreq_ptr = sreq;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
}
