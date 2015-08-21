/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_OPLIST_H_INCLUDED)
#define MPID_RMA_OPLIST_H_INCLUDED

#include "mpl_utlist.h"
#include "mpid_rma_types.h"

int MPIDI_CH3I_RMA_Cleanup_ops_aggressive(MPID_Win * win_ptr);
int MPIDI_CH3I_RMA_Cleanup_target_aggressive(MPID_Win * win_ptr, MPIDI_RMA_Target_t ** target);
int MPIDI_CH3I_RMA_Make_progress_target(MPID_Win * win_ptr, int target_rank, int *made_progress);
int MPIDI_CH3I_RMA_Make_progress_win(MPID_Win * win_ptr, int *made_progress);

extern MPIDI_RMA_Op_t *global_rma_op_pool_head, *global_rma_op_pool_start;
extern MPIDI_RMA_Target_t *global_rma_target_pool_head, *global_rma_target_pool_start;

MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_rmaqueue_alloc);

/* This macro returns two flags: local_completed and remote_completed,
 * to indicate if the completion is reached on this target. */
#define MPIDI_CH3I_RMA_ops_completion(win_, target_, local_completed_, remote_completed_) \
    do {                                                                \
        local_completed_ = 0;                                           \
        remote_completed_ = 0;                                          \
        if ((win_)->states.access_state != MPIDI_RMA_FENCE_ISSUED &&    \
            (win_)->states.access_state != MPIDI_RMA_PSCW_ISSUED &&     \
            (win_)->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED && \
            (target_)->access_state != MPIDI_RMA_LOCK_CALLED &&         \
            (target_)->access_state != MPIDI_RMA_LOCK_ISSUED &&         \
            (target_)->pending_net_ops_list_head == NULL &&             \
            (target_)->pending_user_ops_list_head == NULL &&            \
            (target_)->num_pkts_wait_for_local_completion == 0) {       \
            local_completed_ = 1;                                       \
            if ((target_)->sync.sync_flag == MPIDI_RMA_SYNC_NONE &&     \
                (target_)->num_ops_flush_not_issued == 0 &&             \
                (target_)->sync.outstanding_acks == 0)                  \
                remote_completed_ = 1;                                  \
        }                                                               \
    } while (0)


/* This macro returns a flag: win_remote_completed, to indicate if
 * the remote completion is reached on the entire window. */
#define MPIDI_CH3I_RMA_ops_win_remote_completion(win_ptr_, win_remote_completed_) \
    do {                                                                \
        MPIDI_RMA_Target_t *win_target_ = NULL;                         \
        int i_, num_targets_ = 0;                                       \
        int remote_completed_targets_ = 0;                              \
                                                                        \
        win_remote_completed_ = 0;                                      \
                                                                        \
        for (i_ = 0; i_ < (win_ptr_)->num_slots; i_++) {                \
            for (win_target_ = (win_ptr_)->slots[i_].target_list_head; win_target_;) { \
                int local_ ATTRIBUTE((unused)) = 0, remote_ = 0;        \
                                                                        \
                num_targets_++;                                         \
                                                                        \
                MPIDI_CH3I_RMA_ops_completion((win_ptr_), win_target_, local_, remote_); \
                                                                        \
                remote_completed_targets_ += remote_;                   \
                                                                        \
                win_target_ = win_target_->next;                        \
            }                                                           \
        }                                                               \
                                                                        \
        if (num_targets_ == remote_completed_targets_)                  \
            win_remote_completed_ = 1;                                  \
                                                                        \
    } while (0)

/* This macro returns a flag: win_local_completed, to indicate if
 * the local completion is reached on the entire window. */
#define MPIDI_CH3I_RMA_ops_win_local_completion(win_ptr_, win_local_completed_) \
    do {                                                                \
        MPIDI_RMA_Target_t *win_target_ = NULL;                         \
        int i_, total_remote_cnt_ = 0, total_local_cnt_ = 0;            \
        int remote_completed_targets_ = 0, local_completed_targets_ = 0; \
                                                                        \
        win_local_completed_ = 0;                                       \
                                                                        \
        for (i_ = 0; i_ < (win_ptr_)->num_slots; i_++) {                \
            for (win_target_ = (win_ptr_)->slots[i_].target_list_head; win_target_;) { \
                int local_ = 0, remote_ ATTRIBUTE((unused)) = 0;        \
                                                                        \
                total_local_cnt_++;                                     \
                                                                        \
                MPIDI_CH3I_RMA_ops_completion((win_ptr_), win_target_, local_, remote_); \
                                                                        \
                local_completed_targets_ += local_;                     \
                                                                        \
                win_target_ = win_target_->next;                        \
            }                                                           \
        }                                                               \
                                                                        \
        if (remote_completed_targets_ == total_remote_cnt_ &&           \
            local_completed_targets_ == total_local_cnt_)               \
            win_local_completed_ = 1;                                   \
                                                                        \
    } while (0)


