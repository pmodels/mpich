/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"


int MPIDI_CH3_Req_handler_rma_op_complete(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *ureq = NULL;
    MPIR_Win *win_ptr = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_OP_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_OP_COMPLETE);

    if (sreq->dev.rma_target_ptr != NULL) {
        (sreq->dev.rma_target_ptr)->num_pkts_wait_for_local_completion--;
    }

    /* get window, decrement active request cnt on window */
    MPIR_Win_get_ptr(sreq->dev.source_win_handle, win_ptr);
    MPIR_Assert(win_ptr != NULL);
    MPIDI_CH3I_RMA_Active_req_cnt--;
    MPIR_Assert(MPIDI_CH3I_RMA_Active_req_cnt >= 0);

    if (sreq->dev.request_handle != MPI_REQUEST_NULL) {
        /* get user request */
        MPIR_Request_get_ptr(sreq->dev.request_handle, ureq);
        mpi_errno = MPID_Request_complete(ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_OP_COMPLETE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
