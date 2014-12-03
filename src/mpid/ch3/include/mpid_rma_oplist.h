/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_OPLIST_H_INCLUDED)
#define MPID_RMA_OPLIST_H_INCLUDED

#include "mpl_utlist.h"
#include "mpid_rma_types.h"

int MPIDI_CH3I_RMA_Free_ops_before_completion(MPID_Win * win_ptr);
int MPIDI_CH3I_RMA_Cleanup_ops_aggressive(MPID_Win * win_ptr);
int MPIDI_CH3I_RMA_Cleanup_target_aggressive(MPID_Win * win_ptr, MPIDI_RMA_Target_t ** target);
int MPIDI_CH3I_RMA_Make_progress_target(MPID_Win * win_ptr, int target_rank, int *made_progress);
int MPIDI_CH3I_RMA_Make_progress_win(MPID_Win * win_ptr, int *made_progress);

extern MPIDI_RMA_Op_t *global_rma_op_pool, *global_rma_op_pool_tail, *global_rma_op_pool_start;
extern MPIDI_RMA_Target_t *global_rma_target_pool, *global_rma_target_pool_tail, *global_rma_target_pool_start;

MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_rmaqueue_alloc);

/* MPIDI_CH3I_Win_op_alloc(): get a new op element from op pool and
 * initialize it. If we cannot get one, return NULL. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_op_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline MPIDI_RMA_Op_t *MPIDI_CH3I_Win_op_alloc(MPID_Win * win_ptr)
{
    MPIDI_RMA_Op_t *e;

    if (win_ptr->op_pool == NULL) {
        /* local pool is empty, try to find something in the global pool */
        if (global_rma_op_pool == NULL)
            return NULL;
        else {
            e = global_rma_op_pool;
            MPL_LL_DELETE(global_rma_op_pool, global_rma_op_pool_tail, e);
        }
    }
    else {
        e = win_ptr->op_pool;
        MPL_LL_DELETE(win_ptr->op_pool, win_ptr->op_pool_tail, e);
    }

    e->dataloop = NULL;
    e->request = NULL;
    e->is_dt = 0;
    e->piggyback_lock_candidate = 0;

    return e;
}

/* MPIDI_CH3I_Win_op_free(): put an op element back to the op pool which
 * it belongs to. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_op_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_op_free(MPID_Win * win_ptr, MPIDI_RMA_Op_t * e)
{
    int mpi_errno = MPI_SUCCESS;

    /* Check if we allocated a dataloop for this op (see send/recv_rma_msg) */
    if (e->dataloop != NULL) {
        MPIU_Free(e->dataloop);
    }

    /* We enqueue elements to the right pool, so when they get freed
     * at window free time, they won't conflict with the global pool
     * or other windows */
    /* use PREPEND when return objects back to the pool
       in order to improve cache performance */
    if (e->pool_type == MPIDI_RMA_POOL_WIN)
        MPL_LL_PREPEND(win_ptr->op_pool, win_ptr->op_pool_tail, e);
    else
        MPL_LL_PREPEND(global_rma_op_pool, global_rma_op_pool_tail, e);

    return mpi_errno;
}

/* MPIDI_CH3I_Win_target_alloc(): get a target element from the target pool.
 * If we cannot get one, return NULL. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_target_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline MPIDI_RMA_Target_t *MPIDI_CH3I_Win_target_alloc(MPID_Win * win_ptr)
{
    MPIDI_RMA_Target_t *e;

    if (win_ptr->target_pool == NULL) {
        /* local pool is empty, try to find something in the global pool */
        if (global_rma_target_pool == NULL)
            return NULL;
        else {
            e = global_rma_target_pool;
            MPL_LL_DELETE(global_rma_target_pool, global_rma_target_pool_tail, e);
        }
    }
    else {
        e = win_ptr->target_pool;
        MPL_LL_DELETE(win_ptr->target_pool, win_ptr->target_pool_tail, e);
    }

    e->read_op_list = e->read_op_list_tail = NULL;
    e->write_op_list = e->write_op_list_tail = NULL;
    e->dt_op_list = e->dt_op_list_tail = NULL;
    e->pending_op_list = e->pending_op_list_tail = NULL;
    e->next_op_to_issue = NULL;

    e->target_rank = -1;
    e->access_state = MPIDI_RMA_NONE;
    e->lock_type = MPID_LOCK_NONE;
    e->lock_mode = 0;
    e->accumulated_ops_cnt = 0;
    e->disable_flush_local = 0;
    e->win_complete_flag = 0;
    e->put_acc_issued = 0;

    e->sync.sync_flag = MPIDI_RMA_SYNC_NONE;
    e->sync.outstanding_acks = 0;
    e->sync.have_remote_incomplete_ops = 1; /* When I create a new target, there must be
                                               incomplete ops until a FLUSH/UNLOCK packet
                                               is sent. */
    return e;
}

