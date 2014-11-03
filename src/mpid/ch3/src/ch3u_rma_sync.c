/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : CH3
      description : cvars that control behavior of ch3

cvars:
    - name        : MPIR_CVAR_CH3_RMA_NREQUEST_THRESHOLD
      category    : CH3
      type        : int
      default     : 4000
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Threshold at which the RMA implementation attempts to complete requests
        while completing RMA operations and while using the lazy synchonization
        approach.  Change this value if programs fail because they run out of
        requests or other internal resources

    - name        : MPIR_CVAR_CH3_RMA_NREQUEST_NEW_THRESHOLD
      category    : CH3
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Threshold for the number of new requests since the last attempt to
        complete pending requests.  Higher values can increase performance,
        but may run the risk of exceeding the available number of requests
        or other internal resources.

    - name        : MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED
      category    : CH3
      type        : int
      default     : (-1)
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Threshold for the number of completed requests the runtime finds
        before it stops trying to find more completed requests in garbage
        collection function.
        Note that it works with MPIR_CVAR_CH3_RMA_GC_NUM_TESTED as an OR
        relation, which means runtime will stop checking when either one
        of its following conditions is satisfied or one of conditions of
        MPIR_CVAR_CH3_RMA_GC_NUM_TESTED is satisfied.
        When it is set to negative value, it means runtime will not stop
        checking the operation list until it reaches the end of the list.
        When it is set to positive value, it means runtime will not stop
        checking the operation list until it finds certain number of
        completed requests. When it is set to zero value, the outcome is
        undefined.
        Note that in garbage collection function, if runtime finds a chain
        of completed RMA requests, it will temporarily ignore this CVAR
        and try to find continuous completed requests as many as possible,
        until it meets an incomplete request.

    - name        : MPIR_CVAR_CH3_RMA_GC_NUM_TESTED
      category    : CH3
      type        : int
      default     : 100
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Threshold for the number of RMA requests the runtime tests before
        it stops trying to check more requests in garbage collection
        routine.
        Note that it works with MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED as an
        OR relation, which means runtime will stop checking when either
        one of its following conditions is satisfied or one of conditions
        of MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED is satisfied.
        When it is set to negative value, runtime will not stop checking
        operation list until runtime reaches the end of the list. It has
        the risk of O(N) traversing overhead if there is no completed
        request in the list. When it is set to positive value, it means
        runtime will not stop checking the operation list until it visits
        such number of requests. Higher values may make more completed
        requests to be found, but it has the risk of visiting too many
        requests, leading to significant performance overhead. When it is
        set to zero value, runtime will stop checking the operation list
        immediately, which may cause weird performance in practice.
        Note that in garbage collection function, if runtime finds a chain
        of completed RMA requests, it will temporarily ignore this CVAR and
        try to find continuous completed requests as many as possible, until
        it meets an incomplete request.

    - name        : MPIR_CVAR_CH3_RMA_LOCK_IMMED
      category    : CH3
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Issue a request for the passive target RMA lock immediately.  Default
        behavior is to defer the lock request until the call to MPI_Win_unlock.

    - name        : MPIR_CVAR_CH3_RMA_MERGE_LOCK_OP_UNLOCK
      category    : CH3
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable/disable an optimization that merges lock, op, and unlock
        messages, for single-operation passive target epochs.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* Notes for memory barriers in RMA synchronizations

   When SHM is allocated for RMA window, we need to add memory berriers at proper
   places in RMA synchronization routines to guarantee the ordering of read/write
   operations, so that any operations after synchronization calls will see the
   correct data.

   There are four kinds of operations involved in the following explanation:

   1. Local loads/stores: any operations happening outside RMA epoch and accessing
      each process's own window memory.

   2. SHM operations: any operations happening inside RMA epoch. They may access
      any processes' window memory, which include direct loads/stores, and
      RMA operations that are internally implemented as direct loads/stores in
      MPI implementation.

   3. PROC_SYNC: synchronzations among processes by sending/recving messages.

   4. MEM_SYNC: a full memory barrier. It ensures the ordering of read/write
      operations on each process.

   (1) FENCE synchronization

              RANK 0                           RANK 1

       (local loads/stores)             (local loads/stores)

           WIN_FENCE {                    WIN_FENCE {
               MEM_SYNC                       MEM_SYNC
               PROC_SYNC -------------------- PROC_SYNC
               MEM_SYNC                       MEM_SYNC
           }                              }

        (SHM operations)                  (SHM operations)

           WIN_FENCE {                     WIN_FENCE {
               MEM_SYNC                        MEM_SYNC
               PROC_SYNC --------------------- PROC_SYNC
               MEM_SYNC                        MEM_SYNC
           }                               }

      (local loads/stores)              (local loads/stores)

       We need MEM_SYNC before and after PROC_SYNC for both starting WIN_FENCE
       and ending WIN_FENCE, to ensure the ordering between local loads/stores
       and PROC_SYNC in starting WIN_FENCE (and vice versa in ending WIN_FENCE),
       and the ordering between PROC_SYNC and SHM operations in starting WIN_FENCE
       (and vice versa for ending WIN_FENCE).

       In starting WIN_FENCE, the MEM_SYNC before PROC_SYNC essentially exposes
       previous local loads/stores to other processes; after PROC_SYNC, each
       process knows that everyone else already exposed their local loads/stores;
       the MEM_SYNC after PROC_SYNC ensures that my following SHM operations will
       happen after PROC_SYNC and will see the latest data on other processes.

       In ending WIN_FENCE, the MEM_SYNC before PROC_SYNC essentially exposes
       previous SHM operations to other processes; after PROC_SYNC, each process
       knows everyone else already exposed their SHM operations; the MEM_SYNC
       after PROC_SYNC ensures that my following local loads/stores will happen
       after PROC_SYNC and will see the latest data in my memory region.

   (2) POST-START-COMPLETE-WAIT synchronization

              RANK 0                           RANK 1

                                          (local loads/stores)

           WIN_START {                      WIN_POST {
                                                MEM_SYNC
               PROC_SYNC ---------------------- PROC_SYNC
               MEM_SYNC
           }                                }

         (SHM operations)

           WIN_COMPLETE {                  WIN_WAIT/TEST {
               MEM_SYNC
               PROC_SYNC --------------------- PROC_SYNC
                                               MEM_SYNC
           }                               }

                                          (local loads/stores)

       We need MEM_SYNC before PROC_SYNC for WIN_POST and WIN_COMPLETE, and
       MEM_SYNC after PROC_SYNC in WIN_START and WIN_WAIT/TEST, to ensure the
       ordering between local loads/stores and PROC_SYNC in WIN_POST (and
       vice versa in WIN_WAIT/TEST), and the ordering between PROC_SYNC and SHM
       operations in WIN_START (and vice versa in WIN_COMPLETE).

       In WIN_POST, the MEM_SYNC before PROC_SYNC essentially exposes previous
       local loads/stores to group of origin processes; after PROC_SYNC, origin
       processes knows all target processes already exposed their local
       loads/stores; in WIN_START, the MEM_SYNC after PROC_SYNC ensures that
       following SHM operations will happen after PROC_SYNC and will see the
       latest data on target processes.

       In WIN_COMPLETE, the MEM_SYNC before PROC_SYNC essentailly exposes previous
       SHM operations to group of target processes; after PROC_SYNC, target
       processes knows all origin process already exposed their SHM operations;
       in WIN_WAIT/TEST, the MEM_SYNC after PROC_SYNC ensures that following local
       loads/stores will happen after PROC_SYNC and will see the latest data in
       my memory region.

   (3) Passive target synchronization

              RANK 0                          RANK 1

                                        WIN_LOCK(target=1) {
                                            PROC_SYNC (lock granted)
                                            MEM_SYNC
                                        }

                                        (SHM operations)

                                        WIN_UNLOCK(target=1) {
                                            MEM_SYNC
                                            PROC_SYNC (lock released)
                                        }

         PROC_SYNC -------------------- PROC_SYNC

         WIN_LOCK (target=1) {
             PROC_SYNC (lock granted)
             MEM_SYNC
         }

         (SHM operations)

         WIN_UNLOCK (target=1) {
             MEM_SYNC
             PROC_SYNC (lock released)
         }

         PROC_SYNC -------------------- PROC_SYNC

                                        WIN_LOCK(target=1) {
                                            PROC_SYNC (lock granted)
                                            MEM_SYNC
                                        }

                                        (SHM operations)

                                        WIN_UNLOCK(target=1) {
                                            MEM_SYNC
                                            PROC_SYNC (lock released)
                                        }

         We need MEM_SYNC after PROC_SYNC in WIN_LOCK, and MEM_SYNC before
         PROC_SYNC in WIN_UNLOCK, to ensure the ordering between SHM operations
         and PROC_SYNC and vice versa.

         In WIN_LOCK, the MEM_SYNC after PROC_SYNC guarantees two things:
         (a) it guarantees that following SHM operations will happen after
         lock is granted; (b) it guarantees that following SHM operations
         will happen after any PROC_SYNC with target before WIN_LOCK is called,
         which means those SHM operations will see the latest data on target
         process.

         In WIN_UNLOCK, the MEM_SYNC before PROC_SYNC also guarantees two
         things: (a) it guarantees that SHM operations will happen before
         lock is released; (b) it guarantees that SHM operations will happen
         before any PROC_SYNC with target after WIN_UNLOCK is returned, which
         means following SHM operations on that target will see the latest data.

         WIN_LOCK_ALL/UNLOCK_ALL are same with WIN_LOCK/UNLOCK.

              RANK 0                          RANK 1

         WIN_LOCK_ALL

         (SHM operations)

         WIN_FLUSH(target=1) {
             MEM_SYNC
         }

         PROC_SYNC ------------------------PROC_SYNC

                                           WIN_LOCK(target=1) {
                                               PROC_SYNC (lock granted)
                                               MEM_SYNC
                                           }

                                           (SHM operations)

                                           WIN_UNLOCK(target=1) {
                                               MEM_SYNC
                                               PROC_SYNC (lock released)
                                           }

         WIN_UNLOCK_ALL

         We need MEM_SYNC in WIN_FLUSH to ensure the ordering between SHM
         operations and PROC_SYNC.

         The MEM_SYNC in WIN_FLUSH guarantees that all SHM operations before
         this WIN_FLUSH will happen before any PROC_SYNC with target after
         this WIN_FLUSH, which means SHM operations on target process after
         PROC_SYNC with origin will see the latest data.
