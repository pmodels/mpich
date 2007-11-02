/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

extern void *MPIDI_CH3_packet_buffer;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_do_rts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_do_rts(MPIDI_VC *vc, MPID_Request *sreq, MPIDI_CH3_Pkt_t *rts_pkt,
		     MPID_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int msg_sz;
    int offset;
    int i, j;
    MPID_IOV tmp_iov;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_DO_RTS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_DO_RTS);
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    printf_d ("Entering "FCNAME"\n");
    assert(n_iov <= MPID_IOV_LIMIT);

    DUMP_REQUEST(sreq);
    MPIDI_DBG_Print_packet(rts_pkt);

    sreq->gasnet.vc = vc;

    if (MPIDI_CH3I_SendQ_empty (CH3_NORMAL_QUEUE) && !MPIDI_CH3I_inside_handler) /* MT */
    {
	gn_errno = gasnet_AMRequestMedium0(vc->lpid,
					   MPIDI_CH3_start_packet_handler_id,
					   rts_pkt, sizeof(MPIDI_CH3_Pkt_t));

	if (gn_errno != GASNET_OK)
	{
	    MPID_Abort(NULL, MPI_SUCCESS, -1);
	}
    }
    else
    {
	MPID_Request *rts_sreq;
	int i;
	
	MPIDI_DBG_PRINTF((55, FCNAME, "enqueuing"));
	
	rts_sreq = MPIDI_CH3_Request_create();
	assert(rts_sreq != NULL);	
	sreq->kind = MPID_REQUEST_SEND;
	
	rts_sreq->gasnet.pkt = *rts_pkt;
	rts_sreq->dev.iov[0].MPID_IOV_BUF = (char *) &rts_sreq->gasnet.pkt;
	rts_sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
	rts_sreq->dev.iov_count = 1;
	rts_sreq->gasnet.iov_offset = 0;
	rts_sreq->gasnet.vc = vc;
	MPIDI_CH3I_SendQ_enqueue(rts_sreq, CH3_NORMAL_QUEUE);
    }

    printf_d ("Exiting "FCNAME"\n");
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_DO_RTS);
    return mpi_errno;
}

