/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_iSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_iSend(MPIDI_VC_t * vc, MPID_Request * sreq, void * hdr,
		    MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ISEND);

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* This channel uses a fixed length header, the size of which
     * is the maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t*)hdr);

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE) && !MPIDI_CH3I_inside_handler)
        /* MT */
    {
#if 0
	MPIDI_DBG_PRINTF((55, FCNAME, "Calling gasnet_AMRequestMedium0 (node = "
			  "%d, handler_id = %d (%s), hdr = %p, hdr_sz = %d)\n",
			  vc->lpid, MPIDI_CH3_start_packet_handler_id,
			  "MPIDI_CH3_start_packet_handler_id", hdr, hdr_sz));
#endif
	MPIDI_DBG_PRINTF((55, FCNAME, "  sending %d bytes\n", hdr_sz));
	gn_errno = gasnet_AMRequestMedium0 (vc->lpid,
					    MPIDI_CH3_start_packet_handler_id,
					    hdr, hdr_sz);
	if (gn_errno != GASNET_OK)
	{
	    MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
	}
	
	MPIDI_CH3U_Handle_send_req(vc, sreq, &complete);
    }
    else
    {
	int i;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));

	sreq->gasnet.pkt = *(MPIDI_CH3_Pkt_t *) hdr;
	sreq->dev.iov[0].MPID_IOV_BUF = (char *) &sreq->gasnet.pkt;
	sreq->dev.iov[0].MPID_IOV_LEN = hdr_sz;
	sreq->dev.iov_count = 1;
	sreq->gasnet.iov_offset = 0;
	sreq->gasnet.vc = vc;
	MPIDI_CH3I_SendQ_enqueue(sreq, CH3_NORMAL_QUEUE);
    }

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ISEND);
    return mpi_errno;
}

