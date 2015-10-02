/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD
      category    : CH3
      type        : int
      default     : 65536
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
         Threshold of number of active requests to trigger
         blocking waiting in operation routines. When the
         value is negative, we never blockingly wait in
         operation routines. When the value is zero, we always
         trigger blocking waiting in operation routines to
         wait until no. of active requests becomes zero. When the
         value is positive, we do blocking waiting in operation
         routines to wait until no. of active requests being
         reduced to this value.

    - name        : MPIR_CVAR_CH3_RMA_POKE_PROGRESS_REQ_THRESHOLD
      category    : CH3
      type        : int
      default     : 128
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Threshold at which the RMA implementation attempts to complete requests
        while completing RMA operations and while using the lazy synchonization
        approach.  Change this value if programs fail because they run out of
        requests or other internal resources

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static inline int check_and_switch_target_state(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                                int *is_able_to_issue, int *made_progress);
static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                   int *made_progress);
static inline int issue_ops_win(MPID_Win * win_ptr, int *made_progress);


/* This macro checks if window state is ready for issuing RMA operations. */
#define WIN_READY(win_)                                                 \
    ((win_)->states.access_state == MPIDI_RMA_PER_TARGET ||             \
     (win_)->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED ||        \
     (win_)->states.access_state == MPIDI_RMA_FENCE_GRANTED ||          \
     (win_)->states.access_state == MPIDI_RMA_PSCW_GRANTED ||           \
     (win_)->states.access_state == MPIDI_RMA_LOCK_ALL_GRANTED)


