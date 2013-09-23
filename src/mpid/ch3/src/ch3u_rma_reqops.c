
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"

/* Request-based RMA operations are implemented using generalized requests.
 * Below are the generalized request state and handlers for these operations.
 */

typedef struct {
    MPID_Request *request;
    MPID_Win     *win_ptr;
    int           target_rank;
} MPIDI_CH3I_Rma_req_state_t;


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Rma_req_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Rma_req_poll(void *state, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Rma_req_state_t *req_state = (MPIDI_CH3I_Rma_req_state_t*) state;

    MPIU_UNREFERENCED_ARG(status);

    /* Call flush to complete the operation.  Check that a passive target epoch
     * is still active first; the user could complete the request after calling
     * unlock. */
    /* FIXME: We need per-operation completion to make this more efficient. */
    if (req_state->win_ptr->targets[req_state->target_rank].remote_lock_state
        != MPIDI_CH3_WIN_LOCK_NONE)
    {
        mpi_errno = req_state->win_ptr->RMAFns.Win_flush(req_state->target_rank,
                                                         req_state->win_ptr);
    }

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    MPIR_Grequest_complete_impl(req_state->request);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Rma_req_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Rma_req_wait(int count, void **states, double timeout,
                                   MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    for (i = 0; i < count; i++) {
        /* Call poll to complete the operation */
        mpi_errno = MPIDI_CH3I_Rma_req_poll(states[i], status);
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Rma_req_query
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Rma_req_query(void *state, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_UNREFERENCED_ARG(state);

    /* All status fields, except the error code, are undefined */
    MPIR_STATUS_SET_COUNT(*status, 0);
    MPIR_STATUS_SET_CANCEL_BIT(*status, FALSE);
    status->MPI_SOURCE = MPI_UNDEFINED;
    status->MPI_TAG = MPI_UNDEFINED;

 fn_exit:
    status->MPI_ERROR = mpi_errno;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Rma_req_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Rma_req_free(void *state)
{
    MPIU_Free(state);

    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Rma_req_cancel
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Rma_req_cancel(void *state, int complete)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_UNREFERENCED_ARG(state);

    /* This operation can't be cancelled */
    if (!complete) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**rmareqcancel");
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Rput
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Rput(const void *origin_addr, int origin_count,
               MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr,
               MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIDI_VC_t *orig_vc, *target_vc;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RPUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RPUT);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL &&
                        target_rank != MPI_PROC_NULL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
                            dt_contig, data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && data_sz != 0) {
        mpi_errno = win_ptr->RMAFns.Put(origin_addr, origin_count,
                                        origin_datatype, target_rank,
                                        target_disp, target_count,
                                        target_datatype, win_ptr);

        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);

    /* If the operation is already complete, return a completed request.
     * Otherwise, generate a grequest. */
    /* FIXME: We still may need to flush or sync for shared memory windows */
    if (target_rank == MPI_PROC_NULL || target_rank == win_ptr->comm_ptr->rank ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id) || data_sz == 0)
    {
        mpi_errno = MPIR_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             req_state, &req_state->request);
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

        MPIR_Grequest_complete_impl(req_state->request);
    }
    else {
        mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             MPIDI_CH3I_Rma_req_poll,
                                             MPIDI_CH3I_Rma_req_wait,
                                             req_state, &req_state->request);

        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    *request = req_state->request;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RPUT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Rget
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Rget(void *origin_addr, int origin_count,
               MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPID_Win *win_ptr,
               MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIDI_VC_t *orig_vc, *target_vc;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RGET);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL &&
                        target_rank != MPI_PROC_NULL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
                            dt_contig, data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && data_sz != 0) {
        mpi_errno = win_ptr->RMAFns.Get(origin_addr, origin_count,
                                        origin_datatype, target_rank,
                                        target_disp, target_count,
                                        target_datatype, win_ptr);

        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);

    /* If the operation is already complete, return a completed request.
     * Otherwise, generate a grequest. */
    /* FIXME: We still may need to flush or sync for shared memory windows */
    if (target_rank == MPI_PROC_NULL || target_rank == win_ptr->comm_ptr->rank ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id) || data_sz == 0)
    {
        mpi_errno = MPIR_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             req_state, &req_state->request);
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

        MPIR_Grequest_complete_impl(req_state->request);
    }
    else {
        mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             MPIDI_CH3I_Rma_req_poll,
                                             MPIDI_CH3I_Rma_req_wait,
                                             req_state, &req_state->request);

        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    *request = req_state->request;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RGET);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Raccumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Raccumulate(const void *origin_addr, int origin_count,
                      MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                      int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win_ptr,
                      MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIDI_VC_t *orig_vc, *target_vc;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RACCUMULATE);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL &&
                        target_rank != MPI_PROC_NULL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    MPIDI_Datatype_get_info(origin_count, origin_datatype,
                            dt_contig, data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && data_sz != 0) {
        mpi_errno = win_ptr->RMAFns.Accumulate(origin_addr, origin_count,
                                               origin_datatype, target_rank,
                                               target_disp, target_count,
                                               target_datatype, op, win_ptr);
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);

    /* If the operation is already complete, return a completed request.
     * Otherwise, generate a grequest. */
    /* FIXME: We still may need to flush or sync for shared memory windows */
    if (target_rank == MPI_PROC_NULL || target_rank == win_ptr->comm_ptr->rank ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id) || data_sz == 0)
    {
        mpi_errno = MPIR_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             req_state, &req_state->request);
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

        MPIR_Grequest_complete_impl(req_state->request);
    }
    else {
        mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             MPIDI_CH3I_Rma_req_poll,
                                             MPIDI_CH3I_Rma_req_wait,
                                             req_state, &req_state->request);

        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    *request = req_state->request;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RACCUMULATE);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Rget_accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Rget_accumulate(const void *origin_addr, int origin_count,
                          MPI_Datatype origin_datatype, void *result_addr, int result_count,
                          MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                          int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win_ptr,
                          MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz, trg_data_sz;
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIDI_VC_t *orig_vc, *target_vc;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RGET_ACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RGET_ACCUMULATE);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL &&
                        target_rank != MPI_PROC_NULL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    /* Note that GACC is only a no-op if no data goes in both directions */
    MPIDI_Datatype_get_info(origin_count, origin_datatype,
                            dt_contig, data_sz, dtp, dt_true_lb);
    MPIDI_Datatype_get_info(origin_count, origin_datatype,
                            dt_contig, trg_data_sz, dtp, dt_true_lb);

    /* Enqueue or perform the RMA operation */
    if (target_rank != MPI_PROC_NULL && (data_sz != 0 || trg_data_sz != 0)) {
        mpi_errno = win_ptr->RMAFns.Get_accumulate(origin_addr, origin_count,
                                                   origin_datatype, result_addr,
                                                   result_count, result_datatype,
                                                   target_rank, target_disp,
                                                   target_count, target_datatype,
                                                   op, win_ptr);
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);

    /* If the operation is already complete, return a completed request.
     * Otherwise, generate a grequest. */
    /* FIXME: We still may need to flush or sync for shared memory windows */
    if (target_rank == MPI_PROC_NULL || target_rank == win_ptr->comm_ptr->rank ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id) ||
        (data_sz == 0 && trg_data_sz == 0))
    {
        mpi_errno = MPIR_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             req_state, &req_state->request);
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

        MPIR_Grequest_complete_impl(req_state->request);
    }
    else {
        mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                             MPIDI_CH3I_Rma_req_free,
                                             MPIDI_CH3I_Rma_req_cancel,
                                             MPIDI_CH3I_Rma_req_poll,
                                             MPIDI_CH3I_Rma_req_wait,
                                             req_state, &req_state->request);

        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    }

    *request = req_state->request;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RGET_ACCUMULATE);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

