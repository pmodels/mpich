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
    - name        : MPIR_CVAR_CH3_RMA_ACC_IMMED
      category    : CH3
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use the immediate accumulate optimization

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

static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t *target, int *made_progress);
static inline int issue_ops_win(MPID_Win * win_ptr, int *made_progress);

static int send_flush_msg(int dest, MPID_Win *win_ptr);

#undef FUNCNAME
#define FUNCNAME check_window_state
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int check_window_state(MPID_Win *win_ptr, int *made_progress, int *cannot_issue)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CHECK_WINDOW_STATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_CHECK_WINDOW_STATE);

    (*made_progress) = 0;
    (*cannot_issue) = 0;

    if (win_ptr->states.access_state == MPIDI_RMA_NONE) {
        (*cannot_issue) = 1;
        goto fn_exit;
    }
    else if (win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED) {
        MPID_Request *fence_req_ptr = NULL;
        MPID_Request_get_ptr(win_ptr->fence_sync_req, fence_req_ptr);
        if (MPID_Request_is_complete(fence_req_ptr)) {
            win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
            MPID_Request_release(fence_req_ptr);
            win_ptr->fence_sync_req = MPI_REQUEST_NULL;

            num_active_issued_win--;
            MPIU_Assert(num_active_issued_win >= 0);

            (*made_progress) = 1;
        }
        else {
            (*cannot_issue) = 1;
            goto fn_exit;
        }
    }
    else if (win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED) {
        if (win_ptr->start_req == NULL) {
            /* for MPI_MODE_NOCHECK and all targets on SHM,
               we do not create PSCW requests on window. */
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
                    (*cannot_issue) = 1;
                    goto fn_exit;
                }
            }
            MPIU_Assert(i == win_ptr->start_grp_size);
            win_ptr->states.access_state = MPIDI_RMA_PSCW_GRANTED;

            num_active_issued_win--;
            MPIU_Assert(num_active_issued_win >= 0);

            (*made_progress) = 1;

            MPIU_Free(win_ptr->start_req);
            win_ptr->start_req = NULL;
        }
    }
    else if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED) {
        if (win_ptr->outstanding_locks == 0) {
            win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_GRANTED;
            (*made_progress) = 1;
        }
        else {
            (*cannot_issue) = 1;
            goto fn_exit;
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_CHECK_WINDOW_STATE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME issue_ops_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int issue_ops_target(MPID_Win * win_ptr, MPIDI_RMA_Target_t *target,
                                   int *made_progress)
{
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Op_t *curr_op = NULL;
    int first_op;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    if (win_ptr->non_empty_slots == 0 || target == NULL)
        goto fn_exit;

    /* check per-target state */
    if (win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
        if (target->access_state == MPIDI_RMA_LOCK_CALLED) {
            if (target->sync.sync_flag == MPIDI_RMA_SYNC_NONE ||
                target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH_LOCAL ||
                target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
                if (target->pending_op_list != NULL &&
                    target->pending_op_list->piggyback_lock_candidate) {
                    /* Capable of piggybacking LOCK message with first operation. */
                }
                else {
                    target->access_state = MPIDI_RMA_LOCK_ISSUED;
                    target->outstanding_lock++;
                    MPIU_Assert(target->outstanding_lock == 1);
                    if (target->target_rank == rank) {
                        mpi_errno = acquire_local_lock(win_ptr, target->lock_type);
                        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                    }
                    else {
                        mpi_errno = send_lock_msg(target->target_rank,
                                                  target->lock_type, win_ptr);
                        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                    }
                    (*made_progress) = 1;
                    goto fn_exit;
                }
            }
            else if (target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) {
                if (target->pending_op_list != NULL) {
                    /* Capable of piggybacking LOCK message with first operation. */
                    MPIU_Assert(target->pending_op_list->piggyback_lock_candidate);
                }
                else {
                    /* No RMA operation has ever been posted to this target,
                       finish issuing, no need to acquire the lock. Cleanup
                       function will clean it up. */
                    target->sync.outstanding_acks--;
                    MPIU_Assert(target->sync.outstanding_acks == 0);
                    (*made_progress) = 1;

                    /* Unset target's sync_flag. */
                    target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;
                    goto fn_exit;
                }
            }
        }
        else if (target->access_state == MPIDI_RMA_LOCK_ISSUED) {
            if (target->outstanding_lock == 0) {
                target->access_state = MPIDI_RMA_LOCK_GRANTED;
                (*made_progress) = 1;
            }
            else
                goto fn_exit;
        }
    }

    MPIU_Assert(win_ptr->states.access_state == MPIDI_RMA_FENCE_GRANTED ||
                win_ptr->states.access_state == MPIDI_RMA_PSCW_GRANTED ||
                win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
                win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED ||
                win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_GRANTED);

     if (win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
        win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
        MPIU_Assert(target->access_state == MPIDI_RMA_LOCK_CALLED ||
                    target->access_state == MPIDI_RMA_LOCK_GRANTED);
    }

    /* Deal with when there is no operation in the list. */
    if (target->pending_op_list == NULL) {

        /* At this point, per-target state must be LOCK_GRANTED. */
        if (win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
            win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
            MPIU_Assert(target->access_state == MPIDI_RMA_LOCK_GRANTED);
        }

        if (target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) {
            if (target->target_rank == rank) {
                target->sync.outstanding_acks--;
                MPIU_Assert(target->sync.outstanding_acks == 0);
            }
            else {
                mpi_errno = send_flush_msg(target->target_rank, win_ptr);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
        }
        else if (target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) {
            if (target->target_rank == rank) {
                mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                target->sync.outstanding_acks--;
                MPIU_Assert(target->sync.outstanding_acks == 0);
            }
            else {
                mpi_errno = send_unlock_msg(target->target_rank, win_ptr);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
        }
        (*made_progress) = 1;
        goto finish_issue;
    }

    /* Issue out operations in the list. */
    first_op = 1;
    curr_op = target->next_op_to_issue;
    while (curr_op != NULL) {
        MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;

        if (target->access_state == MPIDI_RMA_LOCK_ISSUED)
            goto fn_exit;

        if (curr_op->next == NULL &&
            target->sync.sync_flag == MPIDI_RMA_SYNC_NONE) {
            /* skip last OP. */
            goto finish_issue;
        }

        if (first_op) {
            /* piggyback on first OP. */
            if (target->access_state == MPIDI_RMA_LOCK_CALLED) {
                MPIU_Assert(curr_op->piggyback_lock_candidate);
                flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK;
                target->access_state = MPIDI_RMA_LOCK_ISSUED;
                target->outstanding_lock++;
                MPIU_Assert(target->outstanding_lock == 1);
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

        target->next_op_to_issue = curr_op->next;

        mpi_errno = issue_rma_op(curr_op, win_ptr, target, flags);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (!curr_op->request) {
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
                     curr_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE ||
                     curr_op->pkt.type == MPIDI_CH3_PKT_ACCUM_IMMED) {
                MPIDI_CH3I_RMA_Ops_append(&(target->write_op_list),
                                          &(target->write_op_list_tail), curr_op);
            }
            else {
                MPIDI_CH3I_RMA_Ops_append(&(target->read_op_list),
                                          &(target->read_op_list_tail), curr_op);
            }
            win_ptr->active_req_cnt++;
        }

        curr_op = target->next_op_to_issue;

        (*made_progress) = 1;
    }

 finish_issue:
    /* Unset target's sync_flag. */
    target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME issue_ops_win
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int issue_ops_win(MPID_Win *win_ptr, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    int start_slot, end_slot, i;
    MPIDI_RMA_Target_t *target = NULL;

    (*made_progress) = 0;

    if (win_ptr->non_empty_slots == 0)
        goto fn_exit;

    MPIU_Assert(win_ptr->states.access_state == MPIDI_RMA_FENCE_GRANTED ||
                win_ptr->states.access_state == MPIDI_RMA_PSCW_GRANTED ||
                win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
                win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED ||
                win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_GRANTED);

    start_slot = win_ptr->comm_ptr->rank % win_ptr->num_slots;
    end_slot = start_slot + win_ptr->num_slots;

    for (i = start_slot; i < end_slot; i++) {
        int idx;
        if (i >= win_ptr->num_slots) idx = i - win_ptr->num_slots;
        else idx = i;

        target = win_ptr->slots[idx].target_list;
        while (target != NULL) {
            int temp = 0;
            mpi_errno = issue_ops_target(win_ptr, target, &temp);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            if (temp)
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
    struct MPIDI_RMA_Op **op_list = NULL, **op_list_tail = NULL;
    int read_flag = 0;
    int i, made_progress = 0;
    int mpi_errno = MPI_SUCCESS;

    MPIU_ERR_CHKANDJUMP(win_ptr->non_empty_slots == 0, mpi_errno, MPI_ERR_OTHER,
                        "**rmanoop");

    /* make nonblocking progress once */
    if (win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED ||
        win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED) {
        mpi_errno = issue_ops_win(win_ptr, &made_progress);
        if (mpi_errno != MPI_SUCCESS) {MPIU_ERR_POP(mpi_errno);}
    }
    if (win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED)
        goto fn_exit;

    /* find targets that have operations */
    for (i = 0; i < win_ptr->num_slots; i++) {
        if (win_ptr->slots[i].target_list != NULL) {
            curr_target = win_ptr->slots[i].target_list;
            while (curr_target != NULL && curr_target->read_op_list == NULL
                   && curr_target->write_op_list == NULL)
                curr_target = curr_target->next;
            if (curr_target != NULL) break;
        }
    }
    if (curr_target == NULL) goto fn_exit;

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
    for (curr_op = *op_list; curr_op != NULL; ) {
        MPID_Request_release(curr_op->request);
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
    MPIU_ERR_CHKANDJUMP(win_ptr->non_empty_slots == 0, mpi_errno, MPI_ERR_OTHER,
                        "**rmanoop");

    /* find the first target that has something to issue */
    for (i = 0; i < win_ptr->num_slots; i++) {
        if (win_ptr->slots[i].target_list != NULL) {
            curr_target = win_ptr->slots[i].target_list;
            while (curr_target != NULL && curr_target->pending_op_list == NULL)
                curr_target = curr_target->next;
            if (curr_target != NULL) break;
        }
    }

    if (curr_target == NULL) goto fn_exit;

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
                                                      &local_completed,
                                                      &remote_completed);
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
    MPIU_ERR_CHKANDJUMP(win_ptr->non_empty_slots == 0, mpi_errno, MPI_ERR_OTHER,
                        "**rmanotarget");

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
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
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
                                                          &local_completed,
                                                          &remote_completed);
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
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
    int mpi_errno = MPI_SUCCESS;
    int cannot_issue = 0, temp_progress = 0;
    MPIDI_RMA_Slot_t *slot;
    MPIDI_RMA_Target_t *target;

    (*made_progress) = 0;

    if (win_ptr->num_slots < win_ptr->comm_ptr->local_size) {
        slot = &(win_ptr->slots[target_rank % win_ptr->num_slots]);
        for (target = slot->target_list;
             target && target->target_rank != target_rank; target = target->next);
    }
    else {
        slot = &(win_ptr->slots[target_rank]);
        target = slot->target_list;
    }

    if (target != NULL) {

        /* check window state */
        mpi_errno = check_window_state(win_ptr, &temp_progress, &cannot_issue);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        if (temp_progress)
            (*made_progress) = 1;

        if (cannot_issue)
            goto fn_exit;

        mpi_errno = issue_ops_target(win_ptr, target, &temp_progress);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        if (temp_progress)
            (*made_progress) = 1;
    }

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
    int temp_progress = 0, cannot_issue = 0;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    /* check window state */
    mpi_errno = check_window_state(win_ptr, &temp_progress, &cannot_issue);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (temp_progress)
        (*made_progress) = 1;

    if (cannot_issue)
        goto fn_exit;

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
    int tmp = 0, cannot_issue = 0;
    int mpi_errno = MPI_SUCCESS;

    (*made_progress) = 0;

    for (win_elem = MPIDI_RMA_Win_list; win_elem; win_elem = win_elem->next) {
        if (win_elem->win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED ||
            win_elem->win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED ||
            win_elem->win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
            win_elem->win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED ||
            win_elem->win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED ||
            win_elem->win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_GRANTED) {

            /* check window state */
            mpi_errno = check_window_state(win_elem->win_ptr, &tmp, &cannot_issue);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            if (tmp)
                (*made_progress) = 1;

            if (cannot_issue)
                continue;

            mpi_errno = issue_ops_win(win_elem->win_ptr, &tmp);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
            if (tmp)
                (*made_progress) = 1;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_flush_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_flush_msg(int dest, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_flush_t *flush_pkt = &upkt.flush;
    MPID_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_FLUSH_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_FLUSH_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(flush_pkt, MPIDI_CH3_PKT_FLUSH);
    flush_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    flush_pkt->source_win_handle = win_ptr->handle;

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* Release the request returned by iStartMsg */
    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_FLUSH_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


int MPIDI_CH3I_Issue_rma_op(MPIDI_RMA_Op_t * op_ptr, MPID_Win * win_ptr,
                            MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);

    switch (op_ptr->pkt.type) {
    case (MPIDI_CH3_PKT_PUT):
    case (MPIDI_CH3_PKT_ACCUMULATE):
    case (MPIDI_CH3_PKT_GET_ACCUM):
        mpi_errno = send_rma_msg(op_ptr, win_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_ACCUM_IMMED):
        mpi_errno = send_contig_acc_msg(op_ptr, win_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_GET):
        mpi_errno = recv_rma_msg(op_ptr, win_ptr, flags);
        break;
    case (MPIDI_CH3_PKT_CAS):
    case (MPIDI_CH3_PKT_FOP):
        mpi_errno = send_immed_rmw_msg(op_ptr, win_ptr, flags);
        break;
    default:
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winInvalidOp");
    }

    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ISSUE_RMA_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
