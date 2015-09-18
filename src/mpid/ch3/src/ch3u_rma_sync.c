/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

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

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH3_RMA_SCALABLE_FENCE_PROCESS_NUM
      category    : CH3
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
          Specify the threshold of switching the algorithm used in
          FENCE from the basic algorithm to the scalable algorithm.
          The value can be nagative, zero or positive.
          When the number of processes is larger than or equal to
          this value, FENCE will use a scalable algorithm which do
          not use O(P) data structure; when the number of processes
          is smaller than the value, FENCE will use a basic but fast
          algorithm which requires an O(P) data structure.

    - name        : MPIR_CVAR_CH3_RMA_DELAY_ISSUING_FOR_PIGGYBACKING
      category    : CH3
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Specify if delay issuing of RMA operations for piggybacking
        LOCK/UNLOCK/FLUSH is enabled. It can be either 0 or 1. When
        it is set to 1, the issuing of LOCK message is delayed until
        origin process see the first RMA operation and piggyback
        LOCK with that operation, and the origin process always keeps
        the current last operation until the ending synchronization
        call in order to piggyback UNLOCK/FLUSH with that operation.
        When it is set to 0, in WIN_LOCK/UNLOCK case, the LOCK message
        is sent out as early as possible, in WIN_LOCK_ALL/UNLOCK_ALL
        case, the origin process still tries to piggyback LOCK message
        with the first operation; for UNLOCK/FLUSH message, the origin
        process no longer keeps the current last operation but only
        piggyback UNLOCK/FLUSH if there is an operation avaliable in
        the ending synchronization call.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_lockqueue_alloc);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winlock_getlocallock);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_wincreate_allgather);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmaqueue_alloc);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmaqueue_set);

void MPIDI_CH3_RMA_Init_sync_pvars(void)
{
    /* rma_lockqueue_alloc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_lockqueue_alloc,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "Allocate Lock Queue element (in seconds)");

    /* rma_winlock_getlocallock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_winlock_getlocallock,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "WIN_LOCK:Get local lock (in seconds)");

    /* rma_wincreate_allgather */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_wincreate_allgather,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "WIN_CREATE:Allgather (in seconds)");

    /* rma_rmaqueue_alloc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmaqueue_alloc,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "Allocate RMA Queue element (in seconds)");

    /* rma_rmaqueue_set */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmaqueue_set,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "Set fields in RMA Queue element (in seconds)");
}

/* These are used to use a common routine to complete lists of RMA
   operations with a single routine, while collecting data that
   distinguishes between different synchronization modes.  This is not
   thread-safe; the best choice for thread-safety is to eliminate this
   ability to discriminate between the different types of RMA synchronization.
*/

/*
 * These routines provide a default implementation of the MPI RMA operations
 * in terms of the low-level, two-sided channel operations.
 */

#define SYNC_POST_TAG 100