/* Given a rank, return slot index */
#define MPIDI_CH3I_RMA_RANK_TO_SLOT(win_ptr_, rank_)                    \
    (((win_ptr_)->num_slots < (win_ptr_)->comm_ptr->local_size) ?       \
     &(win_ptr_)->slots[(rank_) % (win_ptr_)->num_slots] :              \
     &(win_ptr_)->slots[(rank_)])

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_set_active
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_set_active(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (win_ptr->active == FALSE) {
        win_ptr->active = TRUE;

        if (MPIDI_RMA_Win_active_list_head == NULL) {
            /* This is the first active window, activate RMA progress */
            MPID_Progress_activate_hook(MPIDI_CH3I_RMA_Progress_hook_id);
        }

        MPL_DL_DELETE(MPIDI_RMA_Win_inactive_list_head, win_ptr);
        MPL_DL_APPEND(MPIDI_RMA_Win_active_list_head, win_ptr);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_set_inactive
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_set_inactive(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (win_ptr->active == TRUE) {
        win_ptr->active = FALSE;
        MPL_DL_DELETE(MPIDI_RMA_Win_active_list_head, win_ptr);
        MPL_DL_APPEND(MPIDI_RMA_Win_inactive_list_head, win_ptr);

        if (MPIDI_RMA_Win_active_list_head == NULL) {
            /* This is the last active window, de-activate RMA progress */
            MPID_Progress_deactivate_hook(MPIDI_CH3I_RMA_Progress_hook_id);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* MPIDI_CH3I_Win_op_alloc(): get a new op element from op pool and
 * initialize it. If we cannot get one, return NULL. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_op_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline MPIDI_RMA_Op_t *MPIDI_CH3I_Win_op_alloc(MPID_Win * win_ptr)
{
    MPIDI_RMA_Op_t *e;

    if (win_ptr->op_pool_head == NULL) {
        /* local pool is empty, try to find something in the global pool */
        if (global_rma_op_pool_head == NULL)
            return NULL;
        else {
            e = global_rma_op_pool_head;
            MPL_DL_DELETE(global_rma_op_pool_head, e);
        }
    }
    else {
        e = win_ptr->op_pool_head;
        MPL_DL_DELETE(win_ptr->op_pool_head, e);
    }

    e->single_req = NULL;
    e->multi_reqs = NULL;
    e->reqs_size = 0;
    e->ureq = NULL;
    e->piggyback_lock_candidate = 0;
    e->issued_stream_count = 0;

    e->origin_datatype = MPI_DATATYPE_NULL;
    e->result_datatype = MPI_DATATYPE_NULL;

    return e;
}

/* MPIDI_CH3I_Win_op_free(): put an op element back to the op pool which
 * it belongs to. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_op_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_op_free(MPID_Win * win_ptr, MPIDI_RMA_Op_t * e)
{
    int mpi_errno = MPI_SUCCESS;

    if (e->multi_reqs != NULL) {
        MPIU_Free(e->multi_reqs);
    }

    /* We enqueue elements to the right pool, so when they get freed
     * at window free time, they won't conflict with the global pool
     * or other windows */
    /* use PREPEND when return objects back to the pool
     * in order to improve cache performance */
    if (e->pool_type == MPIDI_RMA_POOL_WIN)
        MPL_DL_PREPEND(win_ptr->op_pool_head, e);
    else
        MPL_DL_PREPEND(global_rma_op_pool_head, e);

    return mpi_errno;
}

/* MPIDI_CH3I_Win_target_alloc(): get a target element from the target pool.
 * If we cannot get one, return NULL. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_target_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline MPIDI_RMA_Target_t *MPIDI_CH3I_Win_target_alloc(MPID_Win * win_ptr)
{
    MPIDI_RMA_Target_t *e;

    if (win_ptr->target_pool_head == NULL) {
        /* local pool is empty, try to find something in the global pool */
        if (global_rma_target_pool_head == NULL)
            return NULL;
        else {
            e = global_rma_target_pool_head;
            MPL_DL_DELETE(global_rma_target_pool_head, e);
        }
    }
    else {
        e = win_ptr->target_pool_head;
        MPL_DL_DELETE(win_ptr->target_pool_head, e);
    }

    e->pending_net_ops_list_head = NULL;
    e->pending_user_ops_list_head = NULL;
    e->next_op_to_issue = NULL;

    e->target_rank = -1;
    e->access_state = MPIDI_RMA_NONE;
    e->lock_type = MPID_LOCK_NONE;
    e->lock_mode = 0;
    e->win_complete_flag = 0;

    e->sync.sync_flag = MPIDI_RMA_SYNC_NONE;
    e->sync.outstanding_acks = 0;

    e->num_pkts_wait_for_local_completion = 0;
    e->num_ops_flush_not_issued = 0;

    return e;
}

/* MPIDI_CH3I_Win_target_free(): put a target element back to the target pool
 * it belongs to. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_target_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_target_free(MPID_Win * win_ptr, MPIDI_RMA_Target_t * e)
{
    int mpi_errno = MPI_SUCCESS;

    /* We enqueue elements to the right pool, so when they get freed
     * at window free time, they won't conflict with the global pool
     * or other windows */
    MPIU_Assert(e->pending_net_ops_list_head == NULL);
    MPIU_Assert(e->pending_user_ops_list_head == NULL);

    /* use PREPEND when return objects back to the pool
     * in order to improve cache performance */
    if (e->pool_type == MPIDI_RMA_POOL_WIN)
        MPL_DL_PREPEND(win_ptr->target_pool_head, e);
    else
        MPL_DL_PREPEND(global_rma_target_pool_head, e);

    return mpi_errno;
}

/* MPIDI_CH3I_Win_create_target(): given a rank, create
 * corresponding target in RMA slots. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_create_target
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_create_target(MPID_Win * win_ptr, int target_rank,
                                               MPIDI_RMA_Target_t ** e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Slot_t *slot = NULL;
    MPIDI_RMA_Target_t *t = NULL;

    slot = MPIDI_CH3I_RMA_RANK_TO_SLOT(win_ptr, target_rank);
    t = MPIDI_CH3I_Win_target_alloc(win_ptr);
    if (t == NULL) {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_target_aggressive(win_ptr, &t);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    t->target_rank = target_rank;

    /* Enqueue target into target list. */
    MPL_DL_APPEND(slot->target_list_head, t);

    assert(t != NULL);

    (*e) = t;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDI_CH3I_Win_find_target(): given a rank, find
 * corresponding target in RMA slots. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_find_target
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_find_target(MPID_Win * win_ptr, int target_rank,
                                             MPIDI_RMA_Target_t ** e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Slot_t *slot = NULL;
    MPIDI_RMA_Target_t *t = NULL;

    slot = MPIDI_CH3I_RMA_RANK_TO_SLOT(win_ptr, target_rank);

    t = slot->target_list_head;
    while (t != NULL) {
        if (t->target_rank == target_rank)
            break;
        t = t->next;
    }

    (*e) = t;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDI_CH3I_Win_enqueue_op(): given an operation, enqueue it to the
 * corresponding operation lists in corresponding target element. This
 * routines is only called from operation routines. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_enqueue_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_enqueue_op(MPID_Win * win_ptr, MPIDI_RMA_Op_t * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Target_t *target = NULL;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, op->target_rank, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    if (target == NULL) {
        mpi_errno = MPIDI_CH3I_Win_create_target(win_ptr, op->target_rank, &target);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
            win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_GRANTED) {
            /* If global state is MPIDI_RMA_PER_TARGET, this must not
             * be the first time to create this target (The first time
             * is in Win_lock). Here we recreated it and set the access
             * state to LOCK_GRANTED because before we free the previous
             * one, the lock should already be granted. */
            /* If global state is MPIDI_RMA_LOCK_ALL_GRANTED, all locks
             * should already be granted. So the access state for this
             * target is also set to MPIDI_RMA_LOCK_GRANTED. */
            target->access_state = MPIDI_RMA_LOCK_GRANTED;
        }
        else if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
            /* If global state is MPIDI_RMA_LOCK_ALL_CALLED, this must
             * the first time to create this target, set its access state
             * to MPIDI_RMA_LOCK_CALLED. */
            target->access_state = MPIDI_RMA_LOCK_CALLED;
            target->lock_type = MPI_LOCK_SHARED;
        }
    }

    /* Note that if it is a request-based RMA, do not put it in pending user list,
     * otherwise a wait call before unlock will be blocked. */
    if (MPIR_CVAR_CH3_RMA_DELAY_ISSUING_FOR_PIGGYBACKING && op->ureq == NULL) {
        if (target->pending_user_ops_list_head != NULL) {
            MPIDI_RMA_Op_t *user_op = target->pending_user_ops_list_head;
            /* Move head element of user pending list to net pending list */
            if (target->pending_net_ops_list_head == NULL)
                win_ptr->num_targets_with_pending_net_ops++;
            MPL_DL_DELETE(target->pending_user_ops_list_head, user_op);
            MPL_DL_APPEND(target->pending_net_ops_list_head, user_op);

            if (target->next_op_to_issue == NULL)
                target->next_op_to_issue = user_op;
        }

        /* Enqueue operation into user pending list. */
        MPL_DL_APPEND(target->pending_user_ops_list_head, op);
    }
    else {
        /* Enqueue operation into net pending list. */
        if (target->pending_net_ops_list_head == NULL)
            win_ptr->num_targets_with_pending_net_ops++;
        MPL_DL_APPEND(target->pending_net_ops_list_head, op);

        if (target->next_op_to_issue == NULL)
            target->next_op_to_issue = op;
    }

    if (target->pending_net_ops_list_head != NULL &&
        (win_ptr->states.access_state == MPIDI_RMA_FENCE_GRANTED ||
         win_ptr->states.access_state == MPIDI_RMA_PSCW_GRANTED ||
         win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_GRANTED ||
         target->access_state == MPIDI_RMA_LOCK_GRANTED))
        MPIDI_CH3I_Win_set_active(win_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* MPIDI_CH3I_Win_target_dequeue_and_free(): dequeue and free
 * the target in RMA slots. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_target_dequeue_and_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_target_dequeue_and_free(MPID_Win * win_ptr, MPIDI_RMA_Target_t * e)
{
    int mpi_errno = MPI_SUCCESS;
    int target_rank = e->target_rank;
    MPIDI_RMA_Slot_t *slot;

    slot = MPIDI_CH3I_RMA_RANK_TO_SLOT(win_ptr, target_rank);

    MPL_DL_DELETE(slot->target_list_head, e);

    mpi_errno = MPIDI_CH3I_Win_target_free(win_ptr, e);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_targets_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Cleanup_targets_win(MPID_Win * win_ptr)
{
    MPIDI_RMA_Target_t *target = NULL, *next_target = NULL;
    int i, mpi_errno = MPI_SUCCESS;

    for (i = 0; i < win_ptr->num_slots; i++) {
        for (target = win_ptr->slots[i].target_list_head; target;) {
            next_target = target->next;
            mpi_errno = MPIDI_CH3I_Win_target_dequeue_and_free(win_ptr, target);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            target = next_target;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_get_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_get_op(MPID_Win * win_ptr, MPIDI_RMA_Op_t ** e)
{
    MPIDI_RMA_Op_t *new_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    while (1) {
        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
        if (new_ptr != NULL)
            break;

        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    (*e) = new_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Free an element in the RMA operations list.
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param IN    curr_ptr  Pointer to the element to be freed.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_free_elem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_free_elem(MPID_Win * win_ptr, MPIDI_RMA_Ops_list_t * list,
                                                MPIDI_RMA_Op_t * curr_ptr)
{
    MPIDI_RMA_Op_t *tmp_ptr = curr_ptr;

    MPIU_Assert(curr_ptr != NULL);

    MPL_DL_DELETE(*list, curr_ptr);

    MPIDI_CH3I_Win_op_free(win_ptr, tmp_ptr);
}


#endif /* MPID_RMA_OPLIST_H_INCLUDED */