*/

void MPIDI_CH3_RMA_Init_Pvars(void)
{
}

/* These are used to use a common routine to complete lists of RMA
   operations with a single routine, while collecting data that
   distinguishes between different synchronization modes.  This is not
   thread-safe; the best choice for thread-safety is to eliminate this
   ability to discriminate between the different types of RMA synchronization.
*/

/*
 * These routines provide a default implementation of the MPI RMA operations
 * in terms of the low-level, two-sided channel operations.  A channel
 * may override these functions, on a per-window basis, by overriding
 * the MPID functions in the RMAFns section of MPID_Win object.
 */

#define SYNC_POST_TAG 100

static int wait_for_lock_granted(MPID_Win * win_ptr, int target_rank);
static int do_passive_target_rma(MPID_Win * win_ptr, int target_rank,
                                 int *wait_for_rma_done_pkt, MPIDI_CH3_Pkt_flags_t sync_flags);
static int send_lock_put_or_acc(MPID_Win *, int);
static int send_lock_get(MPID_Win *, int);
static inline int rma_list_complete(MPID_Win * win_ptr, MPIDI_RMA_Ops_list_t * ops_list,
                                    MPIDI_RMA_Ops_list_t * ops_list_tail);
static inline int rma_list_gc(MPID_Win * win_ptr,
                              MPIDI_RMA_Ops_list_t * ops_list,
                              MPIDI_RMA_Ops_list_t * ops_list_tail,
                              MPIDI_RMA_Op_t * last_elm, int *nDone);


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_fence(int assert, MPID_Win * win_ptr)
{
    int i, made_progress = 0;
    int local_completed = 0, remote_completed = 0;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int errflag = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FENCE);

    MPIU_ERR_CHKANDJUMP((win_ptr->states.access_state != MPIDI_RMA_NONE &&
                         win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                         win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED) ||
                        win_ptr->states.exposure_state != MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    win_ptr->posted_ops_cnt = 0;

    if (assert & MPI_MODE_NOPRECEDE) {
        if (assert & MPI_MODE_NOSUCCEED) {
            goto fn_exit;
        }
        else {
            /* It is possible that there is a IBARRIER in MPI_WIN_FENCE with
               MODE_NOPRECEDE not being completed, we let the progress engine
               to delete its request when it is completed. */
            if (win_ptr->fence_sync_req != MPI_REQUEST_NULL) {
                MPID_Request *req_ptr;
                MPID_Request_get_ptr(win_ptr->fence_sync_req, req_ptr);
                MPID_Request_release(req_ptr);
                win_ptr->fence_sync_req = MPI_REQUEST_NULL;
                win_ptr->states.access_state = MPIDI_RMA_NONE;
            }

            if (win_ptr->shm_allocated == TRUE) {
                MPID_Comm *node_comm_ptr = win_ptr->comm_ptr->node_comm;

                /* Ensure ordering of load/store operations. */
                OPA_read_write_barrier();

                mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

                /* Ensure ordering of load/store operations. */
                OPA_read_write_barrier();
            }

            mpi_errno = MPIR_Ibarrier_impl(win_ptr->comm_ptr, &(win_ptr->fence_sync_req));
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            win_ptr->states.access_state = MPIDI_RMA_FENCE_ISSUED;
            num_active_issued_win++;

            goto fn_exit;
        }
    }

    if (win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED) {
        while (win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    }

    /* Set sync_flag in target structs. */
    for (i = 0; i < win_ptr->num_slots; i++) {
        curr_target = win_ptr->slots[i].target_list;
        while (curr_target != NULL) {

            /* set sync_flag in sync struct */
            if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
                curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
                curr_target->sync.have_remote_incomplete_ops = 0;
                curr_target->sync.outstanding_acks++;
            }
            curr_target = curr_target->next;
        }
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr,
                                                   &local_completed,
                                                   &remote_completed);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        if (!remote_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    } while (!remote_completed);

    /* Cleanup all targets on window. */
    mpi_errno = MPIDI_CH3I_RMA_Cleanup_targets_win(win_ptr);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(win_ptr->non_empty_slots == 0);

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    mpi_errno = MPIR_Barrier_impl(win_ptr->comm_ptr, &errflag);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    if (assert & MPI_MODE_NOSUCCEED) {
        win_ptr->states.access_state = MPIDI_RMA_NONE;
    }
    else {
        win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
    }

    /* There should be no active requests. */
    MPIU_Assert(win_ptr->active_req_cnt == 0);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FENCE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


static int fill_ranks_in_win_grp(MPID_Win *win_ptr, MPID_Group *group_ptr, int *ranks_in_win_grp)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ranks_in_grp;
    MPID_Group *win_grp_ptr;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIU_CHKLMEM_MALLOC(ranks_in_grp, int *, group_ptr->size * sizeof(int),
                        mpi_errno, "ranks_in_grp");
    for (i = 0; i < group_ptr->size; i++) ranks_in_grp[i] = i;

    mpi_errno = MPIR_Comm_group_impl(win_ptr->comm_ptr, &win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr, group_ptr->size,
                                                ranks_in_grp, win_grp_ptr, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_FILL_RANKS_IN_WIN_GRP);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_post(MPID_Group * post_grp_ptr, int assert, MPID_Win * win_ptr)
{
    int *post_ranks_in_win_grp;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_POST);

    /* Note that here we cannot distinguish if this exposure epoch is overlapped
       with an exposure epoch of FENCE (which is not allowed), since FENCE may be
       ended up with not unsetting the window state. We can only detect if this
       exposure epoch is overlapped with another exposure epoch of PSCW. */
    MPIU_ERR_CHKANDJUMP(win_ptr->states.exposure_state != MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    win_ptr->states.exposure_state = MPIDI_RMA_PSCW_EXPO;

    win_ptr->at_completion_counter += post_grp_ptr->size;

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    if ((assert & MPI_MODE_NOCHECK) == 0) {
        MPI_Request *req;
        MPI_Status *status;
        int i, post_grp_size, dst, rank;
        MPID_Comm *win_comm_ptr;

        /* NOCHECK not specified. We need to notify the source
         * processes that Post has been called. */

        post_grp_size = post_grp_ptr->size;
        win_comm_ptr = win_ptr->comm_ptr;
        rank = win_ptr->comm_ptr->rank;

        MPIU_CHKLMEM_MALLOC(post_ranks_in_win_grp, int *,
                            post_grp_size * sizeof(int), mpi_errno, "post_ranks_in_win_grp");
        mpi_errno = fill_ranks_in_win_grp(win_ptr, post_grp_ptr, post_ranks_in_win_grp);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        MPIU_CHKLMEM_MALLOC(req, MPI_Request *, post_grp_size * sizeof(MPI_Request),
                            mpi_errno, "req");
        MPIU_CHKLMEM_MALLOC(status, MPI_Status *, post_grp_size * sizeof(MPI_Status),
                            mpi_errno, "status");

        /* Send a 0-byte message to the source processes */
        for (i = 0; i < post_grp_size; i++) {
            dst = post_ranks_in_win_grp[i];

            if (dst != rank) {
                MPID_Request *req_ptr;
                mpi_errno = MPID_Isend(&i, 0, MPI_INT, dst, SYNC_POST_TAG, win_comm_ptr,
                                       MPID_CONTEXT_INTRA_PT2PT, &req_ptr);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                req[i] = req_ptr->handle;
            }
            else {
                req[i] = MPI_REQUEST_NULL;
            }
        }

        mpi_errno = MPIR_Waitall_impl(post_grp_size, req, status);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
            MPIU_ERR_POP(mpi_errno);

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < post_grp_size; i++) {
                if (status[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = status[i].MPI_ERROR;
                    MPIU_ERR_POP(mpi_errno);
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_POST);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_start(MPID_Group * group_ptr, int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(2);
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_START);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_START);

    /* Note that here we cannot distinguish if this access epoch is overlapped
       with an access epoch of FENCE (which is not allowed), since FENCE may be
       ended up with not unsetting the window state. We can only detect if this
       access epoch is overlapped with another access epoch of PSCW or Passive
       Target. */
    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_NONE &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    win_ptr->start_grp_size = group_ptr->size;

    if ((assert & MPI_MODE_NOCHECK) == 0) {
        int i, intra_cnt, inter_cnt;
        MPI_Request *intra_start_req = NULL;
        MPI_Status *intra_start_status = NULL;
        MPID_Comm *comm_ptr = win_ptr->comm_ptr;
        int rank = comm_ptr->rank;

        /* wait for messages from local processes */
        MPIU_CHKPMEM_MALLOC(win_ptr->start_ranks_in_win_grp, int *, win_ptr->start_grp_size * sizeof(int),
                            mpi_errno, "win_ptr->start_ranks_in_win_grp");
        mpi_errno = fill_ranks_in_win_grp(win_ptr, group_ptr, win_ptr->start_ranks_in_win_grp);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* post IRECVs */
        MPIU_CHKPMEM_MALLOC(win_ptr->start_req, MPI_Request *,
                            win_ptr->start_grp_size * sizeof(MPI_Request),
                            mpi_errno, "win_ptr->start_req");

        if (win_ptr->shm_allocated == TRUE) {
            int node_comm_size = comm_ptr->node_comm->local_size;
            MPIU_CHKLMEM_MALLOC(intra_start_req, MPI_Request *,
                                node_comm_size * sizeof(MPI_Request),
                                mpi_errno, "intra_start_req");
            MPIU_CHKLMEM_MALLOC(intra_start_status, MPI_Status *,
                                node_comm_size * sizeof(MPI_Status),
                                mpi_errno, "intra_start_status");
        }

        intra_cnt = 0;
        for (i = 0; i < win_ptr->start_grp_size; i++) {
            MPID_Request *req_ptr;
            MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
            int src = win_ptr->start_ranks_in_win_grp[i];

            if (src != rank) {
                MPIDI_Comm_get_vc(comm_ptr, rank, &orig_vc);
                MPIDI_Comm_get_vc(comm_ptr, src, &target_vc);

                mpi_errno = MPID_Irecv(NULL, 0, MPI_INT, src, SYNC_POST_TAG,
                                       comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &req_ptr);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                if (win_ptr->shm_allocated == TRUE &&
                    orig_vc->node_id == target_vc->node_id) {
                    intra_start_req[intra_cnt++] = req_ptr->handle;
                    win_ptr->start_req[i] = MPI_REQUEST_NULL;
                }
                else {
                    win_ptr->start_req[i] = req_ptr->handle;
                    inter_cnt++;
                }
            }
            else {
                win_ptr->start_req[i] = MPI_REQUEST_NULL;
            }
        }

        /* for targets on SHM, waiting until their IRECVs to be finished */
        if (intra_cnt) {
            mpi_errno = MPIR_Waitall_impl(intra_cnt, intra_start_req, intra_start_status);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                MPIU_ERR_POP(mpi_errno);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPI_ERR_IN_STATUS) {
                for (i = 0; i < intra_cnt; i++) {
                    if (intra_start_status[i].MPI_ERROR != MPI_SUCCESS) {
                        mpi_errno = intra_start_status[i].MPI_ERROR;
                        MPIU_ERR_POP(mpi_errno);
                    }
                }
            }
            /* --END ERROR HANDLING-- */
        }

        if (win_ptr->shm_allocated == TRUE) {
            /* Ensure ordering of load/store operations */
            OPA_read_write_barrier();
        }
    }

    win_ptr->states.access_state = MPIDI_RMA_PSCW_ISSUED;
    num_active_issued_win++;

    MPIU_Assert(win_ptr->posted_ops_cnt == 0);
    MPIU_Assert(win_ptr->active_req_cnt == 0);

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_START);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_complete(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, dst, rank = win_ptr->comm_ptr->rank;
    int local_completed = 0, remote_completed = 0;
    MPID_Comm *win_comm_ptr = win_ptr->comm_ptr;
    MPIDI_RMA_Target_t *curr_target;
    int made_progress;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_COMPLETE);

    /* Access epochs on the same window must be disjoint. */
    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PSCW_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_PSCW_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED) {
        while (win_ptr->states.access_state != MPIDI_RMA_PSCW_GRANTED) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    }

    for (i = 0; i < win_ptr->start_grp_size; i++) {
        dst = win_ptr->start_ranks_in_win_grp[i];
        if (dst == rank) {
            win_ptr->at_completion_counter--;
            MPIU_Assert(win_ptr->at_completion_counter >= 0);
            continue;
        }

        if (win_comm_ptr->local_size <= win_ptr->num_slots)
            curr_target = win_ptr->slots[dst].target_list;
        else {
            curr_target = win_ptr->slots[dst % win_ptr->num_slots].target_list;
            while (curr_target != NULL && curr_target->target_rank != dst)
                curr_target = curr_target->next;
        }

        if (curr_target != NULL) {
            /* set sync_flag in sync struct */
            if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
                curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
                curr_target->sync.have_remote_incomplete_ops = 0;
                curr_target->sync.outstanding_acks++;
            }
            curr_target->win_complete_flag = 1;
        }
        else {
            /* FIXME: do we need to wait for remote completion? */
            mpi_errno = send_decr_at_cnt_msg(dst, win_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }

    /* issue out all operations */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* wait until all slots are empty */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                   &remote_completed);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        if (!remote_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    } while (!remote_completed);

    /* Cleanup all targets on this window. */
    mpi_errno = MPIDI_CH3I_RMA_Cleanup_targets_win(win_ptr);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(win_ptr->non_empty_slots == 0);

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    /* free start group stored in window */
    MPIU_Free(win_ptr->start_ranks_in_win_grp);
    win_ptr->start_ranks_in_win_grp = NULL;

    win_ptr->posted_ops_cnt = 0;
    MPIU_Assert(win_ptr->active_req_cnt == 0);
    MPIU_Assert(win_ptr->start_req == NULL);

    win_ptr->states.access_state = MPIDI_RMA_NONE;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_COMPLETE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Win_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_wait(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_WAIT);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.exposure_state != MPIDI_RMA_PSCW_EXPO,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* wait for all operations from other processes to finish */
    while (win_ptr->at_completion_counter) {
        mpi_errno = wait_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    win_ptr->states.exposure_state = MPIDI_RMA_NONE;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_WAIT);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_test
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_test(MPID_Win * win_ptr, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_TEST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_TEST);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.exposure_state != MPIDI_RMA_PSCW_EXPO,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    *flag = (win_ptr->at_completion_counter) ? 0 : 1;
    if (*flag) {
        /* Ensure ordering of load/store operations. */
        if (win_ptr->shm_allocated == TRUE) {
            OPA_read_write_barrier();
        }

        win_ptr->states.exposure_state = MPIDI_RMA_NONE;
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_TEST);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_lock(int lock_type, int dest, int assert, MPID_Win * win_ptr)
{
    int made_progress = 0;
    int shm_target = FALSE;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *target = NULL;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_LOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_LOCK);

    /* Note that here we cannot distinguish if this access epoch is overlapped
       with an access epoch of FENCE (which is not allowed), since FENCE may be
       ended up with not unsetting the window state. We can only detect if this
       access epoch is overlapped with another access epoch of PSCW or Passive
       Target. */
    if (win_ptr->lock_epoch_count == 0) {
        MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_NONE &&
                            win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                            win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED,
                            mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");
    }
    else {
        MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_NONE &&
                            win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                            win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED &&
                            win_ptr->states.access_state != MPIDI_RMA_PER_TARGET,
                            mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");
    }

    if (dest != MPI_PROC_NULL) {
        /* check if we lock the same target window more than once. */
        mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        MPIU_ERR_CHKANDJUMP(target != NULL, mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");
    }

    /* Error handling is finished. */

    if (win_ptr->lock_epoch_count == 0) {
        win_ptr->states.access_state = MPIDI_RMA_PER_TARGET;
        num_passive_win++;
    }
    win_ptr->lock_epoch_count++;

    if (dest == MPI_PROC_NULL)
        goto fn_exit;

    if (win_ptr->shm_allocated == TRUE) {
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        if (orig_vc->node_id == target_vc->node_id)
            shm_target = TRUE;
    }

    /* Create a new target. */
    mpi_errno = MPIDI_CH3I_Win_create_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Store lock_state (CALLED/ISSUED/GRANTED), lock_type (SHARED/EXCLUSIVE),
       lock_mode (MODE_NOCHECK). */
    if (assert & MPI_MODE_NOCHECK)
        target->access_state = MPIDI_RMA_LOCK_GRANTED;
    else
        target->access_state = MPIDI_RMA_LOCK_CALLED;
    target->lock_type = lock_type;
    target->lock_mode = assert;

    /* If Destination is myself or process on SHM, acquire the lock,
       wait until lock is granted. */
    if (!(assert & MPI_MODE_NOCHECK) && (dest == rank || shm_target)) {
        mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest, &made_progress);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        while (target->access_state != MPIDI_RMA_LOCK_GRANTED) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_LOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Win_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int made_progress = 0;
    int local_completed = 0, remote_completed = 0;
    MPIDI_RMA_Target_t *target = NULL;
    enum MPIDI_RMA_sync_types sync_flag;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated) {
        OPA_read_write_barrier();
    }

    if (dest == MPI_PROC_NULL)
        goto finish_unlock;

    /* When the process tries to acquire the lock on itself, it does not
       go through the progress engine. Therefore, it is possible that
       one process always grants the lock to itself but never process
       events coming from other processes. This may cause deadlock in
       applications where the program execution on target process depends
       on the happening of events from other processes. Here we poke
       the progress engine once to avoid such issue.  */
    mpi_errno = poke_progress_engine();
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Find or recreate target. */
    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (target == NULL) {
        mpi_errno = MPIDI_CH3I_Win_create_target(win_ptr, dest, &target);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
        target->access_state = MPIDI_RMA_LOCK_GRANTED;
    }

    /* Set sync_flag in sync struct. */
    if (target->lock_mode & MPI_MODE_NOCHECK)
        sync_flag = MPIDI_RMA_SYNC_FLUSH;
    else
        sync_flag = MPIDI_RMA_SYNC_UNLOCK;
    if (target->sync.sync_flag < sync_flag) {
        target->sync.sync_flag = sync_flag;
        target->sync.have_remote_incomplete_ops = 0;
        target->sync.outstanding_acks++;
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest,
                                                    &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_target(win_ptr, target,
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
    mpi_errno = MPIDI_CH3I_RMA_Cleanup_single_target(win_ptr, target);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 finish_unlock:
    win_ptr->posted_ops_cnt = 0;
    MPIU_Assert(win_ptr->active_req_cnt == 0);

    win_ptr->lock_epoch_count--;
    if (win_ptr->lock_epoch_count == 0) {
        win_ptr->states.access_state = MPIDI_RMA_NONE;
        num_passive_win--;
        MPIU_Assert(num_passive_win >= 0);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_UNLOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_flush_all
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_flush_all(MPID_Win * win_ptr)
{
    int i, made_progress = 0;
    int local_completed = 0, remote_completed = 0;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPIDI_STATE_MPIDI_WIN_FLUSH_ALL);

    MPIDI_RMA_FUNC_ENTER(MPIDI_STATE_MPIDI_WIN_FLUSH_ALL);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    /* When the process tries to acquire the lock on itself, it does not
       go through the progress engine. Therefore, it is possible that
       one process always grants the lock to itself but never process
       events coming from other processes. This may cause deadlock in
       applications where the program execution on target process depends
       on the happening of events from other processes. Here we poke
       the progress engine once to avoid such issue.  */
    mpi_errno = poke_progress_engine();
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Set sync_flag in sync struct. */
    for (i = 0; i < win_ptr->num_slots; i++) {
        curr_target = win_ptr->slots[i].target_list;
        while (curr_target != NULL) {
            if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
                curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
                curr_target->sync.have_remote_incomplete_ops = 0;
                curr_target->sync.outstanding_acks++;
            }
            curr_target = curr_target->next;
        }
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                   &remote_completed);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        if (!remote_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    } while (!remote_completed);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPIDI_STATE_MPIDI_WIN_FLUSH_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_flush
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_flush(int dest, MPID_Win *win_ptr)
{
    int made_progress = 0;
    int local_completed = 0, remote_completed = 0;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FLUSH);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated) {
        OPA_read_write_barrier();
    }

    /* When the process tries to acquire the lock on itself, it does not
       go through the progress engine. Therefore, it is possible that
       one process always grants the lock to itself but never process
       events coming from other processes. This may cause deadlock in
       applications where the program execution on target process depends
       on the happening of events from other processes. Here we poke
       the progress engine once to avoid such issue.  */
    mpi_errno = poke_progress_engine();
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    if (rank == dest)
        goto fn_exit;

    if (win_ptr->shm_allocated) {
        MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        if (orig_vc->node_id == target_vc->node_id)
            goto fn_exit;
    }

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (target == NULL)
        goto fn_exit;

    /* Set sync_flag in sync struct. */
    if (target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
        target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
        target->sync.have_remote_incomplete_ops = 0;
        target->sync.outstanding_acks++;
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest,
                                                    &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_target(win_ptr, target,
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

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FLUSH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_flush_local
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_flush_local(int dest, MPID_Win * win_ptr)
{
    int made_progress = 0;
    int local_completed = 0, remote_completed = 0;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* When the process tries to acquire the lock on itself, it does not
       go through the progress engine. Therefore, it is possible that
       one process always grants the lock to itself but never process
       events coming from other processes. This may cause deadlock in
       applications where the program execution on target process depends
       on the happening of events from other processes. Here we poke
       the progress engine once to avoid such issue.  */
    mpi_errno = poke_progress_engine();
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    if (rank == dest)
        goto fn_exit;

    if (win_ptr->shm_allocated) {
        MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        if (orig_vc->node_id == target_vc->node_id)
            goto fn_exit;
    }

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    if (target == NULL)
        goto fn_exit;

    /* Set sync_flag in sync struct. */
    if (target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH_LOCAL)
        target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH_LOCAL;

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest,
                                                    &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Wait for local completion. */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_target(win_ptr, target,
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
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_flush_local_all
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_flush_local_all(MPID_Win * win_ptr)
{
    int i, made_progress = 0;
    int local_completed = 0, remote_completed = 0;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* When the process tries to acquire the lock on itself, it does not
       go through the progress engine. Therefore, it is possible that
       one process always grants the lock to itself but never process
       events coming from other processes. This may cause deadlock in
       applications where the program execution on target process depends
       on the happening of events from other processes. Here we poke
       the progress engine once to avoid such issue.  */
    mpi_errno = poke_progress_engine();
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Set sync_flag in sync struct. */
    for (i = 0; i < win_ptr->num_slots; i++) {
        curr_target = win_ptr->slots[i].target_list;
        while (curr_target != NULL) {
            if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH_LOCAL) {
                curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH_LOCAL;
            }
            curr_target = curr_target->next;
        }
    }

    /* issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Wait for local completion. */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
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
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_lock_all
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_lock_all(int assert, MPID_Win * win_ptr)
{
    int i, rank = win_ptr->comm_ptr->rank;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_LOCK_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_LOCK_ALL);

    /* Note that here we cannot distinguish if this access epoch is overlapped
       with an access epoch of FENCE (which is not allowed), since FENCE may be
       ended up with not unsetting the window state. We can only detect if this
       access epoch is overlapped with another access epoch of PSCW or Passive
       Target. */
    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_NONE &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (assert & MPI_MODE_NOCHECK)
        win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_GRANTED;
    else
        win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_CALLED;
    num_passive_win++;

    win_ptr->lock_all_assert = assert;

    MPIU_Assert(win_ptr->outstanding_locks == 0);

    /* Acquire the lock on myself and the lock on processes on SHM.
       No need to create a target for them. */
    if (!(win_ptr->lock_all_assert & MPI_MODE_NOCHECK)) {
        win_ptr->outstanding_locks++;
        mpi_errno = acquire_local_lock(win_ptr, MPI_LOCK_SHARED);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (win_ptr->shm_allocated == TRUE) {
            MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
            for (i = 0; i < win_ptr->comm_ptr->local_size; i++) {
                if (i == rank)
                    continue;
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
                if (orig_vc->node_id == target_vc->node_id) {
                    win_ptr->outstanding_locks++;
                    mpi_errno = send_lock_msg(i, MPI_LOCK_SHARED, win_ptr);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIU_ERR_POP(mpi_errno);
                }
            }
        }

        /* wait for lock to be granted */
        while (win_ptr->outstanding_locks > 0) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_LOCK_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_unlock_all
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_unlock_all(MPID_Win * win_ptr)
{
    int i, made_progress = 0;
    int local_completed = 0,remote_completed = 0;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *curr_target = NULL;
    enum MPIDI_RMA_sync_types sync_flag;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated) {
        OPA_read_write_barrier();
    }

    MPIU_Assert(win_ptr->outstanding_unlocks == 0);

    /* Unlock MYSELF and processes on SHM. */
    if (!(win_ptr->lock_all_assert & MPI_MODE_NOCHECK)) {
        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);

        if (win_ptr->shm_allocated == TRUE) {
            MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
            for (i = 0; i < win_ptr->comm_ptr->local_size; i++) {
                if (i == rank) continue;
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
                if (orig_vc->node_id == target_vc->node_id) {
                    win_ptr->outstanding_unlocks++;
                    mpi_errno = send_unlock_msg(i, win_ptr);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIU_ERR_POP(mpi_errno);
                }
            }
        }
    }

    /* Set sync_flag in sync struct. */
    if (win_ptr->lock_all_assert & MPI_MODE_NOCHECK)
        sync_flag = MPIDI_RMA_SYNC_FLUSH;
    else
        sync_flag = MPIDI_RMA_SYNC_UNLOCK;

    if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
        for (i = 0; i < win_ptr->num_slots; i++) {
            curr_target = win_ptr->slots[i].target_list;
            while (curr_target != NULL) {
                if (curr_target->sync.sync_flag < sync_flag) {
                    curr_target->sync.sync_flag = sync_flag;
                    curr_target->sync.have_remote_incomplete_ops = 0;
                    curr_target->sync.outstanding_acks++;
                }
                curr_target = curr_target->next;
            }
        }
    }
    else {
        for (i = 0; i < win_ptr->comm_ptr->local_size; i++) {
            if (win_ptr->comm_ptr->local_size <= win_ptr->num_slots)
                curr_target = win_ptr->slots[i].target_list;
            else {
                curr_target = win_ptr->slots[i % win_ptr->num_slots].target_list;
                while (curr_target != NULL && curr_target->target_rank != i)
                    curr_target = curr_target->next;
            }

            if (curr_target != NULL) {
                if (curr_target->sync.sync_flag < sync_flag) {
                    curr_target->sync.sync_flag = sync_flag;
                    curr_target->sync.have_remote_incomplete_ops = 0;
                    curr_target->sync.outstanding_acks++;
                }
            }
            else {
                if (win_ptr->lock_all_assert & MPI_MODE_NOCHECK)
                    continue;
                if (i == rank)
                    continue;
                if (win_ptr->shm_allocated == TRUE) {
                    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
                    MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
                    MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
                    if (orig_vc->node_id == target_vc->node_id)
                        continue;
                }

                win_ptr->outstanding_unlocks++;
                mpi_errno = send_unlock_msg(i, win_ptr);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
            }
        }
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_win(win_ptr, &local_completed,
                                                   &remote_completed);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        if (!remote_completed || win_ptr->outstanding_unlocks) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }
    } while (!remote_completed || win_ptr->outstanding_unlocks);

    /* Cleanup all targets on this window. */
    mpi_errno = MPIDI_CH3I_RMA_Cleanup_targets_win(win_ptr);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(win_ptr->non_empty_slots == 0);

    win_ptr->lock_all_assert = 0;
    win_ptr->posted_ops_cnt = 0;
    MPIU_Assert(win_ptr->active_req_cnt == 0);

    win_ptr->states.access_state = MPIDI_EPOCH_NONE;
    num_passive_win--;
    MPIU_Assert(num_passive_win >= 0);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME do_passive_target_rma
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_passive_target_rma(MPID_Win * win_ptr, int target_rank,
                                 int *wait_for_rma_done_pkt, MPIDI_CH3_Pkt_flags_t sync_flags)
{
    int mpi_errno = MPI_SUCCESS, nops;
    MPIDI_RMA_Op_t *curr_ptr;
    int nRequest = 0, nRequestNew = 0;
    MPIDI_STATE_DECL(MPID_STATE_DO_PASSIVE_TARGET_RMA);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_DO_PASSIVE_TARGET_RMA);

    MPIU_Assert(win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_GRANTED ||
                win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_FLUSH ||
                (win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED &&
                 win_ptr->targets[target_rank].remote_lock_assert & MPI_MODE_NOCHECK));

    if (MPIDI_CH3I_RMA_Ops_isempty(&win_ptr->targets[target_rank].rma_ops_list)) {
        /* The ops list is empty -- NOTE: we assume this is because the epoch
         * was flushed.  Any issued ops are already remote complete; done
         * packet is not needed for safe third party communication. */
        *wait_for_rma_done_pkt = 0;
    }
    else {
        MPIDI_RMA_Op_t *tail = MPIDI_CH3I_RMA_Ops_tail(&win_ptr->targets[target_rank].rma_ops_list_tail);

        /* Check if we can piggyback the RMA done acknowlegdement on the last
         * operation in the epoch. */

        if (tail->pkt.type == MPIDI_CH3_PKT_GET ||
            tail->pkt.type == MPIDI_CH3_PKT_CAS ||
            tail->pkt.type == MPIDI_CH3_PKT_FOP || tail->pkt.type == MPIDI_CH3_PKT_GET_ACCUM) {
            /* last operation sends a response message. no need to wait
             * for an additional rma done pkt */
            *wait_for_rma_done_pkt = 0;
        }
        else {
            /* Check if there is a get operation, which can be be performed
             * moved to the end to piggyback the RMA done acknowledgement.  Go
             * through the list and move the first get operation (if there is
             * one) to the end. */

            *wait_for_rma_done_pkt = 1;
            curr_ptr = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

            while (curr_ptr != NULL) {
                if (curr_ptr->pkt.type == MPIDI_CH3_PKT_GET) {
                    /* Found a GET, move it to the end */
                    *wait_for_rma_done_pkt = 0;

                    MPIDI_CH3I_RMA_Ops_unlink(&win_ptr->targets[target_rank].rma_ops_list,
                                              &win_ptr->targets[target_rank].rma_ops_list_tail,
                                              curr_ptr);
                    MPIDI_CH3I_RMA_Ops_append(&win_ptr->targets[target_rank].rma_ops_list,
                                              &win_ptr->targets[target_rank].rma_ops_list_tail,
                                              curr_ptr);
                    break;
                }
                else {
                    curr_ptr = curr_ptr->next;
                }
            }
        }
    }

    curr_ptr = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    nops = 0;
    while (curr_ptr != NULL) {
        nops++;
        curr_ptr = curr_ptr->next;
    }

    curr_ptr = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    while (curr_ptr != NULL) {
        MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;

        /* Assertion: (curr_ptr != NULL) => (nops > 0) */
        MPIU_Assert(nops > 0);
        MPIU_Assert(curr_ptr->target_rank == target_rank);

        /* Piggyback the lock operation on the first op */
        if (win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED) {
            MPIU_Assert(win_ptr->targets[target_rank].remote_lock_assert & MPI_MODE_NOCHECK);
            flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK | MPIDI_CH3_PKT_FLAG_RMA_NOCHECK;

            switch (win_ptr->targets[target_rank].remote_lock_mode) {
            case MPI_LOCK_SHARED:
                flags |= MPIDI_CH3_PKT_FLAG_RMA_SHARED;
                break;
            case MPI_LOCK_EXCLUSIVE:
                flags |= MPIDI_CH3_PKT_FLAG_RMA_EXCLUSIVE;
                break;
            default:
                MPIU_Assert(0);
                break;
            }

            win_ptr->targets[target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
        }

        /* Piggyback the unlock/flush operation on the last op */
        if (curr_ptr->next == NULL) {
            if (sync_flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;
            }
            else if (sync_flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            }
            else {
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_RMA_SYNC, "**ch3|sync_arg",
                                     "**ch3|sync_arg %d", sync_flags);
            }

            /* Inform the target that we want an acknowledgement when the
             * unlock has completed. */
            if (*wait_for_rma_done_pkt) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
            }
        }

        mpi_errno = MPIDI_CH3I_Issue_rma_op(curr_ptr, win_ptr, flags);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        /* If the request is null, we can remove it immediately */
        if (!curr_ptr->request) {
            MPIDI_CH3I_RMA_Ops_free_and_next(win_ptr, &win_ptr->targets[target_rank].rma_ops_list,
                                             &win_ptr->targets[target_rank].rma_ops_list_tail,
                                             &curr_ptr);
        }
        else {
            nRequest++;
            curr_ptr = curr_ptr->next;
            if (nRequest > MPIR_CVAR_CH3_RMA_NREQUEST_THRESHOLD &&
                nRequest - nRequestNew > MPIR_CVAR_CH3_RMA_NREQUEST_NEW_THRESHOLD) {
                int nDone = 0;
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                mpi_errno =
                    rma_list_gc(win_ptr, &win_ptr->targets[target_rank].rma_ops_list,
                                &win_ptr->targets[target_rank].rma_ops_list_tail, curr_ptr,
                                &nDone);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                /* if (nDone > 0) printf("nDone = %d\n", nDone); */
                nRequest -= nDone;
                nRequestNew = nRequest;
            }
        }
    }

    if (nops) {
        mpi_errno = rma_list_complete(win_ptr, &win_ptr->targets[target_rank].rma_ops_list,
                                      &win_ptr->targets[target_rank].rma_ops_list_tail);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }
    else if (sync_flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
        /* No communication operations were left to process, but the RMA epoch
         * is open.  Send an unlock message to release the lock at the target.  */
        mpi_errno = send_unlock_msg(target_rank, win_ptr);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
        *wait_for_rma_done_pkt = 1;
    }
    /* NOTE: Flush -- If RMA ops are issued eagerly, Send_flush_msg should be
     * called here and wait_for_rma_done_pkt should be set. */

    /* MT: avoid processing unissued operations enqueued by other threads
       in rma_list_complete() */
    curr_ptr = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);
    if (curr_ptr && !curr_ptr->request)
        goto fn_exit;
    MPIU_Assert(MPIDI_CH3I_RMA_Ops_isempty(&win_ptr->targets[target_rank].rma_ops_list));

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_DO_PASSIVE_TARGET_RMA);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Win_sync
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_sync(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_SYNC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_SYNC);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    OPA_read_write_barrier();

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_SYNC);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME wait_for_lock_granted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int wait_for_lock_granted(MPID_Win * win_ptr, int target_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_WAIT_FOR_LOCK_GRANTED);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_WAIT_FOR_LOCK_GRANTED);

    /* After the target grants the lock, it sends a lock_granted packet. This
     * packet is received in ch3u_handle_recv_pkt.c.  The handler for the
     * packet sets the remote_lock_state flag to GRANTED.
     */

    MPIU_Assert(win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_REQUESTED ||
                win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_GRANTED);

    /* poke the progress engine until remote_lock_state flag is set to GRANTED */
    if (win_ptr->targets[target_rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (win_ptr->targets[target_rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Progress_end(&progress_state);
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winnoprogress");
            }
            /* --END ERROR HANDLING-- */
        }
        MPID_Progress_end(&progress_state);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_WAIT_FOR_LOCK_GRANTED);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME send_lock_put_or_acc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_lock_put_or_acc(MPID_Win * win_ptr, int target_rank)
{
    int mpi_errno = MPI_SUCCESS, lock_type, origin_dt_derived, iovcnt;
    MPIDI_RMA_Op_t *rma_op;
    MPID_Request *request = NULL;
    MPIDI_VC_t *vc;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Comm *comm_ptr;
    MPID_Datatype *origin_dtp = NULL;
    MPI_Aint origin_type_size;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_put_unlock_t *lock_put_unlock_pkt = &upkt.lock_put_unlock;
    MPIDI_CH3_Pkt_lock_accum_unlock_t *lock_accum_unlock_pkt = &upkt.lock_accum_unlock;
    MPIDI_CH3_Pkt_put_t *put_pkt;
    MPIDI_CH3_Pkt_accum_t *accum_pkt;
    MPIDI_CH3_Pkt_accum_immed_t *accumi_pkt;

    MPIDI_STATE_DECL(MPID_STATE_SEND_LOCK_PUT_OR_ACC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_LOCK_PUT_OR_ACC);

    lock_type = win_ptr->targets[target_rank].remote_lock_mode;

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    if (rma_op->pkt.type == MPIDI_CH3_PKT_PUT) {
        put_pkt = &rma_op->pkt.put;

        MPIDI_Pkt_init(lock_put_unlock_pkt, MPIDI_CH3_PKT_LOCK_PUT_UNLOCK);
        lock_put_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
            MPIDI_CH3_PKT_FLAG_RMA_UNLOCK | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
        lock_put_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
        lock_put_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_put_unlock_pkt->lock_type = lock_type;
        lock_put_unlock_pkt->origin_rank = win_ptr->comm_ptr->rank;

        lock_put_unlock_pkt->addr = put_pkt->addr;
        lock_put_unlock_pkt->count = put_pkt->count;
        lock_put_unlock_pkt->datatype = put_pkt->datatype;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_put_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_put_unlock_pkt);
    }

    else if (rma_op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE) {
        accum_pkt = &rma_op->pkt.accum;

        MPIDI_Pkt_init(lock_accum_unlock_pkt, MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK);
        lock_accum_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
            MPIDI_CH3_PKT_FLAG_RMA_UNLOCK | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
        lock_accum_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
        lock_accum_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_accum_unlock_pkt->lock_type = lock_type;
        lock_accum_unlock_pkt->origin_rank = win_ptr->comm_ptr->rank;

        lock_accum_unlock_pkt->addr = accum_pkt->addr;
        lock_accum_unlock_pkt->count = accum_pkt->count;
        lock_accum_unlock_pkt->datatype = accum_pkt->datatype;
        lock_accum_unlock_pkt->op = accum_pkt->op;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_accum_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_accum_unlock_pkt);
    }
    else if (rma_op->pkt.type == MPIDI_CH3_PKT_ACCUM_IMMED) {
        accumi_pkt = &rma_op->pkt.accum_immed;

        MPIDI_Pkt_init(lock_accum_unlock_pkt, MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK);
        lock_accum_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
            MPIDI_CH3_PKT_FLAG_RMA_UNLOCK | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
        lock_accum_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
        lock_accum_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_accum_unlock_pkt->lock_type = lock_type;
        lock_accum_unlock_pkt->origin_rank = win_ptr->comm_ptr->rank;

        lock_accum_unlock_pkt->addr = accumi_pkt->addr;
        lock_accum_unlock_pkt->count = accumi_pkt->count;
        lock_accum_unlock_pkt->datatype = accumi_pkt->datatype;
        lock_accum_unlock_pkt->op = accumi_pkt->op;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_accum_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_accum_unlock_pkt);
    }
    else {
        /* FIXME: Error return */
        printf("expected short accumulate...\n");
        /* */
    }

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype)) {
        origin_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }
    else {
        origin_dt_derived = 0;
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!origin_dt_derived) {
        /* basic datatype on origin */

        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) rma_op->origin_addr;
        iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
        iovcnt = 2;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &request);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        /* derived datatype on origin */

        iovcnt = 1;

        request = MPID_Request_create();
        if (request == NULL) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
        }

        MPIU_Object_set_ref(request, 2);
        request->kind = MPID_REQUEST_SEND;

        request->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
         * is freed. */

        request->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1(request->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "MPID_Segment_alloc");

        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype, request->dev.segment_ptr, 0);
        request->dev.segment_first = 0;
        request->dev.segment_size = rma_op->origin_count * origin_type_size;

        request->dev.OnFinal = 0;
        request->dev.OnDataAvail = 0;

        mpi_errno = vc->sendNoncontig_fn(vc, request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno) {
            MPID_Datatype_release(request->dev.datatype_ptr);
            MPID_Request_release(request);
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");
        }
        /* --END ERROR HANDLING-- */
    }

    if (request != NULL) {
        if (!MPID_Request_is_complete(request)) {
            MPID_Progress_state progress_state;

            MPID_Progress_start(&progress_state);
            while (!MPID_Request_is_complete(request)) {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_Progress_end(&progress_state);
                    MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");
                }
                /* --END ERROR HANDLING-- */
            }
            MPID_Progress_end(&progress_state);
        }

        mpi_errno = request->status.MPI_ERROR;
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");
        }

        MPID_Request_release(request);
    }

    /* Free MPIDI_RMA_Ops_list - the lock packet should still be in place, so
     * we have to free two elements. */
    MPIDI_CH3I_RMA_Ops_free(win_ptr, &win_ptr->targets[target_rank].rma_ops_list,
                            &win_ptr->targets[target_rank].rma_ops_list_tail);

  fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_LOCK_PUT_OR_ACC);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME send_lock_get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_lock_get(MPID_Win * win_ptr, int target_rank)
{
    int mpi_errno = MPI_SUCCESS, lock_type;
    MPIDI_RMA_Op_t *rma_op;
    MPID_Request *rreq = NULL, *sreq = NULL;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *dtp;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_get_unlock_t *lock_get_unlock_pkt = &upkt.lock_get_unlock;
    MPIDI_CH3_Pkt_get_t *get_pkt;

    MPIDI_STATE_DECL(MPID_STATE_SEND_LOCK_GET);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_LOCK_GET);

    lock_type = win_ptr->targets[target_rank].remote_lock_mode;

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    /* create a request, store the origin buf, cnt, datatype in it,
     * and pass a handle to it in the get packet. When the get
     * response comes from the target, it will contain the request
     * handle. */
    rreq = MPID_Request_create();
    if (rreq == NULL) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }

    MPIU_Object_set_ref(rreq, 2);

    rreq->dev.user_buf = rma_op->origin_addr;
    rreq->dev.user_count = rma_op->origin_count;
    rreq->dev.datatype = rma_op->origin_datatype;
    rreq->dev.target_win_handle = MPI_WIN_NULL;
    rreq->dev.source_win_handle = win_ptr->handle;

    if (!MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype)) {
        MPID_Datatype_get_ptr(rreq->dev.datatype, dtp);
        rreq->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
         * request is freed. */
    }

    get_pkt = &rma_op->pkt.get;

    MPIDI_Pkt_init(lock_get_unlock_pkt, MPIDI_CH3_PKT_LOCK_GET_UNLOCK);
    lock_get_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK | MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;   /* FIXME | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK; */
    lock_get_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
    lock_get_unlock_pkt->source_win_handle = win_ptr->handle;
    lock_get_unlock_pkt->lock_type = lock_type;
    lock_get_unlock_pkt->origin_rank = win_ptr->comm_ptr->rank;

    lock_get_unlock_pkt->addr = get_pkt->addr;
    lock_get_unlock_pkt->count = get_pkt->count;
    lock_get_unlock_pkt->datatype = get_pkt->datatype;
    lock_get_unlock_pkt->request_handle = rreq->handle;

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_get_unlock_pkt, sizeof(*lock_get_unlock_pkt), &sreq);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    /* release the request returned by iStartMsg */
    if (sreq != NULL) {
        MPID_Request_release(sreq);
    }

    /* now wait for the data to arrive */
    if (!MPID_Request_is_complete(rreq)) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(rreq)) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Progress_end(&progress_state);
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");
            }
            /* --END ERROR HANDLING-- */
        }
        MPID_Progress_end(&progress_state);
    }

    mpi_errno = rreq->status.MPI_ERROR;
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");
    }

    /* if origin datatype was a derived datatype, it will get freed when the
     * rreq gets freed. */
    MPID_Request_release(rreq);

    /* Free MPIDI_RMA_Ops_list - the lock packet should still be in place, so
     * we have to free two elements. */
    MPIDI_CH3I_RMA_Ops_free(win_ptr, &win_ptr->targets[target_rank].rma_ops_list,
                            &win_ptr->targets[target_rank].rma_ops_list_tail);

  fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_LOCK_GET);
    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/* list_complete_timer/counter and list_block_timer defined above */

