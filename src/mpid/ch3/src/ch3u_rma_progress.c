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
      default     : 2097152
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
static inline int check_and_switch_window_state(MPID_Win * win_ptr, int *is_able_to_issue);
static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                   int *made_progress);
static inline int issue_ops_win(MPID_Win * win_ptr, int *made_progress);

/* check if we can switch window-wide state: FENCE_ISSUED, PSCW_ISSUED, LOCK_ALL_ISSUED */
#undef FUNCNAME
#define FUNCNAME check_and_switch_window_state
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int check_and_switch_window_state(MPID_Win * win_ptr, int *is_able_to_issue)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CHECK_AND_SWITCH_WINDOW_STATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_CHECK_AND_SWITCH_WINDOW_STATE);

    (*is_able_to_issue) = 0;

    switch (win_ptr->states.access_state) {
    case MPIDI_RMA_PER_TARGET:
    case MPIDI_RMA_LOCK_ALL_CALLED:
    case MPIDI_RMA_FENCE_GRANTED:
    case MPIDI_RMA_PSCW_GRANTED:
    case MPIDI_RMA_LOCK_ALL_GRANTED:
        (*is_able_to_issue) = 1;
        break;

    default:
        break;
    }   /* end of switch */

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_CHECK_AND_SWITCH_WINDOW_STATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME check_and_switch_target_state
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
                        MPIU_ERR_POP(mpi_errno);
                }
                else {
                    mpi_errno = send_lock_msg(target->target_rank, target->lock_type, win_ptr);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIU_ERR_POP(mpi_errno);
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
                if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH && target->put_acc_issued) {
                    flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
                    target->sync.outstanding_acks++;
                }

                mpi_errno = send_decr_at_cnt_msg(target->target_rank, win_ptr, flags);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);

                /* We are done with ending synchronization, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

                (*made_progress) = 1;
            }
        }
        else if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
            if (target->pending_net_ops_list_head == NULL) {
                if (target->target_rank != rank) {
                    if (target->put_acc_issued) {

                        target->sync.outstanding_acks++;

                        mpi_errno = send_flush_msg(target->target_rank, win_ptr);
                        if (mpi_errno != MPI_SUCCESS)
                            MPIU_ERR_POP(mpi_errno);
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
                        MPIU_ERR_POP(mpi_errno);
                }
                else {
                    MPIDI_CH3_Pkt_flags_t flag = MPIDI_CH3_PKT_FLAG_NONE;
                    if (!target->put_acc_issued) {
                        /* We did not issue PUT/ACC since the last
                         * synchronization call, therefore here we
                         * don't need ACK back */

                        flag = MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK;
                    }
                    else {
                        target->sync.outstanding_acks++;
                    }
                    mpi_errno = send_unlock_msg(target->target_rank, win_ptr, flag);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIU_ERR_POP(mpi_errno);
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


