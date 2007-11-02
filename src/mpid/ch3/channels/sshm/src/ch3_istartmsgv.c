/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* STATES:NO WARNINGS */

/*static MPID_Request * create_request(MPID_IOV * iov, int count, int offset, int nb)*/
#undef create_request
#define create_request(sreq, iov, count, offset, nb) \
{ \
    /*MPID_Request * sreq;*/ \
    /*int i;*/ \
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST); \
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY); \
    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST); \
    sreq = MPID_Request_create(); \
    if (sreq == NULL) \
    { \
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0); \
	MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST); \
	goto fn_exit; \
    } \
    MPIU_Object_set_ref(sreq, 2); \
    sreq->kind = MPID_REQUEST_SEND; \
    MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY); \
    memcpy(sreq->dev.iov, iov, count * sizeof(MPID_IOV)); \
    MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY); \
    /*for (i = 0; i < count; i++) { sreq->dev.iov[i] = iov[i]; }*/ \
    if (offset == 0) \
    { \
	/* memcpy(&sreq->ch.pkt, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN); */ \
	/*MPIU_Assert(iov[0].MPID_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t));*/ \
	sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF; \
	sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &sreq->ch.pkt; \
    } \
    sreq->dev.iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) sreq->dev.iov[offset].MPID_IOV_BUF + nb); \
    sreq->dev.iov[offset].MPID_IOV_LEN -= nb; \
    sreq->ch.iov_offset = offset; \
    sreq->dev.iov_count = count; \
    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE; \
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST); \
    /*return sreq;*/ \
}

/*
 * MPIDI_CH3_iStartMsgv() attempts to send the message immediately.  If the entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the iovec and the first buffer pointed to by the iovec (which is assumed to be a
 * MPIDI_CH3_Pkt_t) are copied into the request, and a pointer to the request is returned.  An error condition also results in a
 * request be allocated and the errror being returned in the status field of the request.
 */

/* XXX - What do we do if MPID_Request_create() returns NULL???  If MPIDI_CH3_iStartMsgv() returns NULL, the calling code
   assumes the request completely successfully, but the reality is that we couldn't allocate the memory for a request.  This
   seems like a flaw in the CH3 API. */

/* NOTE - The completion action associated with a request created by CH3_iStartMsgv() is alway MPIDI_CH3_CA_COMPLETE.  This
   implies that CH3_iStartMsgv() can only be used when the entire message can be described by a single iovec of size
   MPID_IOV_LIMIT. */
    
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsgv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsgv(MPIDI_VC_t * vc, MPID_IOV * iov, int n_iov, MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#ifdef MPICH_DBG_OUTPUT
    if (n_iov > MPID_IOV_LIMIT)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	goto fn_exit;
    }
    if (iov[0].MPID_IOV_LEN > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	goto fn_exit;
    }
#endif

    /* The SOCK channel uses a fixed length header, the size of which is the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF);

    if (vc->ch.state == MPIDI_CH3I_VC_STATE_CONNECTED) /* MT */
    {
	/* Connection already formed.  If send queue is empty attempt to send data, queuing any unsent data. */
	if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
	{
	    int nb;

	    MPIDI_DBG_PRINTF((55, FCNAME, "send queue empty, attempting to write"));

	    /* MT - need some signalling to lock down our right to use the channel, thus insuring that the progress engine does
               also try to write */
	    mpi_errno = MPIDI_CH3I_SHM_writev(vc, iov, n_iov, &nb);
	    if (mpi_errno == MPI_SUCCESS)
	    {
		int offset = 0;
    
		MPIDI_DBG_PRINTF((55, FCNAME, "wrote %d bytes", nb));
		
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
			create_request(sreq, iov, n_iov, offset, nb);
			/*sreq = create_request(iov, n_iov, offset, nb);
			if (sreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			    return mpi_errno;
			}
			*/
			MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
			vc->ch.send_active = sreq;
			break;
		    }
		}

		if (offset == n_iov)
		{
		    MPIDI_DBG_PRINTF((55, FCNAME, "entire write complete"));
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - MPIDI_CH3I_SHM_writev failed"));
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
	    create_request(sreq, iov, n_iov, 0, 0);
	    /*
	    sreq = create_request(iov, n_iov, 0, 0);
	    if (sreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		return mpi_errno;
	    }
	    */
	    MPIDI_CH3I_SendQ_enqueue(vc, sreq);
	}
    }
    else if (vc->ch.state == MPIDI_CH3I_VC_STATE_UNCONNECTED)
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "unconnected.  posting connect and enqueuing request"));
	
	/* Form a new connection */
	/*MPIDI_CH3I_VC_post_connect(vc);*/
	
	/* queue the data so it can be sent after the connection is formed */
	create_request(sreq, iov, n_iov, 0, 0);
	/*
	sreq = create_request(iov, n_iov, 0, 0);
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
	*/
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);

	MPIDI_CH3I_VC_post_connect(vc);
    }
    else if (vc->ch.state != MPIDI_CH3I_VC_STATE_FAILED)
    {
	/* Unable to send data at the moment, so queue it for later */
	MPIDI_DBG_PRINTF((55, FCNAME, "forming connection, request enqueued"));
	create_request(sreq, iov, n_iov, 0, 0);
	/*
	sreq = create_request(iov, n_iov, 0, 0);
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    return mpi_errno;
	}
	*/
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);
    }
    else
    {
	/* Connection failed, so allocate a request and return an error. */
	MPIDI_DBG_PRINTF((55, FCNAME, "ERROR - connection failed"));
	sreq = MPID_Request_create();
	if (sreq == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    goto fn_exit;
	}
	sreq->kind = MPID_REQUEST_SEND;
	sreq->cc = 0;
	/* TODO: Create an appropriate error message */
	sreq->status.MPI_ERROR = MPI_ERR_INTERN;
    }

fn_exit:
    *sreq_ptr = sreq;

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    return mpi_errno;
}
