/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

extern void *MPIDI_CH3_packet_buffer;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_do_cts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_do_cts(MPIDI_VC_t *vc, MPID_Request *rreq, MPI_Request sreq_id,
		     MPID_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    int gn_errno;
    int msg_sz;
    int offset;
    int i, j;
    MPID_IOV tmp_iov;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_DO_CTS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_DO_CTS);
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
    MPIU_Assert(n_iov <= MPID_IOV_LIMIT);

    DUMP_REQUEST(rreq);
    if (!MPIDI_CH3I_inside_handler) 
    {
	gn_errno = gasnet_AMRequestMedium4(vc->lpid,
					   MPIDI_CH3_CTS_packet_handler_id,
					   iov, sizeof (MPID_IOV) * n_iov,
					   sreq_id, rreq->handle,
					   rreq->dev.segment_size, n_iov);
    }
    else
    {
	gn_errno = gasnet_AMReplyMedium4(MPIDI_CH3I_gasnet_token,
					 MPIDI_CH3_CTS_packet_handler_id,
					 iov, sizeof (MPID_IOV) * n_iov,
					 sreq_id, rreq->handle,
					 rreq->dev.segment_size, n_iov);
    }

    if (gn_errno != GASNET_OK)
    {
	MPID_Abort(NULL, MPI_SUCCESS, -1, "GASNet send failed");
    }

    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_DO_CTS);
    return mpi_errno;
}

