/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/* STATES:NO WARNINGS */

/*static MPID_Request * create_request(void * pkt, int pkt_sz, int nb)*/
#undef create_request
#define create_request(sreq, pkt, pkt_sz, nb) \
{ \
    MPIDI_STATE_DECL(MPID_STATE_CREATE_REQUEST); \
    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_REQUEST); \
    sreq = MPID_Request_create(); \
    /*MPIU_Assert(sreq != NULL);*/ \
    if (sreq == NULL) \
    { \
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0); \
	MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST); \
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG); \
	return mpi_errno; \
    } \
    MPIU_Object_set_ref(sreq, 2); \
    sreq->kind = MPID_REQUEST_SEND; \
    MPIU_Assert(pkt_sz == sizeof(MPIDI_CH3_Pkt_t)); \
    sreq->ch.pkt = *(MPIDI_CH3_Pkt_t *) pkt; \
    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) &sreq->ch.pkt + nb); \
    sreq->dev.iov[0].MPID_IOV_LEN = pkt_sz - nb; \
    sreq->dev.iov_count = 1; \
    sreq->ch.iov_offset = 0; \
    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE; \
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_REQUEST); \
}

/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If the
 * entire message is successfully sent, then NULL is returned.  Otherwise a
 * request is allocated, the header is copied into the request, and a pointer
 * to the request is returned.  An error condition also results in a request be
 * allocated and the errror being returned in the status field of the
 * request.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void * pkt, MPIDI_msg_sz_t pkt_sz, MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSG);

    MPIU_DBG_PRINTF(("ch3_istartmsg\n"));
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#ifdef MPICH_DBG_OUTPUT
    if (pkt_sz > sizeof(MPIDI_CH3_Pkt_t))
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
	return mpi_errno;
    }
#endif

    /* The IB implementation uses a fixed length header, the size of which is
       the maximum of all possible packet headers */
    pkt_sz = sizeof(MPIDI_CH3_Pkt_t);
    
    /* Connection already formed.  If send queue is empty attempt to send
       data, queuing any unsent data. */
    if (MPIDI_CH3I_SendQ_empty(vc)) /* MT */
    {
	int nb;
	
	/* MT - need some signalling to lock down our right to use the
	   channel, thus insuring that the progress engine does also try to
	   write */
	
	mpi_errno = ibu_write(vc->ch.ibu, pkt, pkt_sz, &nb);
	if (mpi_errno == MPI_SUCCESS)
	{
	    if (nb == pkt_sz)
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "data sent immediately"));
		/* done.  get us out of here as quickly as possible. */
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((55, FCNAME, "send delayed, request enqueued"));
		create_request(sreq, pkt, pkt_sz, nb);
		MPIDI_CH3I_SendQ_enqueue_head(vc, sreq);
		vc->ch.send_active = sreq;
	    }
	}
	else
	{
	    sreq = MPID_Request_create();
	    if (sreq == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
		return mpi_errno;
	    }
	    sreq->kind = MPID_REQUEST_SEND;
	    sreq->cc = 0;
	    /* TODO: Create an appropriate error message based on the return value */
	    sreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ibwrite", 0);
	}
    }
    else
    {
	MPIDI_DBG_PRINTF((55, FCNAME, "send in progress, request enqueued"));
	create_request(sreq, pkt, pkt_sz, 0);
	MPIDI_CH3I_SendQ_enqueue(vc, sreq);
    }

    *sreq_ptr = sreq;

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSG);
    return mpi_errno;
}
