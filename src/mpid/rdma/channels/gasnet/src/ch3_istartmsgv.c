/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

extern void *MPIDI_CH3_packet_buffer;

/*
 * MPIDI_CH3_iStartMsgv() attempts to send the message immediately.
 * If the entire message is successfully sent, then NULL is returned.
 * Otherwise a request is allocated, the iovec and the first buffer
 * pointed to by the iovec (which is assumed to be a MPIDI_CH3_Pkt_t)
 * are copied into the request, and a pointer to the request is
 * returned.  An error condition also results in a request be
 * allocated and the errror being returned in the status field of the
 * request.
 */

/* XXX - What do we do if MPIDI_CH3_Request_create() returns NULL???
   If MPIDI_CH3_iStartMsgv() returns NULL, the calling code assumes
   the request completely successfully, but the reality is that we
   couldn't allocate the memory for a request.  This seems like a flaw
   in the CH3 API. */

/* NOTE - The completion action associated with a request created by CH3_iStartMsgv() is alway MPIDI_CH3_CA_COMPLETE.  This
   implies that CH3_iStartMsgv() can only be used when the entire message can be described by a single iovec of size
   MPID_IOV_LIMIT. */
    
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iStartMsgv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iStartMsgv(MPIDI_VC * vc, MPID_IOV * iov, int n_iov,
			 MPID_Request ** sreq_ptr)
{
    MPID_Request * sreq = NULL;
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int msg_sz;
    int i, j;
    MPID_IOV tmp_iov;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISTARTMSGV);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    printf_d ("Entering "FCNAME"\n");
    assert(n_iov <= MPID_IOV_LIMIT);
    assert(iov[0].MPID_IOV_LEN <= sizeof(MPIDI_CH3_Pkt_t));

    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)iov[0].MPID_IOV_BUF);
    
    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE) && !MPIDI_CH3I_inside_handler)
        /* MT */
    {
	/* get an iov that has no more than MPIDI_CH3_packet_len of data */
	msg_sz = 0;
	for (i = 0; i < n_iov; ++i)
	{
	    if (msg_sz + iov[i].MPID_IOV_LEN > MPIDI_CH3_packet_len)
		break;
	    msg_sz += iov[i].MPID_IOV_LEN;
	}
	if (i < n_iov)
	{
	    tmp_iov = iov[i];
	    iov[i].MPID_IOV_LEN = MPIDI_CH3_packet_len - msg_sz;
	
	    printf_d ("  sending %d bytes\n", msg_sz);
	    gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
						MPIDI_CH3_start_packet_handler_id,
						iov, i+1);
	    if (gn_errno != GASNET_OK)
	    {
		MPID_Abort(NULL, MPI_SUCCESS, -1);
	    }

            /* Create a new request and save remaining portions of the
	     * iov in it. */
 	    sreq = MPIDI_CH3_Request_create();
	    assert(sreq != NULL);
	    MPIU_Object_set_ref(sreq, 2);
	    sreq->kind = MPID_REQUEST_SEND;
	    sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;

	    sreq->dev.iov[0].MPID_IOV_LEN = tmp_iov.MPID_IOV_LEN -
		iov[i].MPID_IOV_LEN;
	    sreq->dev.iov[0].MPID_IOV_BUF = tmp_iov.MPID_IOV_BUF +
		iov[i].MPID_IOV_LEN;
	    
	    for (j = 1; j < n_iov - i - 1; j++)
	    {
		sreq->dev.iov[j] = iov[j+i];
	    }
	    sreq->gasnet.iov_offset = 0;
	    sreq->dev.iov_count = n_iov - i;
	    sreq->gasnet.vc = vc;
	    MPIDI_CH3I_SendQ_enqueue_head (sreq, CH3_NORMAL_QUEUE);
	    assert (MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL);
	    MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
	}
	else
	{
	    printf_d ("  sending %d bytes\n", msg_sz);
	    gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
						MPIDI_CH3_start_packet_handler_id,
						iov, n_iov);
	    if (gn_errno != GASNET_OK)
	    {
		MPID_Abort(NULL, MPI_SUCCESS, -1);
	    }
	}
    }
    else
    {
	int i;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "request enqueued"));
	/* create a request */
	sreq = MPIDI_CH3_Request_create();
	assert(sreq != NULL);
	MPIU_Object_set_ref(sreq, 2);
	sreq->kind = MPID_REQUEST_SEND;

	sreq->gasnet.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->gasnet.pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = iov[0].MPID_IOV_LEN;

	/* copy iov */
	for (i = 1; i < n_iov; i++)
	{
	    sreq->dev.iov[i] = iov[i];
	}

	sreq->gasnet.iov_offset = 0;
	sreq->dev.iov_count = n_iov;
	sreq->dev.ca = MPIDI_CH3_CA_COMPLETE;
	sreq->gasnet.vc = vc;
	MPIDI_CH3I_SendQ_enqueue (sreq, CH3_NORMAL_QUEUE);
    }
    
    *sreq_ptr = sreq;

    printf_d ("Exiting "FCNAME"\n");
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISTARTMSGV);
    return mpi_errno;
}

