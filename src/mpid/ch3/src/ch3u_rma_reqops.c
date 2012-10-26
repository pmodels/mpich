
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

    /* If this is a local operation, it's already complete.  Otherwise, call
     * flush to complete the operation */
    /* FIXME: We still may need to flush or sync for shared memory windows */
    if (req_state->target_rank != req_state->win_ptr->comm_ptr->rank) {
        mpi_errno = req_state->win_ptr->RMAFns.Win_flush(req_state->target_rank,
                                                         req_state->win_ptr);

        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**rmareqop");
        }
    }

    MPIR_Grequest_complete_impl(req_state->request);

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
    status->count = 0;
    status->cancelled = FALSE;
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
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RPUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RPUT);

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    /* Enqueue the RMA operation */
    mpi_errno = win_ptr->RMAFns.Put(origin_addr, origin_count,
                                    origin_datatype, target_rank,
                                    target_disp, target_count,
                                    target_datatype, win_ptr);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                         MPIDI_CH3I_Rma_req_free,
                                         MPIDI_CH3I_Rma_req_cancel,
                                         MPIDI_CH3I_Rma_req_poll,
                                         NULL, req_state, &req_state->request);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    *request = req_state->request;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RPUT);

 fn_exit:
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
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RGET);

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    /* Enqueue the RMA operation */
    mpi_errno = win_ptr->RMAFns.Get(origin_addr, origin_count,
                                    origin_datatype, target_rank,
                                    target_disp, target_count,
                                    target_datatype, win_ptr);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                         MPIDI_CH3I_Rma_req_free,
                                         MPIDI_CH3I_Rma_req_cancel,
                                         MPIDI_CH3I_Rma_req_poll,
                                         NULL, req_state, &req_state->request);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    *request = req_state->request;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RGET);

 fn_exit:
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
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RACCUMULATE);

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    /* Enqueue the RMA operation */
    mpi_errno = win_ptr->RMAFns.Accumulate(origin_addr, origin_count,
                                           origin_datatype, target_rank,
                                           target_disp, target_count,
                                           target_datatype, op, win_ptr);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                         MPIDI_CH3I_Rma_req_free,
                                         MPIDI_CH3I_Rma_req_cancel,
                                         MPIDI_CH3I_Rma_req_poll,
                                         NULL, req_state, &req_state->request);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    *request = req_state->request;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RACCUMULATE);

 fn_exit:
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
    MPIDI_CH3I_Rma_req_state_t *req_state;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_RGET_ACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_RGET_ACCUMULATE);

    MPIU_CHKPMEM_MALLOC(req_state, MPIDI_CH3I_Rma_req_state_t*,
                        sizeof(MPIDI_CH3I_Rma_req_state_t), mpi_errno,
                        "req-based RMA state");

    req_state->win_ptr = win_ptr;
    req_state->target_rank = target_rank;

    /* Enqueue the RMA operation */
    mpi_errno = win_ptr->RMAFns.Get_accumulate(origin_addr, origin_count,
                                               origin_datatype, result_addr,
                                               result_count, result_datatype,
                                               target_rank, target_disp,
                                               target_count, target_datatype,
                                               op, win_ptr);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIX_Grequest_start_impl(MPIDI_CH3I_Rma_req_query,
                                         MPIDI_CH3I_Rma_req_free,
                                         MPIDI_CH3I_Rma_req_cancel,
                                         MPIDI_CH3I_Rma_req_poll,
                                         NULL, req_state, &req_state->request);

    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    *request = req_state->request;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_RGET_ACCUMULATE);

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