#undef FUNCNAME
#define FUNCNAME flush_local_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int flush_local_all(MPID_Win * win_ptr)
{
    int i, made_progress = 0;
    int local_completed = 0;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_FLUSH_LOCAL_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_FLUSH_LOCAL_ALL);

    /* Set sync_flag in sync struct. */
    for (i = 0; i < win_ptr->num_slots; i++) {
        curr_target = win_ptr->slots[i].target_list_head;
        while (curr_target != NULL) {
            if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH_LOCAL) {
                curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH_LOCAL;
            }

            curr_target = curr_target->next;
        }
    }

    /* issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Wait for local completion */
    do {
        MPIDI_CH3I_RMA_ops_win_local_completion(win_ptr, local_completed);

        if (!local_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    } while (!local_completed);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_FLUSH_LOCAL_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME flush_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int flush_all(MPID_Win * win_ptr)
{
    int i, made_progress = 0;
    int remote_completed = 0;
    MPIDI_RMA_Target_t *curr_target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_FLUSH_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_FLUSH_ALL);

    /* Set sync_flag in sync struct. */
    for (i = 0; i < win_ptr->num_slots; i++) {
        curr_target = win_ptr->slots[i].target_list_head;
        while (curr_target != NULL) {
            if (curr_target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
                curr_target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
            }

            curr_target = curr_target->next;
        }
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        MPIDI_CH3I_RMA_ops_win_remote_completion(win_ptr, remote_completed);
        if (!remote_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    } while (!remote_completed);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_FLUSH_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME fence_barrier_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int fence_barrier_complete(MPID_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;

    MPIDI_STATE_DECL(MPID_STATE_FENCE_BARRIER_COMPLETE);
    MPIDI_FUNC_ENTER(MPID_STATE_FENCE_BARRIER_COMPLETE);

    MPID_Win_get_ptr(sreq->dev.source_win_handle, win_ptr);
    MPIU_Assert(win_ptr != NULL);

    /* decrement incomplete ibarrier request counter */
    win_ptr->sync_request_cnt--;
    MPIU_Assert(win_ptr->sync_request_cnt >= 0);

    if (win_ptr->sync_request_cnt == 0) {
        if (win_ptr->states.access_state == MPIDI_RMA_FENCE_ISSUED) {
            win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;

            if (win_ptr->num_targets_with_pending_net_ops) {
                mpi_errno = MPIDI_CH3I_Win_set_active(win_ptr);
                if (mpi_errno != MPI_SUCCESS) {
                    MPIR_ERR_POP(mpi_errno);
                }
            }
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_FENCE_BARRIER_COMPLETE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


/********************************************************************************/
/* Active Target synchronization (including WIN_FENCE, WIN_POST, WIN_START,     */
/* WIN_COMPLETE, WIN_WAIT, WIN_TEST)                                            */
/********************************************************************************/

#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_fence(int assert, MPID_Win * win_ptr)
{
    int i;
    MPIDI_RMA_Target_t *curr_target = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int comm_size = win_ptr->comm_ptr->local_size;
    int scalable_fence_enabled = 0;
    int *rma_target_marks = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    MPIR_ERR_CHKANDJUMP((win_ptr->states.access_state != MPIDI_RMA_NONE &&
                         win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                         win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED) ||
                        win_ptr->states.exposure_state != MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Judge if we should switch to scalable FENCE algorithm */
    if (comm_size >= MPIR_CVAR_CH3_RMA_SCALABLE_FENCE_PROCESS_NUM) {
        scalable_fence_enabled = 1;
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    if (assert & MPI_MODE_NOPRECEDE) {
        if (assert & MPI_MODE_NOSUCCEED) {
            goto finish_fence;
        }
        else {
            MPI_Request fence_sync_req;

            if (win_ptr->shm_allocated == TRUE) {
                MPID_Comm *node_comm_ptr = win_ptr->comm_ptr->node_comm;

                mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
            }

            mpi_errno = MPIR_Ibarrier_impl(win_ptr->comm_ptr, &fence_sync_req);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);

            if (fence_sync_req == MPI_REQUEST_NULL) {
                /* ibarrier completed immediately. */
                win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
            }
            else {
                MPID_Request *req_ptr;

                /* Set window access state properly. */
                win_ptr->states.access_state = MPIDI_RMA_FENCE_ISSUED;

                MPID_Request_get_ptr(fence_sync_req, req_ptr);
                if (!MPID_Request_is_complete(req_ptr)) {
                    req_ptr->dev.source_win_handle = win_ptr->handle;
                    req_ptr->request_completed_cb = fence_barrier_complete;
                    win_ptr->sync_request_cnt++;
                }
                else {
                    /* ibarrier completed immediately. */
                    win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
                }

                MPID_Request_release(req_ptr);
            }

            goto finish_fence;
        }
    }

    /* Perform basic algorithm by calling reduce-scatter */
    if (!scalable_fence_enabled) {
        MPIU_CHKLMEM_MALLOC(rma_target_marks, int *, comm_size * sizeof(int),
                            mpi_errno, "rma_target_marks");
        for (i = 0; i < comm_size; i++)
            rma_target_marks[i] = 0;

        for (i = 0; i < win_ptr->num_slots; i++) {
            curr_target = win_ptr->slots[i].target_list_head;
            while (curr_target != NULL) {
                rma_target_marks[curr_target->target_rank] = 1;
                curr_target = curr_target->next;
            }
        }

        win_ptr->at_completion_counter += comm_size;

        mpi_errno = MPIR_Reduce_scatter_block_impl(MPI_IN_PLACE, rma_target_marks, 1,
                                                   MPI_INT, MPI_SUM, win_ptr->comm_ptr, &errflag);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        win_ptr->at_completion_counter -= comm_size;
        win_ptr->at_completion_counter += rma_target_marks[0];
        MPIU_Assert(win_ptr->at_completion_counter >= 0);

        win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
    }

    if (!scalable_fence_enabled) {
        for (i = 0; i < win_ptr->num_slots; i++) {
            curr_target = win_ptr->slots[i].target_list_head;
            while (curr_target != NULL) {
                /* flag is set in order to decrement complete counter on target */
                curr_target->win_complete_flag = 1;

                curr_target = curr_target->next;
            }
        }

        mpi_errno = flush_local_all(win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = flush_all(win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    /* waiting for all outstanding ACKs */
    while (win_ptr->outstanding_acks > 0) {
        mpi_errno = wait_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    /* Cleanup all targets on window. */
    mpi_errno = MPIDI_CH3I_RMA_Cleanup_targets_win(win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if (scalable_fence_enabled) {
        mpi_errno = MPIR_Barrier_impl(win_ptr->comm_ptr, &errflag);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* Set window access state properly. */
        if (assert & MPI_MODE_NOSUCCEED) {
            win_ptr->states.access_state = MPIDI_RMA_NONE;
        }
        else {
            win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
        }
    }
    else {
        /* Waiting for all operations targeting at me to be finished. */
        while (win_ptr->at_completion_counter) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }

        if (assert & MPI_MODE_NOSUCCEED) {
            win_ptr->states.access_state = MPIDI_RMA_NONE;
        }
        else {
            MPI_Request fence_sync_req;

            /* Prepare for the next possible epoch */
            mpi_errno = MPIR_Ibarrier_impl(win_ptr->comm_ptr, &fence_sync_req);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);

            if (fence_sync_req == MPI_REQUEST_NULL) {
                /* ibarrier completed immediately. */
                win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
            }
            else {
                MPID_Request *req_ptr;

                win_ptr->states.access_state = MPIDI_RMA_FENCE_ISSUED;

                MPID_Request_get_ptr(fence_sync_req, req_ptr);
                if (!MPID_Request_is_complete(req_ptr)) {
                    req_ptr->dev.source_win_handle = win_ptr->handle;
                    req_ptr->request_completed_cb = fence_barrier_complete;
                    win_ptr->sync_request_cnt++;
                }
                else {
                    /* ibarrier completed immediately. */
                    win_ptr->states.access_state = MPIDI_RMA_FENCE_GRANTED;
                }

                MPID_Request_release(req_ptr);
            }

            if (win_ptr->shm_allocated == TRUE) {
                MPID_Comm *node_comm_ptr = win_ptr->comm_ptr->node_comm;
                mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");
            }
        }
    }

  finish_fence:
    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_post
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_post(MPID_Group * post_grp_ptr, int assert, MPID_Win * win_ptr)
{
    int *post_ranks_in_win_grp;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_POST);

    /* Note that here we cannot distinguish if this exposure epoch is overlapped
     * with an exposure epoch of FENCE (which is not allowed), since FENCE may be
     * ended up with not unsetting the window state. We can only detect if this
     * exposure epoch is overlapped with another exposure epoch of PSCW. */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.exposure_state != MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    /* Set window exposure state properly. */
    win_ptr->states.exposure_state = MPIDI_RMA_PSCW_EXPO;

    win_ptr->at_completion_counter += post_grp_ptr->size;

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
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

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
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
                req[i] = req_ptr->handle;
            }
            else {
                req[i] = MPI_REQUEST_NULL;
            }
        }

        mpi_errno = MPIR_Waitall_impl(post_grp_size, req, status);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
            MPIR_ERR_POP(mpi_errno);

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < post_grp_size; i++) {
                if (status[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = status[i].MPI_ERROR;
                    MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_POST);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME start_req_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int start_req_complete(MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;

    MPIDI_STATE_DECL(MPID_STATE_START_REQ_COMPLETE);
    MPIDI_FUNC_ENTER(MPID_STATE_START_REQ_COMPLETE);

    MPID_Win_get_ptr(req->dev.source_win_handle, win_ptr);
    MPIU_Assert(win_ptr != NULL);

    win_ptr->sync_request_cnt--;
    MPIU_Assert(win_ptr->sync_request_cnt >= 0);

    if (win_ptr->sync_request_cnt == 0) {
        win_ptr->states.access_state = MPIDI_RMA_PSCW_GRANTED;

        if (win_ptr->num_targets_with_pending_net_ops)
            MPIDI_CH3I_Win_set_active(win_ptr);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_START_REQ_COMPLETE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_start
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_start(MPID_Group * group_ptr, int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(2);
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_START);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_START);

    /* Note that here we cannot distinguish if this access epoch is overlapped
     * with an access epoch of FENCE (which is not allowed), since FENCE may be
     * ended up with not unsetting the window state. We can only detect if this
     * access epoch is overlapped with another access epoch of PSCW or Passive
     * Target. */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_NONE &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    win_ptr->start_grp_size = group_ptr->size;

    MPIU_CHKPMEM_MALLOC(win_ptr->start_ranks_in_win_grp, int *,
                        win_ptr->start_grp_size * sizeof(int),
                        mpi_errno, "win_ptr->start_ranks_in_win_grp");

    mpi_errno = fill_ranks_in_win_grp(win_ptr, group_ptr, win_ptr->start_ranks_in_win_grp);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if ((assert & MPI_MODE_NOCHECK) == 0) {
        int i, intra_cnt;
        MPI_Request *intra_start_req = NULL;
        MPI_Status *intra_start_status = NULL;
        MPID_Comm *comm_ptr = win_ptr->comm_ptr;
        int rank = comm_ptr->rank;

        /* wait for messages from local processes */

        /* post IRECVs */
        if (win_ptr->shm_allocated == TRUE) {
            int node_comm_size = comm_ptr->node_comm->local_size;
            MPIU_CHKLMEM_MALLOC(intra_start_req, MPI_Request *,
                                node_comm_size * sizeof(MPI_Request), mpi_errno, "intra_start_req");
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
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);

                if (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id) {
                    intra_start_req[intra_cnt++] = req_ptr->handle;
                }
                else {
                    if (!MPID_Request_is_complete(req_ptr)) {
                        req_ptr->dev.source_win_handle = win_ptr->handle;
                        req_ptr->request_completed_cb = start_req_complete;
                        win_ptr->sync_request_cnt++;
                    }

                    MPID_Request_release(req_ptr);
                }
            }
        }

        /* for targets on SHM, waiting until their IRECVs to be finished */
        if (intra_cnt) {
            mpi_errno = MPIR_Waitall_impl(intra_cnt, intra_start_req, intra_start_status);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                MPIR_ERR_POP(mpi_errno);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno == MPI_ERR_IN_STATUS) {
                for (i = 0; i < intra_cnt; i++) {
                    if (intra_start_status[i].MPI_ERROR != MPI_SUCCESS) {
                        mpi_errno = intra_start_status[i].MPI_ERROR;
                        MPIR_ERR_POP(mpi_errno);
                    }
                }
            }
            /* --END ERROR HANDLING-- */
        }
    }

    /* Set window access state properly. */
    win_ptr->states.access_state = MPIDI_RMA_PSCW_ISSUED;

    if (win_ptr->sync_request_cnt == 0) {
        win_ptr->states.access_state = MPIDI_RMA_PSCW_GRANTED;
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_START);
    return mpi_errno;
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_complete(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, dst, rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *curr_target;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

    /* Access epochs on the same window must be disjoint. */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PSCW_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_PSCW_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    if (win_ptr->states.access_state == MPIDI_RMA_PSCW_ISSUED) {
        while (win_ptr->states.access_state != MPIDI_RMA_PSCW_GRANTED) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    for (i = 0; i < win_ptr->start_grp_size; i++) {
        dst = win_ptr->start_ranks_in_win_grp[i];
        if (dst == rank) {
            win_ptr->at_completion_counter--;
            MPIU_Assert(win_ptr->at_completion_counter >= 0);
            continue;
        }

        curr_target = MPIDI_CH3I_RMA_RANK_TO_SLOT(win_ptr, dst)->target_list_head;
        while (curr_target != NULL && curr_target->target_rank != dst)
            curr_target = curr_target->next;

        if (curr_target != NULL) {
            curr_target->win_complete_flag = 1;
        }
        else {
            /* FIXME: do we need to wait for remote completion? */
            mpi_errno = send_decr_at_cnt_msg(dst, win_ptr, MPIDI_CH3_PKT_FLAG_NONE);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    mpi_errno = flush_local_all(win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* waiting for all outstanding ACKs */
    while (win_ptr->outstanding_acks > 0) {
        mpi_errno = wait_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    /* Cleanup all targets on this window. */
    mpi_errno = MPIDI_CH3I_RMA_Cleanup_targets_win(win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Set window access state properly. */
    win_ptr->states.access_state = MPIDI_RMA_NONE;

    /* free start group stored in window */
    MPIU_Free(win_ptr->start_ranks_in_win_grp);
    win_ptr->start_ranks_in_win_grp = NULL;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_wait(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.exposure_state != MPIDI_RMA_PSCW_EXPO,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* wait for all operations from other processes to finish */
    while (win_ptr->at_completion_counter) {
        mpi_errno = wait_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    /* Set window exposure state properly. */
    win_ptr->states.exposure_state = MPIDI_RMA_NONE;

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_test
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_test(MPID_Win * win_ptr, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_TEST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_TEST);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.exposure_state != MPIDI_RMA_PSCW_EXPO,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    *flag = (win_ptr->at_completion_counter) ? 0 : 1;
    if (*flag) {
        /* Set window exposure state properly. */
        win_ptr->states.exposure_state = MPIDI_RMA_NONE;

        /* Ensure ordering of load/store operations. */
        if (win_ptr->shm_allocated == TRUE) {
            OPA_read_write_barrier();
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_TEST);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/********************************************************************************/
/* Passive Target synchronization (including WIN_LOCK, WIN_UNLOCK, WIN_FLUSH,   */
/* WIN_FLUSH_LOCAL, WIN_LOCK_ALL, WIN_UNLOCK_ALL, WIN_FLUSH_ALL,                */
/* WIN_FLUSH_LOCAL_ALL, WIN_SYNC)                                               */
/********************************************************************************/

#undef FUNCNAME
#define FUNCNAME MPID_Win_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_lock(int lock_type, int dest, int assert, MPID_Win * win_ptr)
{
    int made_progress = 0;
    int shm_target = FALSE;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *target = NULL;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_LOCK);

    /* Note that here we cannot distinguish if this access epoch is overlapped
     * with an access epoch of FENCE (which is not allowed), since FENCE may be
     * ended up with not unsetting the window state. We can only detect if this
     * access epoch is overlapped with another access epoch of PSCW or Passive
     * Target. */
    if (win_ptr->lock_epoch_count == 0) {
        MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_NONE &&
                            win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                            win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED,
                            mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");
    }
    else {
        MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET,
                            mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");
    }

    if (dest != MPI_PROC_NULL) {
        /* check if we lock the same target window more than once. */
        mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
        MPIR_ERR_CHKANDJUMP(target != NULL, mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");
    }

    /* Error handling is finished. */

    if (win_ptr->lock_epoch_count == 0) {
        /* Set window access state properly. */
        win_ptr->states.access_state = MPIDI_RMA_PER_TARGET;
    }
    win_ptr->lock_epoch_count++;

    if (dest == MPI_PROC_NULL)
        goto finish_lock;

    if (win_ptr->shm_allocated == TRUE) {
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        if (orig_vc->node_id == target_vc->node_id)
            shm_target = TRUE;
    }

    /* Create a new target. */
    mpi_errno = MPIDI_CH3I_Win_create_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Store lock_state (CALLED/ISSUED/GRANTED), lock_type (SHARED/EXCLUSIVE),
     * lock_mode (MODE_NOCHECK). */
    if (assert & MPI_MODE_NOCHECK)
        target->access_state = MPIDI_RMA_LOCK_GRANTED;
    else
        target->access_state = MPIDI_RMA_LOCK_CALLED;
    target->lock_type = lock_type;
    target->lock_mode = assert;

    /* If Destination is myself or process on SHM, acquire the lock,
     * wait until lock is granted. */
    if (!(assert & MPI_MODE_NOCHECK)) {
        if (dest == rank || shm_target) {
            mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest, &made_progress);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);

            while (target->access_state != MPIDI_RMA_LOCK_GRANTED) {
                mpi_errno = wait_progress_engine();
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
            }
        }
        else if (!MPIR_CVAR_CH3_RMA_DELAY_ISSUING_FOR_PIGGYBACKING) {
            /* if DELAY_ISSUING_FOR_PIGGYBACKING is turned off, send lock request now
             * since we do not want to piggyback LOCK with future OP */
            mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest, &made_progress);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    }

  finish_lock:
    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_unlock(int dest, MPID_Win * win_ptr)
{
    int made_progress = 0;
    int local_completed ATTRIBUTE((unused)) = 0, remote_completed = 0;
    MPIDI_RMA_Target_t *target = NULL;
    enum MPIDI_RMA_sync_types sync_flag;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated) {
        OPA_read_write_barrier();
    }

    if (dest == MPI_PROC_NULL)
        goto finish_unlock;

    /* Find or recreate target. */
    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    if (target == NULL) {
        mpi_errno = MPIDI_CH3I_Win_create_target(win_ptr, dest, &target);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
        target->access_state = MPIDI_RMA_LOCK_GRANTED;
    }

    /* Set sync_flag in sync struct. */
    if (target->lock_mode & MPI_MODE_NOCHECK)
        sync_flag = MPIDI_RMA_SYNC_FLUSH;
    else
        sync_flag = MPIDI_RMA_SYNC_UNLOCK;
    if (target->sync.sync_flag < sync_flag) {
        target->sync.sync_flag = sync_flag;
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        MPIDI_CH3I_RMA_ops_completion(win_ptr, target, local_completed, remote_completed);

        if (!remote_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    } while (!remote_completed);

  finish_unlock:
    if (win_ptr->comm_ptr->rank == dest) {
        /* In some cases (e.g. target is myself),
         * this function call does not go through the progress engine.
         * Therefore, it is possible that this process never process
         * events coming from other processes. This may cause deadlock in
         * applications where the program execution on this process depends
         * on the happening of events from other processes. Here we poke
         * the progress engine once to avoid such issue.  */
        mpi_errno = poke_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    win_ptr->lock_epoch_count--;
    if (win_ptr->lock_epoch_count == 0) {
        /* Set window access state properly. */
        win_ptr->states.access_state = MPIDI_RMA_NONE;
    }

    if (target != NULL) {
        /* Cleanup the target. */
        mpi_errno = MPIDI_CH3I_Win_target_dequeue_and_free(win_ptr, target);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_flush
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_flush(int dest, MPID_Win * win_ptr)
{
    int made_progress = 0;
    int local_completed ATTRIBUTE((unused)) = 0, remote_completed = 0;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FLUSH);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated) {
        OPA_read_write_barrier();
    }

    if (dest == MPI_PROC_NULL)
        goto finish_flush;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    if (target == NULL)
        goto finish_flush;

    if (rank == dest)
        goto finish_flush;

    if (win_ptr->shm_allocated) {
        MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        if (orig_vc->node_id == target_vc->node_id)
            goto finish_flush;
    }

    /* Set sync_flag in sync struct. */
    if (target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH) {
        target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH;
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        MPIDI_CH3I_RMA_ops_completion(win_ptr, target, local_completed, remote_completed);

        if (!remote_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    } while (!remote_completed);

  finish_flush:
    if (win_ptr->comm_ptr->rank == dest) {
        /* In some cases (e.g. target is myself),
         * this function call does not go through the progress engine.
         * Therefore, it is possible that this process never process
         * events coming from other processes. This may cause deadlock in
         * applications where the program execution on this process depends
         * on the happening of events from other processes. Here we poke
         * the progress engine once to avoid such issue.  */
        mpi_errno = poke_progress_engine();
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FLUSH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_flush_local
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_flush_local(int dest, MPID_Win * win_ptr)
{
    int made_progress = 0;
    int local_completed = 0, remote_completed ATTRIBUTE((unused)) = 0;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *target = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_LOCAL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FLUSH_LOCAL);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated) {
        OPA_read_write_barrier();
    }

    if (dest == MPI_PROC_NULL)
        goto fn_exit;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, dest, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    if (target == NULL)
        goto fn_exit;

    if (rank == dest)
        goto fn_exit;

    if (win_ptr->shm_allocated) {
        MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        if (orig_vc->node_id == target_vc->node_id)
            goto fn_exit;
    }

    /* Set sync_flag in sync struct. */
    if (target->sync.sync_flag < MPIDI_RMA_SYNC_FLUSH_LOCAL)
        target->sync.sync_flag = MPIDI_RMA_SYNC_FLUSH_LOCAL;

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_target(win_ptr, dest, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Wait for local completion. */
    do {
        MPIDI_CH3I_RMA_ops_completion(win_ptr, target, local_completed, remote_completed);

        if (!local_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    } while (!local_completed);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FLUSH_LOCAL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_lock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_lock_all(int assert, MPID_Win * win_ptr)
{
    int i, rank = win_ptr->comm_ptr->rank;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_LOCK_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_LOCK_ALL);

    /* Note that here we cannot distinguish if this access epoch is overlapped
     * with an access epoch of FENCE (which is not allowed), since FENCE may be
     * ended up with not unsetting the window state. We can only detect if this
     * access epoch is overlapped with another access epoch of PSCW or Passive
     * Target. */
    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_NONE &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_FENCE_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Set window access state properly. */
    if (assert & MPI_MODE_NOCHECK)
        win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_GRANTED;
    else
        win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_CALLED;

    win_ptr->lock_all_assert = assert;

    MPIU_Assert(win_ptr->outstanding_locks == 0);

    /* Acquire the lock on myself and the lock on processes on SHM.
     * No need to create a target for them. */
    if (!(win_ptr->lock_all_assert & MPI_MODE_NOCHECK)) {
        win_ptr->outstanding_locks++;
        mpi_errno = acquire_local_lock(win_ptr, MPI_LOCK_SHARED);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

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
                        MPIR_ERR_POP(mpi_errno);
                }
            }
        }

        /* wait for lock to be granted */
        while (win_ptr->outstanding_locks > 0) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_unlock_all(MPID_Win * win_ptr)
{
    int i, made_progress = 0;
    int remote_completed = 0;
    int rank = win_ptr->comm_ptr->rank;
    MPIDI_RMA_Target_t *curr_target = NULL;
    enum MPIDI_RMA_sync_types sync_flag;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK_ALL);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated) {
        OPA_read_write_barrier();
    }

    /* Unlock MYSELF and processes on SHM. */
    if (!(win_ptr->lock_all_assert & MPI_MODE_NOCHECK)) {
        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (win_ptr->shm_allocated == TRUE) {
            MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
            for (i = 0; i < win_ptr->comm_ptr->local_size; i++) {
                if (i == rank)
                    continue;
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
                if (orig_vc->node_id == target_vc->node_id) {
                    mpi_errno = send_unlock_msg(i, win_ptr, MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIR_ERR_POP(mpi_errno);
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
            curr_target = win_ptr->slots[i].target_list_head;
            while (curr_target != NULL) {
                if (curr_target->sync.sync_flag < sync_flag) {
                    curr_target->sync.sync_flag = sync_flag;
                }
                curr_target = curr_target->next;
            }
        }
    }
    else {
        for (i = 0; i < win_ptr->comm_ptr->local_size; i++) {
            curr_target = MPIDI_CH3I_RMA_RANK_TO_SLOT(win_ptr, i)->target_list_head;
            while (curr_target != NULL && curr_target->target_rank != i)
                curr_target = curr_target->next;

            if (curr_target != NULL) {
                if (curr_target->sync.sync_flag < sync_flag) {
                    curr_target->sync.sync_flag = sync_flag;
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

                mpi_errno = send_unlock_msg(i, win_ptr, MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK);
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
            }
        }
    }

    /* Issue out all operations. */
    mpi_errno = MPIDI_CH3I_RMA_Make_progress_win(win_ptr, &made_progress);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Wait for remote completion. */
    do {
        MPIDI_CH3I_RMA_ops_win_remote_completion(win_ptr, remote_completed);
        if (!remote_completed) {
            mpi_errno = wait_progress_engine();
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    } while (!remote_completed);

    /* Cleanup all targets on this window. */
    mpi_errno = MPIDI_CH3I_RMA_Cleanup_targets_win(win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* Set window access state properly. */
    win_ptr->states.access_state = MPIDI_RMA_NONE;

    /* reset lock_all assert on window. */
    win_ptr->lock_all_assert = 0;

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_flush_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_flush_all(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPIDI_STATE_MPID_WIN_FLUSH_ALL);

    MPIDI_RMA_FUNC_ENTER(MPIDI_STATE_MPID_WIN_FLUSH_ALL);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    mpi_errno = flush_all(win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPIDI_STATE_MPID_WIN_FLUSH_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_flush_local_all
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_flush_local_all(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    mpi_errno = flush_local_all(win_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FLUSH_LOCAL_ALL);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPID_Win_sync
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Win_sync(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_SYNC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_SYNC);

    MPIR_ERR_CHKANDJUMP(win_ptr->states.access_state != MPIDI_RMA_PER_TARGET &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_CALLED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_ISSUED &&
                        win_ptr->states.access_state != MPIDI_RMA_LOCK_ALL_GRANTED,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    OPA_read_write_barrier();

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_SYNC);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