/* MPIDI_CH3I_Win_target_free(): put a target element back to the target pool
 * it belongs to. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_target_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_target_free(MPID_Win * win_ptr, MPIDI_RMA_Target_t * e)
{
    int mpi_errno = MPI_SUCCESS;

    /* We enqueue elements to the right pool, so when they get freed
     * at window free time, they won't conflict with the global pool
     * or other windows */
    MPIU_Assert(e->read_op_list == NULL);
    MPIU_Assert(e->write_op_list == NULL);
    MPIU_Assert(e->dt_op_list == NULL);
    MPIU_Assert(e->pending_op_list == NULL);

    /* use PREPEND when return objects back to the pool
       in order to improve cache performance */
    if (e->pool_type == MPIDI_RMA_POOL_WIN)
        MPL_LL_PREPEND(win_ptr->target_pool, win_ptr->target_pool_tail, e);
    else
        MPL_LL_PREPEND(global_rma_target_pool, global_rma_target_pool_tail, e);

    return mpi_errno;
}

/* MPIDI_CH3I_Win_create_target(): given a rank, create
 * corresponding target in RMA slots. */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_create_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_create_target(MPID_Win * win_ptr, int target_rank,
                                               MPIDI_RMA_Target_t **e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Slot_t *slot = NULL;
    MPIDI_RMA_Target_t *t = NULL;

    if (win_ptr->num_slots < win_ptr->comm_ptr->local_size)
        slot = &(win_ptr->slots[target_rank % win_ptr->num_slots]);
    else
        slot = &(win_ptr->slots[target_rank]);

    t = MPIDI_CH3I_Win_target_alloc(win_ptr);
    if (t == NULL) {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_target_aggressive(win_ptr, &t);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    t->target_rank = target_rank;

    if (slot->target_list == NULL)
        win_ptr->non_empty_slots++;

    /* Enqueue target into target list. */
    MPL_LL_APPEND(slot->target_list, slot->target_list_tail, t);

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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_find_target(MPID_Win * win_ptr, int target_rank,
                                             MPIDI_RMA_Target_t **e)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Slot_t *slot = NULL;
    MPIDI_RMA_Target_t *t = NULL;

    if (win_ptr->num_slots < win_ptr->comm_ptr->local_size)
        slot = &(win_ptr->slots[target_rank % win_ptr->num_slots]);
    else
        slot = &(win_ptr->slots[target_rank]);

    t = slot->target_list;
    while (t != NULL) {
        if (t->target_rank == target_rank)
            break;
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_enqueue_op(MPID_Win * win_ptr,
                                            MPIDI_RMA_Op_t * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Target_t *target = NULL;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, op->target_rank, &target);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    if (target == NULL) {
        mpi_errno = MPIDI_CH3I_Win_create_target(win_ptr, op->target_rank, &target);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
               the first time to create this target, set its access state
               to MPIDI_RMA_LOCK_CALLED. */
            target->access_state = MPIDI_RMA_LOCK_CALLED;
            target->lock_type = MPI_LOCK_SHARED;
        }
    }

    /* Enqueue operation into pending list. */
    MPL_LL_APPEND(target->pending_op_list, target->pending_op_list_tail, op);
    if (target->next_op_to_issue == NULL)
        target->next_op_to_issue = op;

    /* Increment the counter for accumulated posted operations */
    target->accumulated_ops_cnt++;
    win_ptr->accumulated_ops_cnt++;

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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_target_dequeue_and_free(MPID_Win * win_ptr,
                                                         MPIDI_RMA_Target_t * e)
{
    int mpi_errno = MPI_SUCCESS;
    int target_rank = e->target_rank;
    MPIDI_RMA_Slot_t *slot;

    if (win_ptr->num_slots < win_ptr->comm_ptr->local_size)
        slot = &(win_ptr->slots[target_rank % win_ptr->num_slots]);
    else
        slot = &(win_ptr->slots[target_rank]);

    MPL_LL_DELETE(slot->target_list, slot->target_list_tail, e);

    mpi_errno = MPIDI_CH3I_Win_target_free(win_ptr, e);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (slot->target_list == NULL)
        win_ptr->non_empty_slots--;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_ops_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Cleanup_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t *target,
                                                    int *local_completed, int *remote_completed)
{
    MPIDI_RMA_Op_t *curr_op = NULL;
    MPIDI_RMA_Op_t **op_list = NULL, **op_list_tail = NULL;
    int read_flag = 0, write_flag = 0;
    int mpi_errno = MPI_SUCCESS;

    (*local_completed) = 0;
    (*remote_completed) = 0;

    if (win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED)
        goto fn_exit;

    if (target == NULL)
        goto fn_exit;

    if (target->access_state == MPIDI_RMA_LOCK_CALLED ||
        target->access_state == MPIDI_RMA_LOCK_ISSUED)
        goto fn_exit;

    if (target->pending_op_list == NULL &&
        target->read_op_list == NULL &&
        target->write_op_list == NULL &&
        target->dt_op_list == NULL)
        goto cleanup_target;

    if (target->read_op_list != NULL) {
        op_list = &(target->read_op_list);
        op_list_tail = &(target->read_op_list_tail);
        read_flag = 1;
    }
    else if (target->write_op_list != NULL) {
        op_list = &(target->write_op_list);
        op_list_tail = &(target->write_op_list_tail);
        write_flag = 1;
    }
    else if (target->dt_op_list != NULL) {
        op_list = &(target->dt_op_list);
        op_list_tail = &(target->dt_op_list_tail);
    }
    else {
        /* only pending op list is not NULL, nothing we can do here. */
        goto fn_exit;
    }

    curr_op = *op_list;
    while (curr_op != NULL) {
        if (MPID_Request_is_complete(curr_op->request)) {
            /* If there's an error, return it */
            mpi_errno = curr_op->request->status.MPI_ERROR;
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

            /* No errors, free the request */
            MPID_Request_release(curr_op->request);

            /* Release user request */
            if (curr_op->ureq) {
                /* User request must be completed by progress engine */
                MPIU_Assert(MPID_Request_is_complete(curr_op->ureq));

                /* Release the ch3 ref */
                MPID_Request_release(curr_op->ureq);
            }

            /* dequeue the operation and free it */
            MPL_LL_DELETE(*op_list, *op_list_tail, curr_op);
            MPIDI_CH3I_Win_op_free(win_ptr, curr_op);
            win_ptr->active_req_cnt--;

            if (*op_list == NULL) {
                if (read_flag == 1) {
                    read_flag = 0;
                    if (target->write_op_list != NULL) {
                        op_list = &(target->write_op_list);
                        op_list_tail = &(target->write_op_list_tail);
                        write_flag = 1;
                    }
                    else if (target->dt_op_list != NULL) {
                        op_list = &(target->dt_op_list);
                        op_list_tail = &(target->dt_op_list_tail);
                    }
                    else
                        break;
                }
                else if (write_flag == 1) {
                    write_flag = 0;
                    if (target->dt_op_list != NULL) {
                        op_list = &(target->dt_op_list);
                        op_list_tail = &(target->dt_op_list_tail);
                    }
                    else
                        break;
                }
            }
            /* next op */
            curr_op = *op_list;
        }
        else
            break;
    }

  cleanup_target:
    if (target->pending_op_list == NULL &&
        target->read_op_list == NULL && target->write_op_list == NULL &&
        target->dt_op_list == NULL) {

        (*local_completed) = 1;

        /* for the conditions that need to be satisfied before we free the
         * target, see the MPIDI_RMA_Target definition in
         * mpid_rma_types.h */
        if (target->sync.sync_flag == MPIDI_RMA_SYNC_NONE &&
            target->sync.outstanding_acks == 0 &&
            target->sync.have_remote_incomplete_ops == 0) {
            (*remote_completed) = 1;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_ops_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Cleanup_ops_win(MPID_Win *win_ptr,
                                                 int *local_completed, int *remote_completed)
{
    MPIDI_RMA_Target_t *target = NULL;
    int num_targets = 0, local_completed_targets = 0, remote_completed_targets = 0;
    int i, mpi_errno = MPI_SUCCESS;

    (*local_completed) = 0;
    (*remote_completed) = 0;

    for (i = 0; i < win_ptr->num_slots; i++) {
        for (target = win_ptr->slots[i].target_list; target; ) {
            int local = 0, remote = 0;

            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_target(win_ptr, target, &local, &remote);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            num_targets++;
            local_completed_targets += local;
            remote_completed_targets += remote;

            target = target->next;
        }
    }

    if (num_targets == local_completed_targets)
        (*local_completed) = 1;
    if (num_targets == remote_completed_targets)
        (*remote_completed) = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_single_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Cleanup_single_target(MPID_Win *win_ptr, MPIDI_RMA_Target_t *target)
{
    int mpi_errno = MPI_SUCCESS;

    /* dequeue the target and free it. */
    mpi_errno = MPIDI_CH3I_Win_target_dequeue_and_free(win_ptr, target);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_targets_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Cleanup_targets_win(MPID_Win *win_ptr)
{
    MPIDI_RMA_Target_t *target = NULL, *next_target = NULL;
    int i, mpi_errno = MPI_SUCCESS;

    for (i = 0; i < win_ptr->num_slots; i++) {
        for (target = win_ptr->slots[i].target_list; target; ) {
            next_target = target->next;
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_single_target(win_ptr, target);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            target = next_target;
        }
    }

    MPIU_Assert(win_ptr->non_empty_slots == 0);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Win_get_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Win_get_op(MPID_Win * win_ptr, MPIDI_RMA_Op_t **e)
{
    MPIDI_RMA_Op_t *new_ptr = NULL;
    int local_completed = 0, remote_completed = 0;
    int mpi_errno = MPI_SUCCESS;

    while (1) {
        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
        if (new_ptr != NULL) break;

        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr,
                                                   &local_completed,
                                                   &remote_completed);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
        if (new_ptr != NULL) break;

        if (MPIDI_RMA_Pkt_orderings->flush_remote) {
            mpi_errno = MPIDI_CH3I_RMA_Free_ops_before_completion(win_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
        if (new_ptr != NULL) break;

        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    (*e) = new_ptr;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* Append an element to the tail of the RMA ops list
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param IN    elem      Pointer to the element to be appended
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_append
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_append(MPIDI_RMA_Ops_list_t * list, MPIDI_RMA_Ops_list_t * list_tail,
                                             MPIDI_RMA_Op_t * elem)
{
    MPL_LL_APPEND(*list, *list_tail, elem);
}


/* Unlink an element from the RMA ops list
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param IN    elem      Pointer to the element to be unlinked
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_unlink
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_unlink(MPIDI_RMA_Ops_list_t * list, MPIDI_RMA_Ops_list_t *list_tail,
                                             MPIDI_RMA_Op_t * elem)
{
    MPL_LL_DELETE(*list, *list_tail, elem);
}


/* Free an element in the RMA operations list.
 *
 * @param IN    list      Pointer to the RMA ops list
 * @param IN    curr_ptr  Pointer to the element to be freed.
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Ops_free_elem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void MPIDI_CH3I_RMA_Ops_free_elem(MPID_Win * win_ptr, MPIDI_RMA_Ops_list_t * list,
                                                MPIDI_RMA_Ops_list_t * list_tail,
                                                MPIDI_RMA_Op_t * curr_ptr)
{
    MPIDI_RMA_Op_t *tmp_ptr = curr_ptr;

    MPIU_Assert(curr_ptr != NULL);

    MPL_LL_DELETE(*list, *list_tail, curr_ptr);

    MPIDI_CH3I_Win_op_free(win_ptr, tmp_ptr);
}


#endif /* MPID_RMA_OPLIST_H_INCLUDED */
