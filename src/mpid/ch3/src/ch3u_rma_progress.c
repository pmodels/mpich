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

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static inline int check_target_state(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                     int *made_progress);
static inline int check_window_state(MPID_Win * win_ptr, int *made_progress);
static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                   int *made_progress);
static inline int issue_ops_win(MPID_Win * win_ptr, int *made_progress);

/* check if we can switch window-wide state: FENCE_ISSUED, PSCW_ISSUED, LOCK_ALL_ISSUED */
#undef FUNCNAME
#define FUNCNAME check_window_state
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int check_window_state(MPID_Win * win_ptr, int *made_progress)
{
    MPID_Request *fence_req_ptr = NULL;
    int i, mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CHECK_WINDOW_STATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_CHECK_WINDOW_STATE);

    (*made_progress) = 0;

    switch (win_ptr->states.access_state) {
    case MPIDI_RMA_FENCE_ISSUED:
        MPID_Request_get_ptr(win_ptr->fence_sync_req, fence_req_ptr);
        if (MPID_Request_is_complete(fence_req_ptr)) {
            win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
            MPID_Request_release(fence_req_ptr);
            win_ptr->fence_sync_req = MPI_REQUEST_NULL;

            num_active_issued_win--;
            MPIU_Assert(num_active_issued_win >= 0);

            (*made_progress) = 1;
        }
        break;

    case MPIDI_RMA_PSCW_ISSUED:
        if (win_ptr->start_req == NULL) {
            /* for MPI_MODE_NOCHECK and all targets on SHM,
             * we do not create PSCW requests on window. */
            win_ptr->states.access_state = MPIDI_RMA_PSCW_GRANTED;

            num_active_issued_win--;
            MPIU_Assert(num_active_issued_win >= 0);

            (*made_progress) = 1;
        }
        else {
            for (i = 0; i < win_ptr->start_grp_size; i++) {
                MPID_Request *start_req_ptr = NULL;
                if (win_ptr->start_req[i] == MPI_REQUEST_NULL)
                    continue;
                MPID_Request_get_ptr(win_ptr->start_req[i], start_req_ptr);
                if (MPID_Request_is_complete(start_req_ptr)) {
                    MPID_Request_release(start_req_ptr);
                    win_ptr->start_req[i] = MPI_REQUEST_NULL;
                }
                else {
                    break;
                }
            }

            if (i == win_ptr->start_grp_size) {
                win_ptr->states.access_state = MPIDI_RMA_PSCW_GRANTED;

                num_active_issued_win--;
                MPIU_Assert(num_active_issued_win >= 0);

                (*made_progress) = 1;

                MPIU_Free(win_ptr->start_req);
                win_ptr->start_req = NULL;
            }
        }
        break;

    case MPIDI_RMA_LOCK_ALL_ISSUED:
        if (win_ptr->outstanding_locks == 0) {
            win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_GRANTED;
            (*made_progress) = 1;
        }
        break;

    default:
        break;
    }   /* end of switch */

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_CHECK_WINDOW_STATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME check_target_state
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int check_target_state(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                     int *made_progress)
{
    int rank = win_ptr->comm_ptr->rank;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    if (target == NULL)
        goto fn_exit;

    /* This check should only be performed when window-wide sync is finished, or
     * current sync is per-target sync. */
    if (win_ptr->states.access_state == MPIDI_RMA_NONE ||
        win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED) {
        goto fn_exit;
    }

    switch (target->access_state) {
    case MPIDI_RMA_LOCK_CALLED:
        if (target->sync.sync_flag == MPIDI_RMA_SYNC_NONE ||
            target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH_LOCAL ||
            target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
            if (target->pending_op_list == NULL ||
                !target->pending_op_list->piggyback_lock_candidate) {
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
            if (target->pending_op_list == NULL) {
                /* No RMA operation has ever been posted to this target,
                 * finish issuing, no need to acquire the lock. Cleanup
                 * function will clean it up. */
                target->access_state = MPIDI_RMA_LOCK_GRANTED;

                target->sync.outstanding_acks--;
                MPIU_Assert(target->sync.outstanding_acks >= 0);

                /* We are done with ending synchronization, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

                (*made_progress) = 1;
            }
            else {
                /* if we reach WIN_UNLOCK and there is still operation existing
                 * in pending list, this operation must be the only operation
                 * and it is prepared to piggyback LOCK and UNLOCK. */
                MPIU_Assert(target->pending_op_list->next == NULL);
                MPIU_Assert(target->pending_op_list->piggyback_lock_candidate);
            }
        }
        break;

    case MPIDI_RMA_LOCK_GRANTED:
    case MPIDI_RMA_NONE:
        if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
            if (target->pending_op_list == NULL) {
                if (target->target_rank == rank) {
                    target->sync.outstanding_acks--;
                    MPIU_Assert(target->sync.outstanding_acks >= 0);
                }
                else {
                    if (target->put_acc_issued) {
                        mpi_errno = send_flush_msg(target->target_rank, win_ptr);
                        if (mpi_errno != MPI_SUCCESS)
                            MPIU_ERR_POP(mpi_errno);
                    }
                    else {
                        /* We did not issue PUT/ACC since the last
                         * synchronization call, therefore here we
                         * don't need ACK back */
                        target->sync.outstanding_acks--;
                        MPIU_Assert(target->sync.outstanding_acks >= 0);
                    }
                }

                /* We are done with ending synchronization, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

                (*made_progress) = 1;
            }
        }
        else if (target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) {
            if (target->pending_op_list == NULL) {
                if (target->target_rank == rank) {
                    target->sync.outstanding_acks--;
                    MPIU_Assert(target->sync.outstanding_acks >= 0);

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
                        target->sync.outstanding_acks--;
                        MPIU_Assert(target->sync.outstanding_acks >= 0);

                        flag = MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK;
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

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME issue_ops_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                   int *made_progress)
{
    MPIDI_RMA_Op_t *curr_op = NULL;
    MPIDI_CH3_Pkt_flags_t flags;
    int first_op = 1, mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    if (win_ptr->non_empty_slots == 0 || target == NULL)
        goto fn_exit;

    /* Exit if window-wide sync is not finished */
    if (win_ptr->states.access_state == MPIDI_RMA_NONE ||
        win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED)
        goto fn_exit;

    /* Exit if per-target sync is not finished */
    if (win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
        if (target->access_state == MPIDI_RMA_LOCK_ISSUED)
            goto fn_exit;
    }

    /* Issue out operations in the list. */
    curr_op = target->next_op_to_issue;
    while (curr_op != NULL) {

        if (target->access_state == MPIDI_RMA_LOCK_ISSUED) {
            /* It is possible that the previous OP+LOCK changes
             * lock state to LOCK_ISSUED. */
            break;
        }

        if (curr_op->next == NULL &&
            target->sync.sync_flag == MPIDI_RMA_SYNC_NONE && curr_op->ureq == NULL) {
            /* Skip the last OP if sync_flag is NONE since we
             * want to leave it to the ending synchronization
             * so that we can piggyback LOCK / FLUSH.
             * However, if it is a request-based RMA, do not
             * skip it (otherwise a wait call before unlock
             * will be blocked). */
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

        if (curr_op->next == NULL) {
            /* piggyback on last OP. */
            if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
                if (target->win_complete_flag)
                    flags |= MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER;
            }
            else if (target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            }
        }

        mpi_errno = issue_rma_op(curr_op, win_ptr, target, flags);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        (*made_progress) = 1;

        if (curr_op->pkt.type == MPIDI_CH3_PKT_PUT ||
            curr_op->pkt.type == MPIDI_CH3_PKT_PUT_IMMED ||
            curr_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE ||
            curr_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE_IMMED) {
            target->put_acc_issued = 1; /* set PUT_ACC_FLAG when sending
                                         * PUT/ACC operation. */
        }

        if ((curr_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE ||
             curr_op->pkt.type == MPIDI_CH3_PKT_GET_ACCUM) && curr_op->issued_stream_count > 0) {
            /* For ACC-like operations, if not all stream units
             * are issued out, we stick to the current operation,
             * otherwise we move on to the next operation. */
            target->next_op_to_issue = curr_op;
        }
        else
            target->next_op_to_issue = curr_op->next;

        if (target->next_op_to_issue == NULL) {
            if (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH || flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
                /* We are done with ending sync, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;
            }
        }

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            /* If this operation is piggybacked with LOCK,
             * do not move it out of pending list, and do
             * not complete the user request, because we
             * may need to re-transmit it. */
            break;
        }

        if (curr_op->ureq != NULL) {
            mpi_errno = set_user_req_after_issuing_op(curr_op);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        if (curr_op->reqs_size == 0) {
            MPIU_Assert(curr_op->reqs == NULL);
            /* Sending is completed immediately. */
            MPIDI_CH3I_RMA_Ops_free_elem(win_ptr, &(target->pending_op_list),
                                         &(target->pending_op_list_tail), curr_op);
        }
        else {
            /* Sending is not completed immediately. */

            MPIDI_CH3I_RMA_Ops_unlink(&(target->pending_op_list),
                                      &(target->pending_op_list_tail), curr_op);
            if (curr_op->is_dt) {
                MPIDI_CH3I_RMA_Ops_append(&(target->dt_op_list),
                                          &(target->dt_op_list_tail), curr_op);
            }
            else if (curr_op->pkt.type == MPIDI_CH3_PKT_PUT ||
                     curr_op->pkt.type == MPIDI_CH3_PKT_PUT_IMMED ||
                     curr_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE ||
                     curr_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE_IMMED) {
                MPIDI_CH3I_RMA_Ops_append(&(target->write_op_list),
                                          &(target->write_op_list_tail), curr_op);
            }
            else {
                MPIDI_CH3I_RMA_Ops_append(&(target->read_op_list),
                                          &(target->read_op_list_tail), curr_op);
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int issue_ops_win(MPID_Win * win_ptr, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    int start_slot, end_slot, i, idx;
    MPIDI_RMA_Target_t *target = NULL;

    (*made_progress) = 0;

    if (win_ptr->non_empty_slots == 0)
        goto fn_exit;

    /* Exit if window-wide sync is not finished */
    if (win_ptr->states.access_state == MPIDI_RMA_NONE ||
        win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED)
        goto fn_exit;

    /* FIXME: we should optimize the issuing pattern here. */

    start_slot = win_ptr->comm_ptr->rank % win_ptr->num_slots;
    end_slot = start_slot + win_ptr->num_slots;
    for (i = start_slot; i < end_slot; i++) {
        if (i < win_ptr->num_slots)
            idx = i;
        else
            idx = i - win_ptr->num_slots;

        target = win_ptr->slots[idx].target_list;
        while (target != NULL) {
            int temp_progress = 0;

            /* check target state */
            mpi_errno = check_target_state(win_ptr, target, &temp_progress);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
            if (temp_progress)
                (*made_progress) = 1;

            /* issue operations to this target */
            mpi_errno = issue_ops_target(win_ptr, target, &temp_progress);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
            if (temp_progress)
                (*made_progress) = 1;

            target = target->next;
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Free_ops_before_completion(MPID_Win * win_ptr)
{
    MPIDI_RMA_Op_t *curr_op = NULL;
    MPIDI_RMA_Target_t *curr_target = NULL;
    MPIDI_RMA_Op_t **op_list = NULL, **op_list_tail = NULL;
    int read_flag = 0;
    int i, made_progress = 0;
    int mpi_errno = MPI_SUCCESS;

    /* If we are in an free_ops_before_completion, the window must be holding
     * up resources.  If it isn't, we are in the wrong window and
     * incorrectly entered this function. */
    MPIU_ERR_CHKANDJUMP(win_ptr->non_empty_slots == 0, mpi_errno, MPI_ERR_OTHER, "**rmanoop");

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
        if (win_ptr->slots[i].target_list != NULL) {
            curr_target = win_ptr->slots[i].target_list;
            while (curr_target != NULL) {
                if (curr_target->read_op_list != NULL || curr_target->write_op_list != NULL) {
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
    curr_target->disable_flush_local = 1;

    if (curr_target->read_op_list != NULL) {
        op_list = &curr_target->read_op_list;
        op_list_tail = &curr_target->read_op_list_tail;
        read_flag = 1;
    }
    else {
        op_list = &curr_target->write_op_list;
        op_list_tail = &curr_target->write_op_list_tail;
    }

    /* free all ops in the list since we do not need to maintain them anymore */
    for (curr_op = *op_list; curr_op != NULL;) {
        if (curr_op->reqs_size > 0) {
            MPIU_Assert(curr_op->reqs != NULL);
            for (i = 0; i < curr_op->reqs_size; i++) {
                if (curr_op->reqs[i] != NULL) {
                    MPID_Request_release(curr_op->reqs[i]);
                    curr_op->reqs[i] = NULL;
                    win_ptr->active_req_cnt--;
                }
            }

            /* free req array in this op */
            MPIU_Free(curr_op->reqs);
            curr_op->reqs = NULL;
            curr_op->reqs_size = 0;
        }
        MPL_LL_DELETE(*op_list, *op_list_tail, curr_op);
        MPIDI_CH3I_Win_op_free(win_ptr, curr_op);

        if (*op_list == NULL) {
            if (read_flag == 1) {
                op_list = &curr_target->write_op_list;
                op_list = &curr_target->write_op_list_tail;
                read_flag = 0;
            }
        }
        curr_op = *op_list;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Cleanup_ops_aggressive
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Cleanup_ops_aggressive(MPID_Win * win_ptr)
{
    int i, local_completed = 0, remote_completed = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int made_progress = 0;

    /* If we are in an aggressive cleanup, the window must be holding
     * up resources.  If it isn't, we are in the wrong window and
     * incorrectly entered this function. */
    MPIU_ERR_CHKANDJUMP(win_ptr->non_empty_slots == 0, mpi_errno, MPI_ERR_OTHER, "**rmanoop");

    /* find the first target that has something to issue */
    for (i = 0; i < win_ptr->num_slots; i++) {
        if (win_ptr->slots[i].target_list != NULL) {
            curr_target = win_ptr->slots[i].target_list;
            while (curr_target != NULL && curr_target->pending_op_list == NULL)
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
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_target(win_ptr, curr_target,
                                                      &local_completed, &remote_completed);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Cleanup_target_aggressive(MPID_Win * win_ptr, MPIDI_RMA_Target_t ** target)
{
    int i, local_completed = 0, remote_completed = 0;
    int made_progress = 0;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int mpi_errno = MPI_SUCCESS;

    (*target) = NULL;

    /* If we are in an aggressive cleanup, the window must be holding
     * up resources.  If it isn't, we are in the wrong window and
     * incorrectly entered this function. */
    MPIU_ERR_CHKANDJUMP(win_ptr->non_empty_slots == 0, mpi_errno, MPI_ERR_OTHER, "**rmanotarget");

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
            if (win_ptr->slots[i].target_list != NULL)
                break;
        curr_target = win_ptr->slots[i].target_list;
        if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
            curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
            curr_target->sync.have_remote_incomplete_ops = 0;
            curr_target->sync.outstanding_acks++;
        }

        /* Issue out all operations. */
        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, curr_target->target_rank,
                                                        &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        /* Wait for remote completion. */
        do {
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_target(win_ptr, curr_target,
                                                          &local_completed, &remote_completed);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
            if (!remote_completed) {
                mpi_errno = wait_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        } while (!remote_completed);

        /* Cleanup the target. */
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_single_target(win_ptr, curr_target);
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_target(MPID_Win * win_ptr, int target_rank, int *made_progress)
{
    int temp_progress = 0;
    MPIDI_RMA_Target_t *target = NULL;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    /* check window state */
    mpi_errno = check_window_state(win_ptr, &temp_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (temp_progress)
        (*made_progress) = 1;

    /* find target element */
    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* check target state */
    mpi_errno = check_target_state(win_ptr, target, &temp_progress);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);
    if (temp_progress)
        (*made_progress) = 1;

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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_win(MPID_Win * win_ptr, int *made_progress)
{
    int temp_progress = 0;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    /* check window state */
    mpi_errno = check_window_state(win_ptr, &temp_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (temp_progress)
        (*made_progress) = 1;

    /* issue operations on window */
    mpi_errno = issue_ops_win(win_ptr, &temp_progress);
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
#define FUNCNAME MPIDI_CH3I_RMA_Make_progress_global
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_RMA_Make_progress_global(int *made_progress)
{
    MPIDI_RMA_Win_list_t *win_elem = MPIDI_RMA_Win_list;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    for (win_elem = MPIDI_RMA_Win_list; win_elem; win_elem = win_elem->next) {
        int temp_progress = 0;

        if (win_elem->win_ptr->states.access_state == MPIDI_RMA_NONE ||
            win_elem->win_ptr->states.access_state == MPIDI_RMA_FENCE_GRANTED ||
            win_elem->win_ptr->states.access_state == MPIDI_RMA_PSCW_GRANTED)
            continue;

        mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_elem->win_ptr, &temp_progress);
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
