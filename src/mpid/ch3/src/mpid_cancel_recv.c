/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Cancel_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Cancel_recv(MPIR_Request * rreq)
{
    int netmod_cancelled = TRUE;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CANCEL_RECV);
    
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CANCEL_RECV);
    
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__RECV);
    
    /* If the netmod has its own cancel_recv function, we need to call
       it here. ANYSOURCE cancels (netmod and otherwise) are handled by
       MPIDI_CH3U_Recvq_DP below. */
#ifdef ENABLE_COMM_OVERRIDES
    if (rreq->dev.match.parts.rank != MPI_ANY_SOURCE)
    {
        MPIDI_VC_t *vc;
        MPIDI_Comm_get_vc_set_active(rreq->comm, rreq->dev.match.parts.rank, &vc);
        if (vc->comm_ops && vc->comm_ops->cancel_recv)
            netmod_cancelled = !vc->comm_ops->cancel_recv(NULL, rreq);
    }
#endif

    if (netmod_cancelled && MPIDI_CH3U_Recvq_DP(rreq))
    {
	MPL_DBG_MSG_P(MPIDI_CH3_DBG_OTHER,VERBOSE,
		       "request 0x%08x cancelled", rreq->handle);
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
        mpi_errno = MPID_Request_complete(rreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else
    {
	MPL_DBG_MSG_P(MPIDI_CH3_DBG_OTHER,VERBOSE,
	    "request 0x%08x already matched, unable to cancel", rreq->handle);
    }

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CANCEL_RECV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
