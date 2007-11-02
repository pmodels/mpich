/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

extern void *MPIDI_CH3_packet_buffer;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPID_Request * sreq, MPID_IOV * iov,
		     int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int msg_sz;
    int offset;
    int i, j;
    int complete;
    MPID_IOV tmp_iov;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISENDV);
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIU_Assert(n_iov <= MPID_IOV_LIMIT);
    MPIU_Assert(iov[0].MPID_IOV_LEN <= sizeof(MPIDI_CH3_Pkt_t));

    /* The channel uses a fixed length header, the size of which is
     * the maximum of all possible packet headers */
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)iov[0].MPID_IOV_BUF);

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE) &&
	!MPIDI_CH3I_inside_handler)
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
	
	    MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", msg_sz));
	    gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
						MPIDI_CH3_start_packet_handler_id,
						iov, i+1);
	    if (gn_errno != GASNET_OK)
	    {
		MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
	    }

	    sreq->dev.iov[0].MPID_IOV_LEN = tmp_iov.MPID_IOV_LEN -
		iov[i].MPID_IOV_LEN;
	    sreq->dev.iov[0].MPID_IOV_BUF = (void *)((char *) tmp_iov.MPID_IOV_BUF
						     + iov[i].MPID_IOV_LEN);
	    
	    for (j = 1; j < n_iov - i - 1; j++)
	    {
		sreq->dev.iov[j] = iov[j+i];
	    }
	    sreq->gasnet.iov_offset = 0;
	    sreq->dev.iov_count = n_iov - i;
	    sreq->gasnet.vc = vc;
	    MPIDI_CH3I_SendQ_enqueue_head (sreq, CH3_NORMAL_QUEUE);
	    MPIU_Assert (MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL);
	    MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
	}
	else
	{
	    MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", msg_sz));
	    gn_errno = gasnet_AMRequestMediumv0(vc->lpid,
						MPIDI_CH3_start_packet_handler_id,
						iov, n_iov);
	    if (gn_errno != GASNET_OK)
	    {
		MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
	    }
	    
	    MPIDI_CH3U_Handle_send_req (vc, sreq, &complete);

	    if (!complete)
	    {
		sreq->gasnet.iov_offset = 0;
		sreq->gasnet.vc = vc;
		MPIDI_CH3I_SendQ_enqueue_head (sreq, CH3_NORMAL_QUEUE);
		MPIU_Assert (MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] == NULL);
		MPIDI_CH3I_active_send[CH3_NORMAL_QUEUE] = sreq;
	    }
	}    
    }
    else
    {
	int i;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));
	
	sreq->gasnet.pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPID_IOV_BUF;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->gasnet.pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = iov[0].MPID_IOV_LEN;

	for (i = 1; i < n_iov; i++)
	{
	    sreq->dev.iov[i] = iov[i];
	}

	sreq->dev.iov_count = n_iov;
	sreq->gasnet.iov_offset = 0;
	sreq->gasnet.vc = vc;
	MPIDI_CH3I_SendQ_enqueue(sreq, CH3_NORMAL_QUEUE);

    }
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
    return mpi_errno;
}