static inline int rma_list_complete(MPID_Win * win_ptr, MPIDI_RMA_Ops_list_t * ops_list,
                                    MPIDI_RMA_Ops_list_t *ops_list_tail)
{
    int ntimes = 0, mpi_errno = 0;
    MPIDI_RMA_Op_t *curr_ptr;
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    /* Process all operations until they are complete */
    while (!MPIDI_CH3I_RMA_Ops_isempty(ops_list)) {
        int nDone = 0;
        mpi_errno = rma_list_gc(win_ptr, ops_list, ops_list_tail, NULL, &nDone);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
        ntimes++;

        /* Wait for something to arrive */
        /* In some tests, this hung unless the test ensured that
         * there was an incomplete request. */
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);

        /* MT: avoid processing unissued operations enqueued by other
           threads in MPID_Progress_wait() */
        if (curr_ptr && !curr_ptr->request) {
            /* This RMA operation has not been issued yet. */
            break;
        }
        if (curr_ptr && !MPID_Request_is_complete(curr_ptr->request)) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Progress_end(&progress_state);
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winnoprogress");
            }
            /* --END ERROR HANDLING-- */
        }
    }   /* While list of rma operation is non-empty */
    MPID_Progress_end(&progress_state);

  fn_fail:
    return mpi_errno;
}

