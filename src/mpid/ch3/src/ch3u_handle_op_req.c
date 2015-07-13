/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

/* Note the the following function is called when request-based RMA operation
   is completed at origin side. Here we complete the user request associated
   with this request-based operation. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_ReqOpsComplete
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_ReqOpsComplete(MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *ureq = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_REQOPSCOMPLETE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_REQOPSCOMPLETE);

    MPID_Request_get_ptr(sreq->dev.request_handle, ureq);
    MPIU_Assert(ureq != NULL);

    /* Complete user request and release ref of completion handler.
     * Note that ch3 ref is released by later clean_up call. */
    mpi_errno = MPID_Request_complete(ureq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_REQOPSCOMPLETE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Req_handler_rma_sync_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Req_handler_rma_sync_complete(MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_SYNC_COMPLETE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_SYNC_COMPLETE);

    MPID_Win_get_ptr(sreq->dev.source_win_handle, win_ptr);
    MPIU_Assert(win_ptr != NULL);

    /* decrement pending incomplete ibarrier request counter */
    win_ptr->dangling_request_cnt--;
    MPIU_Assert(win_ptr->dangling_request_cnt >= 0);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_SYNC_COMPLETE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
