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
static void update_request(MPID_Request * sreq, MPID_IOV * iov, int iov_count, int iov_offset, MPIU_Size_t nb)
{
    int i;
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST);

    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST);

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
    sreq->dev.iov[iov_offset].MPID_IOV_BUF = 
	(MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[iov_offset].MPID_IOV_BUF + nb);
    sreq->dev.iov[iov_offset].MPID_IOV_LEN -= nb;
    sreq->dev.iov_count = iov_count;

    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPID_Request * sreq, MPID_IOV * iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int complete = TRUE;
    MPIDI_CH3_Pkt_t * pkt;
    int stream_no, ppid;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#ifdef MPICH_DBG_OUTPUT
    /* --BEGIN ERROR HANDLING-- */
    if (n_iov > MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
	return mpi_errno;
    }
    if (iov[0].MPID_IOV_LEN > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
#endif

    /* The sctp channel uses a fixed length header, the size of which is the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)iov[0].MPID_IOV_BUF);

    /* find out stream no. */
    pkt = (MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF;
    stream_no = Req_Stream_from_pkt_and_req(pkt, sreq);
    ppid = 0;

    if (SEND_CONNECTED(vc, stream_no) == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send data, queuing any unsent data. */
	if (!SEND_ACTIVE(vc, stream_no)) /* MT */ /* FIXME check specific sendQ and send_active */
	{
	    MPIU_Assert(MPIDI_CH3I_SendQ_empty_x(vc, stream_no));

	    MPIU_Size_t nb;
	    int rc;

	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	    
	    /* MT - need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
               also try to write */

	    /* FIXME: the current code only agressively writes the first IOV.  Eventually it should be changed to agressively write
               as much as possible.  Ideally, the code would be shared between the send routines and the progress engine. */
	    rc = MPIDU_Sctp_writev(vc, iov, n_iov, stream_no, ppid, &nb);

	    if (rc == MPI_SUCCESS)
	    {
		int offset = 0;

		MPIDI_DBG_PRINTF((55, FCNAME, "wrote %ld bytes", (unsigned long) nb));
		
		while (offset < n_iov)
		{
		    if ((int)iov[offset].MPID_IOV_LEN <= nb)
		    {
			nb -= iov[offset].MPID_IOV_LEN;
			offset++;
		    }
		    else
		    {
			MPIDI_DBG_PRINTF((55, FCNAME, "partial write, request enqueued at head"));
			update_request(sreq, iov, n_iov, offset, nb);
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
		    MPIDI_DBG_PRINTF((55, FCNAME, "write complete, calling MPIDI_CH3U_Handle_send_req()"));
		    MPIDI_CH3U_Handle_send_req(vc, sreq, &complete);
		    if (!complete)
		    {
			MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
			
			MPIDU_Sctp_post_writev(vc, sreq, 0, NULL, stream_no);
			
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
							     sreq, vc->ch, vc);
			}
			/* --END ERROR HANDLING-- */
		    }
		}
	    }
	    /* --BEGIN ERROR HANDLING-- */
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "MPIDU_Sctp_writev failed, rc=%d", rc));
		/* Connection just failed.  Mark the request complete and return an error. */
		MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_FAILED");
		vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
		sreq->status.MPI_ERROR = MPI_ERR_INTERN;
		 /* MT - CH3U_Request_complete performs write barrier */
		MPIDI_CH3U_Request_complete(sreq);
	    }
	    /* --END ERROR HANDLING-- */
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue not empty, enqueuing"));
	    update_request(sreq, iov, n_iov, 0, 0);
	    
	    MPIDI_CH3I_SendQ_enqueue_x(vc, sreq, stream_no);
	}
    }
    else if (SEND_CONNECTED(vc, stream_no) == MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	/* Form a new connection, queuing the data so it can be sent later. */
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  enqueuing request"));
	update_request(sreq, iov, n_iov, 0, 0);

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
    else if (SEND_CONNECTED(vc, stream_no) != MPIDI_CH3I_VC_STATE_FAILED)
    {        
	/* Unable to send data at the moment, so queue it for later */
	MPIDI_DBG_PRINTF((55, FCNAME, "still connecting.  enqueuing request"));
	update_request(sreq, iov, n_iov, 0, 0);
	MPIDI_CH3I_SendQ_enqueue_x(vc, sreq, stream_no); /* FIXME enqueue x */
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "connection failed"));
	/* Connection failed.  Mark the request complete and return an error. */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	/* MT - CH3U_Request_complete performs write barrier */
	MPIDI_CH3U_Request_complete(sreq);
    }
    /* --END ERROR HANDLING-- */

 fn_exit:
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */    
    goto fn_exit;
    /* --END ERROR HANDLING-- */    
}

