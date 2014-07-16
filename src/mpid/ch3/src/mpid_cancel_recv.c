/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_CANCEL_RECV);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CANCEL_RECV);
    
    MPIU_Assert(rreq->kind == MPID_REQUEST_RECV);
    
    if (MPIDI_CH3U_Recvq_DP(rreq))
    {
        /* code in cancel_recv */
        /* FIXME: The vc is only needed to find which function to call*/
        /* This is otherwise any_source ready */
#ifdef ENABLE_COMM_OVERRIDES
        {
            MPIDI_VC_t *vc;
            MPIU_Assert(rreq->dev.match.parts.rank != MPI_ANY_SOURCE);
            MPIDI_Comm_get_vc_set_active(rreq->comm, rreq->dev.match.parts.rank, &vc);
            if (vc->comm_ops && vc->comm_ops->cancel_recv)
            {
                mpi_errno = vc->comm_ops->cancel_recv(NULL, rreq);
                if (mpi_errno)
                    goto fn_exit;
             }
        }
#endif
    MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
		       "request 0x%08x cancelled", rreq->handle);
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
	MPID_REQUEST_SET_COMPLETED(rreq);
	MPID_Request_release(rreq);
    }
    else
    {
	MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
	    "request 0x%08x already matched, unable to cancel", rreq->handle);
    }

    fn_fail:
    fn_exit:
       MPIDI_FUNC_EXIT(MPID_STATE_MPID_CANCEL_RECV);
       return mpi_errno;
}
