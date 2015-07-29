/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Req_handler_rma_op_complete
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3_Req_handler_rma_op_complete(MPID_Request * sreq)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPID_Request *ureq = NULL;
    MPIDI_RMA_Op_t *op = NULL;
    MPID_Win *win_ptr = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_OP_COMPLETE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_OP_COMPLETE);

    /* get window, decrement active request cnt on window */
    MPID_Win_get_ptr(sreq->dev.source_win_handle, win_ptr);
    MPIU_Assert(win_ptr != NULL);
    win_ptr->active_req_cnt--;

    if (sreq->dev.request_handle != MPI_REQUEST_NULL) {
        /* get user request */
        MPID_Request_get_ptr(sreq->dev.request_handle, ureq);
        mpi_errno = MPID_Request_complete(ureq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }
    }

    op = sreq->dev.rma_op_ptr;
    /* Note: if rma_op_ptr is NULL, it means the operation
     * is freed before the completion. */
    if (op != NULL) {
        /* Mark this request as NULL in the operation struct,
         * so that when operation is freed before completion,
         * the operation do not need to try to notify this
         * request which is already completed. */
        if (op->reqs_size == 1) {
            MPIU_Assert(op->single_req->handle == sreq->handle);
            op->single_req = NULL;
        }
        else {
            for (i = 0; i < op->reqs_size; i++) {
                if (op->multi_reqs[i] == NULL)
                    continue;
                else if (op->multi_reqs[i]->handle == sreq->handle) {
                    op->multi_reqs[i] = NULL;
                    break;
                }
            }
        }

        op->ref_cnt--;
        MPIU_Assert(op->ref_cnt >= 0);
        if (op->ref_cnt == 0) {
            MPIDI_RMA_Target_t *target = NULL;
            MPIDI_RMA_Op_t **list_ptr = NULL;

            mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, op->target_rank, &target);
            if (mpi_errno != MPI_SUCCESS) {
                MPIU_ERR_POP(mpi_errno);
            }
            MPIU_Assert(target != NULL);

            MPIDI_CH3I_RMA_Get_issued_list_ptr(target, op, list_ptr, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
                MPIU_ERR_POP(mpi_errno);
            }

            MPIDI_CH3I_RMA_Ops_free_elem(win_ptr, list_ptr, op);
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQ_HANDLER_RMA_OP_COMPLETE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