/* This routine is used to do garbage collection work on completed RMA
   requests so far. It is used to clean up the RMA requests that are
   not completed immediately when issuing out but are completed later
   when poking progress engine, so that they will not waste internal
   resources.
*/
static inline int rma_list_gc(MPID_Win * win_ptr,
                              MPIDI_RMA_Ops_list_t * ops_list,
                              MPIDI_RMA_Ops_list_t * ops_list_tail,
                              MPIDI_RMA_Op_t * last_elm, int *nDone)
{
    int mpi_errno = 0;
    MPIDI_RMA_Op_t *curr_ptr;
    int nComplete = 0;
    int nVisit = 0;

    curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
    do {
        /* MT: avoid processing unissued operations enqueued by other threads
           in rma_list_complete() */
        if (curr_ptr && !curr_ptr->request) {
            /* This RMA operation has not been issued yet. */
            break;
        }
        if (MPID_Request_is_complete(curr_ptr->request)) {
            /* Once we find a complete request, we complete
             * as many as possible until we find an incomplete
             * or null request */
            do {
                nComplete++;
                mpi_errno = curr_ptr->request->status.MPI_ERROR;
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS) {
                    MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");
                }
                /* --END ERROR HANDLING-- */
                MPID_Request_release(curr_ptr->request);
                MPIDI_CH3I_RMA_Ops_free_and_next(win_ptr, ops_list, ops_list_tail, &curr_ptr);
                nVisit++;

                /* MT: avoid processing unissued operations enqueued by other
                   threads in rma_list_complete() */
                if (curr_ptr && !curr_ptr->request) {
                    /* This RMA operation has not been issued yet. */
                    break;
                }
            }
            while (curr_ptr && curr_ptr != last_elm && MPID_Request_is_complete(curr_ptr->request));
            if ((MPIR_CVAR_CH3_RMA_GC_NUM_TESTED >= 0 &&
                 nVisit >= MPIR_CVAR_CH3_RMA_GC_NUM_TESTED) ||
                (MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED >= 0 &&
                 nComplete >= MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED)) {
                /* MPIR_CVAR_CH3_RMA_GC_NUM_TESTED: Once we tested certain
                 * number of requests, we stop checking the rest of the
                 * operation list and break out the loop. */
                /* MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED: Once we found
                 * certain number of completed requests, we stop checking
                 * the rest of the operation list and break out the loop. */
                break;
            }
        }
        else {
            /* proceed to the next entry.  */
            curr_ptr = curr_ptr->next;
            nVisit++;
            if (MPIR_CVAR_CH3_RMA_GC_NUM_TESTED >= 0 && nVisit >= MPIR_CVAR_CH3_RMA_GC_NUM_TESTED) {
                /* MPIR_CVAR_CH3_RMA_GC_NUM_TESTED: Once we tested certain
                 * number of requests, we stop checking the rest of the
                 * operation list and break out the loop. */
                break;
            }
        }
    } while (curr_ptr && curr_ptr != last_elm);

    /* if (nComplete) printf("Completed %d requests\n", nComplete); */

    *nDone = nComplete;

  fn_fail:
    return mpi_errno;
}
