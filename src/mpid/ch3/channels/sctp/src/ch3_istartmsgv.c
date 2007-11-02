/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "sctp_common.h"

#undef FUNCNAME
#define FUNCNAME create_request
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static MPID_Request * create_request(MPID_IOV * iov, int iov_count, int iov_offset, MPIU_Size_t nb)
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
	MPIU_Assert(iov[0].MPID_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t));
	sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) iov[0].MPID_IOV_BUF;
	sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &sreq->dev.pending_pkt;
    }
    sreq->dev.iov[iov_offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[iov_offset].MPID_IOV_BUF + nb);
    sreq->dev.iov[iov_offset].MPID_IOV_LEN -= nb;
    sreq->dev.iov_count = iov_count;
    sreq->dev.OnDataAvail = 0;

    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST);
    return sreq;
}

/*
 * MPIDI_CH3_iStartMsgv() attempts to send the message immediately.  If the entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the iovec and the first buffer pointed to by the iovec (which is assumed to be a
 * MPIDI_CH3_Pkt_t) are copied into the request, and a pointer to the request is returned.  An error condition also results in a
 * request be allocated and the errror being returned in the status field of the request.
 */

/* XXX - What do we do if MPIDI_CH3_Request_create() returns NULL???  If MPIDI_CH3_iStartMsgv() returns NULL, the calling code
   assumes the request completely successfully, but the reality is that we couldn't allocate the memory for a request.  This
   seems like a flaw in the CH3 API. */

/* NOTE - The completion action associated with a request created by CH3_iStartMsgv() is alway MPIDI_CH3_CA_COMPLETE.  This
   implies that CH3_iStartMsgv() can only be used when the entire message can be described by a single iovec of size
   MPID_IOV_LIMIT. */
    
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsgv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsgv(MPIDI_VC_t * vc, MPID_IOV * iov, int n_iov, MPID_Request ** sreq_ptr)
{
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * conn_req = NULL;
    int stream_no, ppid;
    MPIDI_CH3_Pkt_t* pkt;
    

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#ifdef MPICH_DBG_OUTPUT
    /* --BEGIN ERROR HANDLING-- */
    if (n_iov > MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	goto fn_fail;
    }
    if (iov[0].MPID_IOV_LEN > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
#endif

    /* The sctp channel uses a fixed length header, the size of which is the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF);

    /* find out stream no. */
    pkt = (MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF;
    stream_no = Req_Stream_from_pkt_and_req(pkt, *sreq_ptr);
    ppid = 0;
    
    if (SEND_CONNECTED(vc, stream_no) == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send data, queuing any unsent data. */
	if (!SEND_ACTIVE(vc, stream_no)) /* MT */
	{
	    MPIU_Assert(MPIDI_CH3I_SendQ_empty_x(vc, stream_no));

	    MPIU_Size_t nb = 0;
	    int rc;

	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	    
	    /* MT - need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
               also try to write */
	    rc = MPIDU_Sctp_writev(vc, iov, n_iov, stream_no, ppid, &nb);
	    
	    if (rc == MPI_SUCCESS)
	    {
		int offset = 0;
    
		MPIDI_DBG_PRINTF((55, FCNAME, "wrote %ld bytes", (unsigned long) nb));
		
		while (offset < n_iov)
		{
		    if (nb >= (int)iov[offset].MPID_IOV_LEN)
		    {
			nb -= iov[offset].MPID_IOV_LEN;
			offset++;
		    }
		    else
		    {
			MPIDI_DBG_PRINTF((55, FCNAME, "partial write, request enqueued at head"));
			sreq = create_request(iov, n_iov, offset, nb);

			/* --BEGIN ERROR HANDLING-- */
			if (sreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**nomem", 0);
			    goto fn_fail;
			}
			/* --END ERROR HANDLING-- */
			MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
			
			MPIDU_Sctp_post_writev(vc, sreq, offset, NULL, stream_no);
				
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
							     sreq, vc->ch, vc);
			}
			/* --END ERROR HANDLING-- */
			break;
		    }
		}

		if (offset == n_iov)
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "entire write complete"));
		}
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - MPIDU_Sctp_writev failed, rc=%d", rc));
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
	    sreq = create_request(iov, n_iov, 0, 0);
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
    else if (SEND_CONNECTED(vc, stream_no) == MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  posting connect and enqueuing request"));
	
	/* queue the data so it can be sent after the connection is formed */
	sreq = create_request(iov, n_iov, 0, 0);
	/* --BEGIN ERROR HANDLING-- */
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	/* Form a new connection */
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
	sreq = create_request(iov, n_iov, 0, 0);
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
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
    }
    /* --END ERROR HANDLING-- */

  fn_exit:
    *sreq_ptr = sreq;
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */    
    goto fn_exit;
    /* --END ERROR HANDLING-- */    
    
}
