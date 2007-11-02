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
    
    MPIU_Assert(rreq->kind == MPID_REQUEST_RECV);
    
    if (MPIDI_CH3U_Recvq_DP(rreq))
    {
	MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
		       "request 0x%08x cancelled", rreq->handle);
	rreq->status.cancelled = TRUE;
	rreq->status.count = 0;
	MPID_REQUEST_SET_COMPLETED(rreq);
	MPID_Request_release(rreq);
    }
    else
    {
	MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
	    "request 0x%08x already matched, unable to cancel", rreq->handle);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CANCEL_RECV);
    return MPI_SUCCESS;
}