#undef FUNCNAME
#define FUNCNAME issue_ops_target
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                   int *made_progress)
{
    MPIDI_RMA_Op_t *curr_op = NULL;
    MPIDI_CH3_Pkt_flags_t flags;
    int first_op = 1, mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    if (win_ptr->num_targets_with_pending_net_ops == 0 || target == NULL ||
        target->pending_net_ops_list_head == NULL)
        goto fn_exit;

    /* Issue out operations in the list. */
    curr_op = target->next_op_to_issue;
    while (curr_op != NULL) {
        int op_completed = FALSE;
        int is_read_op = FALSE;

        if (target->access_state == MPIDI_RMA_LOCK_ISSUED) {
            /* It is possible that the previous OP+LOCK changes
             * lock state to LOCK_ISSUED. */
            break;
        }

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

        /* piggyback FLUSH on every OP if ordered flush is not guaranteed. */
        if (!MPIDI_CH3U_Win_pkt_orderings.am_flush_ordered)
            flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;

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
             || (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK)))
            target->sync.outstanding_acks++;

        mpi_errno = issue_rma_op(curr_op, win_ptr, target, flags);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        (*made_progress) = 1;

        MPIDI_CH3I_RMA_PKT_IS_READ_OP(curr_op->pkt, is_read_op);
        if (!is_read_op) {
            target->put_acc_issued = 1; /* set PUT_ACC_FLAG when sending
                                         * PUT/ACC operation. */
        }

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
            if (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH || flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
                /* We are done with ending sync, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;
            }
        }

        mpi_errno = check_and_set_req_completion(win_ptr, target, curr_op, &op_completed);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }

        if (op_completed == FALSE) {
            /* Poke the progress engine when next_op_to_issue is not the current OP, in
             * order to make sure the issuing function is re-entrant safe. */
            if (target->next_op_to_issue != curr_op &&
                win_ptr->active_req_cnt > MPIR_CVAR_CH3_RMA_POKE_PROGRESS_REQ_THRESHOLD) {
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }

        curr_op = target->next_op_to_issue;

    }   /* end of while loop */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME issue_ops_win
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
                MPIU_ERR_POP(mpi_errno);
            if (temp_progress)
                (*made_progress) = 1;
            if (!is_able_to_issue) {
                continue;
            }

            /* issue operations to this target */
            mpi_errno = issue_ops_target(win_ptr, target, &temp_progress);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
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
#define FUNCNAME MPIDI_CH3I_RMA_Free_ops_before_completion
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Free_ops_before_completion(MPID_Win * win_ptr)
{
    MPIDI_RMA_Op_t *curr_op = NULL;
    MPIDI_RMA_Target_t *curr_target = NULL;
    MPIDI_RMA_Op_t **op_list_head = NULL;
    int read_flag = 0;
    int i, made_progress = 0;
    int mpi_errno = MPI_SUCCESS;

    /* make nonblocking progress once */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    if (win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED)
        goto fn_exit;

    /* find targets that have operations */
    for (i = 0; i < win_ptr->num_slots; i++) {
        if (win_ptr->slots[i].target_list_head != NULL) {
            curr_target = win_ptr->slots[i].target_list_head;
            while (curr_target != NULL) {
                if (curr_target->issued_read_op_list_head != NULL ||
                    curr_target->issued_write_op_list_head != NULL) {
                    if (win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
                        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
                        if (curr_target->access_state == MPIDI_RMA_LOCK_GRANTED)
                            break;
                    }
                    else {
                        break;
                    }
                }
                curr_target = curr_target->next;
            }
            if (curr_target != NULL)
                break;
        }
    }

    if (curr_target == NULL)
        goto fn_exit;

    /* After we do this, all following Win_flush_local
     * must do a Win_flush instead. */
    curr_target->sync.upgrade_flush_local = 1;

    op_list_head = &curr_target->issued_read_op_list_head;
    read_flag = 1;

    curr_op = *op_list_head;

    /* free all ops in the list since we do not need to maintain them anymore */
    while (1) {
        if (curr_op != NULL) {
            /* Here we mark the operation pointer in the request
             * as NULL, so that when request is completed, it will
             * not try to free this operation. */
            if (curr_op->reqs_size == 1) {
                MPIU_Assert(curr_op->single_req != NULL);
                curr_op->single_req->dev.rma_op_ptr = NULL;
            }
            else {
                MPIU_Assert(curr_op->reqs_size > 1);
                MPIU_Assert(curr_op->multi_reqs != NULL);
                for (i = 0; i < curr_op->reqs_size; i++) {
                    if (curr_op->multi_reqs[i] != NULL) {
                        curr_op->multi_reqs[i]->dev.rma_op_ptr = NULL;
                    }
                }
            }
            MPIDI_CH3I_RMA_Ops_free_elem(win_ptr, op_list_head, curr_op);
        }
        else {
            if (read_flag == 1) {
                op_list_head = &curr_target->issued_write_op_list_head;
                read_flag = 0;
            }
            else {
                /* we reach the tail of write_op_list, break out. */
                break;
            }
        }
        curr_op = *op_list_head;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_ops_aggressive
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
        MPIU_ERR_POP(mpi_errno);

    /* Wait for local completion. */
    do {
        MPIDI_CH3I_RMA_ops_completion(win_ptr, curr_target, local_completed, remote_completed);

        if (!local_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
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
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
                    MPIU_ERR_POP(mpi_errno);
                if (curr_target == NULL) {
                    win_ptr->outstanding_locks++;
                    mpi_errno = send_lock_msg(i, MPI_LOCK_SHARED, win_ptr);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIU_ERR_POP(mpi_errno);
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
            MPIU_ERR_POP(mpi_errno);

        /* Wait for remote completion. */
        do {
            MPIDI_CH3I_RMA_ops_completion(win_ptr, curr_target, local_completed, remote_completed);

            if (!remote_completed) {
                mpi_errno = wait_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        } while (!remote_completed);

        /* Cleanup the target. */
        mpi_errno = MPIDI_CH3I_Win_target_dequeue_and_free(win_ptr, curr_target);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

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
#define FCNAME MPIU_QUOTE(FUNCNAME)
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

    /* check window state */
    mpi_errno = check_and_switch_window_state(win_ptr, &is_able_to_issue);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (!is_able_to_issue) {
        mpi_errno = poke_progress_engine();
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* find target element */
    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* check and try to switch target state */
    mpi_errno = check_and_switch_target_state(win_ptr, target, &is_able_to_issue, &temp_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (temp_progress)
        (*made_progress) = 1;
    if (!is_able_to_issue) {
        mpi_errno = poke_progress_engine();
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* issue operations to this target */
    mpi_errno = issue_ops_target(win_ptr, target, &temp_progress);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);
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
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_win(MPID_Win * win_ptr, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    int is_able_to_issue = 0;
    int temp_progress = 0;

    (*made_progress) = 0;

    /* NOTE: this function is called from either synchronization routines
     * (MPI_WIN_FENCE, MPI_WIN_LOCK...), or aggressive cleanup functions.
     * It cannot be called from the progress engine.
     * Here we poke the progress engine if window state is not satisfied (i.e. NBC is not
     * finished). If it is allowed to be called from progress engine, when RMA progress
     * is registered / executed before NBC progress, it will cause the progress engine
     * to re-entrant RMA progress endlessly. */

    /* check and try to switch window state */
    mpi_errno = check_and_switch_window_state(win_ptr, &is_able_to_issue);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (!is_able_to_issue) {
        mpi_errno = poke_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    mpi_errno = issue_ops_win(win_ptr, &temp_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
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
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_global(int *made_progress)
{
    MPID_Win *win_ptr;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    if (MPIDI_RMA_Win_active_list_head == NULL)
        goto fn_exit;

    for (win_ptr = MPIDI_RMA_Win_active_list_head; win_ptr; win_ptr = win_ptr->next) {
        int temp_progress = 0;
        int is_able_to_issue = 0;

        if (win_ptr->states.access_state == MPIDI_RMA_NONE ||
            win_ptr->states.access_state == MPIDI_RMA_FENCE_GRANTED ||
            win_ptr->states.access_state == MPIDI_RMA_PSCW_GRANTED)
            continue;

        /* check and try to switch window state */
        mpi_errno = check_and_switch_window_state(win_ptr, &is_able_to_issue);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
        if (!is_able_to_issue) {
            continue;
        }

        mpi_errno = issue_ops_win(win_ptr, &temp_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
        if (temp_progress)
            (*made_progress) = 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