#undef FUNCNAME
#define FUNCNAME check_and_switch_target_state
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int check_and_switch_target_state(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                                int *is_able_to_issue, int *made_progress)
{
    int rank = win_ptr->comm_ptr->rank;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;
    (*is_able_to_issue) = 0;

    if (target == NULL)
        goto fn_exit;

    /* When user event happens, move op in user pending list to network pending list */
    if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH ||
        target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH_LOCAL ||
        target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK || target->win_complete_flag) {

        MPIDI_RMA_Op_t *user_op = target->pending_user_ops_list_head;

        if (user_op != NULL) {
            if (target->pending_net_ops_list_head == NULL)
                win_ptr->num_targets_with_pending_net_ops++;

            MPL_DL_DELETE(target->pending_user_ops_list_head, user_op);
            MPL_DL_APPEND(target->pending_net_ops_list_head, user_op);

            if (target->next_op_to_issue == NULL)
                target->next_op_to_issue = user_op;
        }
    }

    switch (target->access_state) {
    case MPIDI_RMA_LOCK_CALLED:
        if (target->sync.sync_flag == MPIDI_RMA_SYNC_NONE ||
            target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH_LOCAL ||
            target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
            if ((target->pending_net_ops_list_head == NULL ||
                 !target->pending_net_ops_list_head->piggyback_lock_candidate) &&
                (target->pending_user_ops_list_head == NULL ||
                 !target->pending_user_ops_list_head->piggyback_lock_candidate)) {
                /* issue lock request */
                target->access_state = MPIDI_RMA_LOCK_ISSUED;
                if (target->target_rank == rank) {
                    mpi_errno = acquire_local_lock(win_ptr, target->lock_type);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIR_ERR_POP(mpi_errno);
                }
                else {
                    mpi_errno = send_lock_msg(target->target_rank, target->lock_type, win_ptr);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIR_ERR_POP(mpi_errno);
                }

                (*made_progress) = 1;
            }
        }
        else if (target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) {
            if (target->pending_net_ops_list_head == NULL) {
                /* No RMA operation has ever been posted to this target,
                 * finish issuing, no need to acquire the lock. Cleanup
                 * function will clean it up. */
                target->access_state = MPIDI_RMA_LOCK_GRANTED;

                /* We are done with ending synchronization, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

                (*made_progress) = 1;
            }
            else {
                /* if we reach WIN_UNLOCK and there is still operation existing
                 * in pending list, this operation must be the only operation
                 * and it is prepared to piggyback LOCK and UNLOCK. */
                MPIU_Assert(MPIR_CVAR_CH3_RMA_DELAY_ISSUING_FOR_PIGGYBACKING);
                MPIU_Assert(target->pending_net_ops_list_head->next == NULL);
                MPIU_Assert(target->pending_net_ops_list_head->piggyback_lock_candidate);
            }
        }
        break;

    case MPIDI_RMA_LOCK_GRANTED:
    case MPIDI_RMA_NONE:
        if (target->win_complete_flag) {
            if (target->pending_net_ops_list_head == NULL) {
                MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;
                if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH &&
                    target->num_ops_flush_not_issued > 0) {
                    flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
                    win_ptr->outstanding_acks++;
                    target->sync.outstanding_acks++;
                    target->num_ops_flush_not_issued = 0;
                }

                mpi_errno = send_decr_at_cnt_msg(target->target_rank, win_ptr, flags);
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);

                /* We are done with ending synchronization, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

                (*made_progress) = 1;
            }
        }
        else if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
            if (target->pending_net_ops_list_head == NULL) {
                if (target->target_rank != rank) {
                    if (target->num_ops_flush_not_issued > 0) {

                        win_ptr->outstanding_acks++;
                        target->sync.outstanding_acks++;
                        target->num_ops_flush_not_issued = 0;

                        mpi_errno = send_flush_msg(target->target_rank, win_ptr);
                        if (mpi_errno != MPI_SUCCESS)
                            MPIR_ERR_POP(mpi_errno);
                    }
                }

                /* We are done with ending synchronization, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

                (*made_progress) = 1;
            }
        }
        else if (target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) {
            if (target->pending_net_ops_list_head == NULL) {
                if (target->target_rank == rank) {
                    mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIR_ERR_POP(mpi_errno);
                }
                else {
                    MPIDI_CH3_Pkt_flags_t flag = MPIDI_CH3_PKT_FLAG_NONE;
                    if (target->num_ops_flush_not_issued == 0) {
                        flag = MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK;
                    }
                    else {
                        win_ptr->outstanding_acks++;
                        target->sync.outstanding_acks++;
                        target->num_ops_flush_not_issued = 0;
                    }
                    mpi_errno = send_unlock_msg(target->target_rank, win_ptr, flag);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIR_ERR_POP(mpi_errno);
                }

                /* We are done with ending synchronization, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

                (*made_progress) = 1;
            }
        }
        break;

    default:
        break;
    }   /* end of switch */

    if (target->access_state != MPIDI_RMA_LOCK_ISSUED) {
        (*is_able_to_issue) = 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Note: we should prevent this function to be re-entrant. It has the risk of
 * causing too many re-entrance and using up function stack. */
#undef FUNCNAME
#define FUNCNAME issue_ops_target
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                   int *made_progress)
{
    MPIDI_RMA_Op_t *curr_op = NULL;
    MPIDI_CH3_Pkt_flags_t flags;
    int first_op = 1, mpi_errno = MPI_SUCCESS;
    static int fn_reentrance_check = FALSE;

    /* this function is not reentrant.  if it is invoked in a
     * reentrant manner, simply exit without doing anything. */
    if (fn_reentrance_check == TRUE)
        goto fn_exit;
    fn_reentrance_check = TRUE;

    (*made_progress) = 0;

    if (win_ptr->num_targets_with_pending_net_ops == 0 || target == NULL ||
        target->pending_net_ops_list_head == NULL)
        goto finish_issue;

    /* Issue out operations in the list. */
    curr_op = target->next_op_to_issue;
    while (curr_op != NULL) {
        int op_completed = FALSE;

        if (target->access_state == MPIDI_RMA_LOCK_ISSUED) {
            /* It is possible that the previous OP+LOCK changes
             * lock state to LOCK_ISSUED. */
            break;
        }

        target->num_ops_flush_not_issued++;

        flags = MPIDI_CH3_PKT_FLAG_NONE;

        if (first_op) {
            /* piggyback on first OP. */
            if (target->access_state == MPIDI_RMA_LOCK_CALLED) {
                MPIU_Assert(curr_op->piggyback_lock_candidate);
                if (target->lock_type == MPI_LOCK_SHARED)
                    flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
                else {
                    MPIU_Assert(target->lock_type == MPI_LOCK_EXCLUSIVE);
                    flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
                }
                target->access_state = MPIDI_RMA_LOCK_ISSUED;
            }
            first_op = 0;
        }

        /* piggyback FLUSH on current OP if one of the following
         * conditions meet:
         * (1) ordered flush is not guaranteed;
         * (2) operation is a READ op (GET, GACC, FOP, CAS) */
        if ((!MPIDI_CH3U_Win_pkt_orderings.am_flush_ordered) ||
            MPIDI_CH3I_RMA_PKT_IS_READ_OP(curr_op->pkt)) {
            flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
        }

        if (curr_op->next == NULL) {
            /* piggyback on last OP. */
            if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            }
            else if (target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;

                /* if piggyback UNLOCK then unset FLUSH (set for every
                 * operation on out-of-order network). */
                flags &= ~MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            }
            if (target->win_complete_flag)
                flags |= MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
        }

        /* only increase ack counter when FLUSH or UNLOCK flag is set,
         * but without LOCK piggyback. */
        if (((flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)
             || (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))) {
            win_ptr->outstanding_acks++;
            target->sync.outstanding_acks++;
            target->num_ops_flush_not_issued = 0;
        }

        mpi_errno = issue_rma_op(curr_op, win_ptr, target, flags);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        (*made_progress) = 1;

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* If this operation is piggybacked with LOCK,
             * do not move it out of pending list, and do
             * not complete the user request, because we
             * may need to re-transmit it. */
            break;
        }

        target->next_op_to_issue = curr_op->next;

        if (target->next_op_to_issue == NULL) {
            if (((target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) &&
                 (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)) ||
                ((target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) &&
                 (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))) {
                /* We are done with ending sync, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;
            }
        }

        mpi_errno = check_and_set_req_completion(win_ptr, target, curr_op, &op_completed);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        if (op_completed == FALSE) {
            if (MPIDI_CH3I_RMA_Active_req_cnt > MPIR_CVAR_CH3_RMA_POKE_PROGRESS_REQ_THRESHOLD) {
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
            }
        }

        curr_op = target->next_op_to_issue;

    }   /* end of while loop */

  finish_issue:
    fn_reentrance_check = FALSE;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME issue_ops_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int issue_ops_win(MPID_Win * win_ptr, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    int start_slot, end_slot, i, idx;
    int is_able_to_issue = 0;
    int temp_progress = 0;
    MPIDI_RMA_Target_t *target = NULL;

    (*made_progress) = 0;

    /* FIXME: we should optimize the issuing pattern here. */

    start_slot = win_ptr->comm_ptr->rank % win_ptr->num_slots;
    end_slot = start_slot + win_ptr->num_slots;
    for (i = start_slot; i < end_slot; i++) {
        if (i < win_ptr->num_slots)
            idx = i;
        else
            idx = i - win_ptr->num_slots;

        for (target = win_ptr->slots[idx].target_list_head; target != NULL; target = target->next) {
            /* check and try to switch target state */
            mpi_errno = check_and_switch_target_state(win_ptr, target, &is_able_to_issue,
                                                      &temp_progress);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            if (temp_progress)
                (*made_progress) = 1;
            if (!is_able_to_issue) {
                continue;
            }

            /* issue operations to this target */
            mpi_errno = issue_ops_target(win_ptr, target, &temp_progress);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            if (temp_progress)
                (*made_progress) = 1;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_ops_aggressive
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Cleanup_ops_aggressive(MPID_Win * win_ptr)
{
    int i, local_completed = 0, remote_completed ATTRIBUTE((unused)) = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int made_progress = 0;

    /* find the first target that has something to issue */
    for (i = 0; i < win_ptr->num_slots; i++) {
        if (win_ptr->slots[i].target_list_head != NULL) {
            curr_target = win_ptr->slots[i].target_list_head;
            while (curr_target != NULL && curr_target->pending_net_ops_list_head == NULL &&
                   curr_target->pending_user_ops_list_head == NULL)
                curr_target = curr_target->next;
            if (curr_target != NULL)
                break;
        }
    }

    if (curr_target == NULL)
        goto fn_exit;

    if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH_LOCAL)
        curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH_LOCAL;

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, curr_target->target_rank,
                                                    &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Wait for local completion. */
    do {
        MPIDI_CH3I_RMA_ops_completion(win_ptr, curr_target, local_completed, remote_completed);

        if (!local_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    } while (!local_completed);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_target_aggressive
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Cleanup_target_aggressive(MPID_Win * win_ptr, MPIDI_RMA_Target_t ** target)
{
    int i, local_completed ATTRIBUTE((unused)) = 0, remote_completed = 0;
    int made_progress = 0;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int mpi_errno = MPI_SUCCESS;

    (*target) = NULL;

    if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
        /* switch to window-wide protocol */
        MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
        for (i = 0; i < win_ptr->comm_ptr->local_size; i++) {
            if (i == win_ptr->comm_ptr->rank)
                continue;
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
            if (orig_vc->node_id != target_vc->node_id) {
                mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, i, &curr_target);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                if (curr_target == NULL) {
                    win_ptr->outstanding_locks++;
                    mpi_errno = send_lock_msg(i, MPI_LOCK_SHARED, win_ptr);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_ISSUED;
    }

    do {
        /* find a non-empty slot and set the FLUSH flag on the first
         * target */
        /* TODO: we should think about better strategies on selecting the target */
        for (i = 0; i < win_ptr->num_slots; i++)
            if (win_ptr->slots[i].target_list_head != NULL)
                break;
        curr_target = win_ptr->slots[i].target_list_head;
        if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
            curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
        }

        /* Issue out all operations. */
        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, curr_target->target_rank,
                                                        &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* Wait for remote completion. */
        do {
            MPIDI_CH3I_RMA_ops_completion(win_ptr, curr_target, local_completed, remote_completed);

            if (!remote_completed) {
                mpi_errno = wait_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
            }
        } while (!remote_completed);

        /* Cleanup the target. */
        mpi_errno = MPIDI_CH3I_Win_target_dequeue_and_free(win_ptr, curr_target);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* check if we got a target */
        (*target) = MPIDI_CH3I_Win_target_alloc(win_ptr);

    } while ((*target) == NULL);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Make_progress_target
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_target(MPID_Win * win_ptr, int target_rank, int *made_progress)
{
    int temp_progress = 0;
    int is_able_to_issue = 0;
    MPIDI_RMA_Target_t *target = NULL;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    /* NOTE: this function is called from either operation routines (MPI_PUT, MPI_GET...),
     * or aggressive cleanup functions. It cannot be called from the progress engine.
     * Here we poke the progress engine if window state is not satisfied (i.e. NBC is not
     * finished). If it is allowed to be called from progress engine, when RMA progress
     * is registered / executed before NBC progress, it will cause the progress engine
     * to re-entrant RMA progress endlessly. */

    /* check window state, if it is not ready, poke the progress engine */
    if (!WIN_READY(win_ptr)) {
        mpi_errno = poke_progress_engine();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* find target element */
    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* check and try to switch target state */
    mpi_errno = check_and_switch_target_state(win_ptr, target, &is_able_to_issue, &temp_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    if (temp_progress)
        (*made_progress) = 1;
    if (!is_able_to_issue) {
        mpi_errno = poke_progress_engine();
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* issue operations to this target */
    mpi_errno = issue_ops_target(win_ptr, target, &temp_progress);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (temp_progress)
        (*made_progress) = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Make_progress_win
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_win(MPID_Win * win_ptr, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    int temp_progress = 0;

    (*made_progress) = 0;

    /* NOTE: this function is called from either synchronization routines
     * (MPI_WIN_FENCE, MPI_WIN_LOCK...), or aggressive cleanup functions.
     * It cannot be called from the progress engine.
     * Here we poke the progress engine if window state is not satisfied (i.e. NBC is not
     * finished). If it is allowed to be called from progress engine, when RMA progress
     * is registered / executed before NBC progress, it will cause the progress engine
     * to re-entrant RMA progress endlessly. */

    /* check and try to switch window state, if it is not ready, poke the progress engine */
    if (!WIN_READY(win_ptr)) {
        mpi_errno = poke_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    mpi_errno = issue_ops_win(win_ptr, &temp_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    if (temp_progress)
        (*made_progress) = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Make_progress_global
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_global(int *made_progress)
{
    MPID_Win *win_ptr;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    if (MPIDI_RMA_Win_active_list_head == NULL)
        goto fn_exit;

    for (win_ptr = MPIDI_RMA_Win_active_list_head; win_ptr; win_ptr = win_ptr->next) {
        int temp_progress = 0;

        if (win_ptr->states.access_state == MPIDI_RMA_NONE)
            continue;

        /* check and try to switch window state */
        if (!WIN_READY(win_ptr))
            continue;

        mpi_errno = issue_ops_win(win_ptr, &temp_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
        if (temp_progress)
            (*made_progress) = 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
