/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*static void update_request(MPID_Request * sreq, MPID_IOV * iov, int count, int offset, int nb)*/
#undef update_request
#define update_request(sreq, iov, count, offset, nb) \
{ \
    int i; \
    MPIDI_STATE_DECL(MPID_STATE_UPDATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_UPDATE_REQUEST); \
    for (i = 0; i < count; i++) \
    { \
	sreq->dev.iov[i] = iov[i]; \
    } \
    if (offset == 0) \
    { \
	/*MPIU_Assert(iov[0].MPID_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t));*/ \
	sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *) iov[0].MPID_IOV_BUF; \
	sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &sreq->dev.pending_pkt; \
    } \
    sreq->dev.iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[offset].MPID_IOV_BUF + nb); \
    sreq->dev.iov[offset].MPID_IOV_LEN -= nb; \
    sreq->dev.iov_count = count; \
    sreq->dev.iov_offset = offset; \
    MPIDI_FUNC_EXIT(MPID_STATE_UPDATE_REQUEST); \
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPID_Request * sreq, MPID_IOV * iov, 
		     int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISENDV);

#ifdef MPICH_DBG_OUTPUT
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
#endif

    /* The ssm channel uses a fixed length header, the size of which is the 
       maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF);

    if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send 
	   data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vcch)) /* MT */
	{
	    int nb;

	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));
	    
	    /* MT - need some signalling to lock down our right to use the 
	       channel, thus insuring that the progress engine does
               also try to write */

	    /* FIXME: the current code only agressively writes the first IOV. 
	       Eventually it should be changed to agressively write
               as much as possible.  Ideally, the code would be shared between 
	       the send routines and the progress engine. */
	    if (vcch->bShm)
	    {
		mpi_errno = MPIDI_CH3I_SHM_writev(vc, iov, n_iov, &nb);
	    }
	    else
	    {
		MPIDU_Sock_size_t snb;
		mpi_errno = MPIDU_Sock_writev(vcch->sock, iov, n_iov, &snb);
		nb = snb;
	    }
	    if (mpi_errno == MPI_SUCCESS)
	    {
		int offset = 0;

		MPIDI_DBG_PRINTF((55, FCNAME, "wrote %d bytes", nb));
		
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
			MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
			if (vcch->bShm)
			{
			    vcch->send_active = sreq;
			}
			else
			{
			    /*MPIDI_CH3I_SSM_VC_post_write(vc, sreq);*/
			    MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
			    vcch->conn->send_active = sreq;
			    mpi_errno = MPIDU_Sock_post_writev(vcch->conn->sock, sreq->dev.iov + offset,
				sreq->dev.iov_count - offset, NULL);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
				    "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
				    sreq, vcch->conn, vc);
			    }
			}
			break;
		    }

		}
		if (offset == n_iov)
		{
		    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
		    MPIDI_DBG_PRINTF((55, FCNAME, "write complete"));
		    reqFn = sreq->dev.OnDataAvail;
		    if (!reqFn) {
			MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
			MPIDI_CH3U_Request_complete(sreq);
		    }
		    else {
			int complete;
			mpi_errno = reqFn( vc, sreq, &complete );
			if (mpi_errno) MPIU_ERR_POP(mpi_errno);
			if (!complete) {
			    sreq->dev.iov_offset = 0;
			    MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
			    if (vcch->bShm)
			    {
				vcch->send_active = sreq;
			    }
			    else
			    {
				MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
				vcch->conn->send_active = sreq;
				mpi_errno = MPIDU_Sock_post_writev(vcch->conn->sock, sreq->dev.iov, sreq->dev.iov_count, NULL);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								     "**ch3|sock|postwrite", "ch3|sock|postwrite %p %p %p",
								     sreq, vcch->conn, vc);
				}
			    }
			}
		    }
		}
	    }
	    else
	    {
		vcch->state = MPIDI_CH3I_VC_STATE_FAILED;
		/* TODO: Create an appropriate error message based on the value of errno */
		sreq->status.MPI_ERROR = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ssmwritev", 0);
		/* MT - CH3U_Request_complete performs write barrier */
		MPIDI_CH3U_Request_complete(sreq);
	    }
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue not empty, enqueuing"));
	    update_request(sreq, iov, n_iov, 0, 0);
	    MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	}
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTING)
    {
	/* Queuing the data so it can be sent later. */
	MPIDI_DBG_PRINTF((55, FCNAME, "connecting.  enqueuing request"));
	update_request(sreq, iov, n_iov, 0, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    else if (vcch->state == MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	/* Form a new connection, queuing the data so it can be sent later. */
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  enqueuing request"));
	/*MPIDI_CH3I_VC_post_connect(vc);*/
	update_request(sreq, iov, n_iov, 0, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
	mpi_errno = MPIDI_CH3I_VC_post_connect(vc);
	if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }
    else if (vcch->state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIDI_DBG_PRINTF((55, FCNAME, "still connecting.  enqueuing request"));
	update_request(sreq, iov, n_iov, 0, 0);
	MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    else
    {
	/* Connection failed.  Mark the request complete and return an error. */
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
	/* MT - CH3U_Request_complete performs write barrier */
	MPIDI_CH3U_Request_complete(sreq);
    }

 fn_fail:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
    return mpi_errno;
}

