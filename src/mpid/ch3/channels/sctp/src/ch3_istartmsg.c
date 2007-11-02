/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME create_request
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static MPID_Request * create_request(void * hdr, MPIDI_msg_sz_t hdr_sz, MPIU_Size_t nb)
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
    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) hdr;
    sreq->dev.iov[0].MPID_IOV_BUF = 
	(MPID_IOV_BUF_CAST)((char *) &sreq->dev.pending_pkt + nb);
    sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz - nb;
    sreq->dev.iov_count = 1;
    sreq->dev.OnDataAvail = 0;
    
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST);
    return sreq;
}

/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If the entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the header is copied into the request, and a pointer to the request is returned.
 * An error condition also results in a request be allocated and the errror being returned in the status field of the request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void * hdr, MPIDI_msg_sz_t hdr_sz, MPID_Request ** sreq_ptr)
{
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    int stream_no, ppid;
    MPIDI_CH3_Pkt_t* pkt;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#ifdef MPICH_DBG_OUTPUT
    /* --BEGIN ERROR HANDLING-- */
    if (hdr_sz > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
#endif

    /* The sctp channel uses a fixed length header, the size of which is the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    /* find out stream no. */
    pkt = (MPIDI_CH3_Pkt_t*) hdr;
    stream_no = Req_Stream_from_pkt_and_req(pkt, *sreq_ptr);  /*  don't know pkt type here so pass it in */
    ppid = 0;

    if (SEND_CONNECTED(vc, stream_no) == MPIDI_CH3I_VC_STATE_CONNECTED)
    {
	/* Connection already formed.  If send queue is empty attempt to send data, queuing any unsent data. */
	if (!SEND_ACTIVE(vc, stream_no)) /* MT */
	{
	    MPIU_Assert(MPIDI_CH3I_SendQ_empty_x(vc, stream_no));

	    MPIU_Size_t nb;
	    int rc;

	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	    
	    /* MT - need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
               not also try to write */
	    rc = MPIDU_Sctp_write(vc, hdr, hdr_sz, stream_no, ppid, &nb);
	    
	    if (rc == MPI_SUCCESS)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "wrote %ld bytes", (unsigned long) nb));
		
		if (nb == hdr_sz)
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "entire write complete, %d bytes", nb));
		    /* done.  get us out of here as quickly as possible. */
		}
		else
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "partial write of %d bytes, request enqueued at head", nb));
		    sreq = create_request(hdr, hdr_sz, nb);

		    /* --BEGIN ERROR HANDLING-- */
		    if (sreq == NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**nomem", 0);
			goto fn_fail;
		    }
		    /* --END ERROR HANDLING-- */
                    
		    /* put in in Global SendQ */
		    MPIDU_Sctp_post_write(vc, sreq, hdr_sz-nb, hdr_sz-nb, NULL, stream_no); 

		    MPIDI_DBG_PRINTF((55, FCNAME, "posting write, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
		    
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p", /* FIXME change error code */
							 sreq, vc->ch, vc);
			goto fn_fail;
		    }
		    /* --END ERROR HANDLING-- */
		}
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - MPIDU_Sctp_write failed, rc=%d", rc));
		sreq = MPID_Request_create();
		if (sreq == NULL)
		{
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		    goto fn_fail;
		}
		sreq->kind = MPID_REQUEST_SEND;
		sreq->cc = 0;
		sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "send in progress, request enqueued"));
	    sreq = create_request(hdr, hdr_sz, 0);
	    /* --BEGIN ERROR HANDLING-- */
	    if (sreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	    MPIDI_CH3I_SendQ_enqueue_x(vc, sreq, stream_no);
	}
    }
    else if (SEND_CONNECTED(vc, stream_no) == MPIDI_CH3I_VC_STATE_UNCONNECTED) /* MT */
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  posting connect and enqueuing request"));
	
	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(hdr, hdr_sz, 0);
	/* --BEGIN ERROR HANDLING-- */
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
        
	/* Form a new connection, called once per association (i.e. not per stream) */
        if(vc->ch.pkt == NULL)
        {
            mpi_errno = MPIDI_CH3I_VC_post_connect(vc);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */
        }
        
	MPIDU_Sctp_stream_init(vc, sreq, stream_no);
    }
    else if (vc->ch.state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIDI_DBG_PRINTF((55, FCNAME, "forming connection, request enqueued"));
	sreq = create_request(hdr, hdr_sz, 0);
	/* --BEGIN ERROR HANDLING-- */
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
	MPIDU_Sctp_stream_init(vc, sreq, stream_no);
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	/* Connection failed, so allocate a request and return an error. */
	MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - connection failed"));
	sreq = MPID_Request_create();
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    goto fn_fail;
	}
	sreq->kind = MPID_REQUEST_SEND;
	sreq->cc = 0;
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    *sreq_ptr = sreq;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */    
    goto fn_exit;
    /* --END ERROR HANDLING-- */    
}
