/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Cancel_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Cancel_recv(MPID_Request * rreq)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_CANCEL_RECV);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CANCEL_RECV);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    assert(rreq->kind == MPID_REQUEST_RECV);
    
    if (MPIDI_CH3U_Recvq_DP(rreq))
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request 0x%08x cancelled", rreq->handle));
	rreq->status.cancelled = TRUE;
	rreq->status.count = 0;
	MPID_Request_set_completed(rreq);
	MPID_Request_release(rreq);
    }
    else
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "request 0x%08x already matched, unable to cancel", rreq->handle));
    }

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CANCEL_RECV);
    return MPI_SUCCESS;
}
