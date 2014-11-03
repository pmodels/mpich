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
static inline int rma_list_complete(MPID_Win * win_ptr, MPIDI_RMA_Ops_list_t * ops_list);
static inline int rma_list_gc(MPID_Win * win_ptr,
                              MPIDI_RMA_Ops_list_t * ops_list,
                              MPIDI_RMA_Op_t * last_elm, int *nDone);


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_fence(int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    int *rma_target_proc, *nops_to_proc, i, total_op_count, *curr_ops_cnt;
    MPIDI_RMA_Op_t *curr_ptr;
    MPIDI_RMA_Ops_list_t *ops_list;
    MPID_Comm *comm_ptr;
    MPI_Win source_win_handle, target_win_handle;
    MPID_Progress_state progress_state;
    int errflag = FALSE;
    MPIU_CHKLMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FENCE);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_NONE &&
                        win_ptr->epoch_state != MPIDI_EPOCH_FENCE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Note that the NOPRECEDE and NOSUCCEED must be specified by all processes
     * in the window's group if any specify it */
    if (assert & MPI_MODE_NOPRECEDE) {
        /* Error: Operations were issued and the user claimed NOPRECEDE */
        MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_FENCE,
                            mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

        win_ptr->fence_issued = (assert & MPI_MODE_NOSUCCEED) ? 0 : 1;
        goto shm_barrier;
    }

    if (win_ptr->fence_issued == 0) {
        /* win_ptr->fence_issued == 0 means either this is the very first
         * call to fence or the preceding fence had the
         * MPI_MODE_NOSUCCEED assert.
         *
         * If this fence has MPI_MODE_NOSUCCEED, do nothing and return.
         * Otherwise just increment the fence count and return. */

        if (!(assert & MPI_MODE_NOSUCCEED))
            win_ptr->fence_issued = 1;
    }
    else {
        int nRequest = 0;
        int nRequestNew = 0;

        /* Ensure ordering of load/store operations. */
        if (win_ptr->shm_allocated == TRUE) {
            OPA_read_write_barrier();
        }

        /* This is the second or later fence. Do all the preceding RMA ops. */
        comm_ptr = win_ptr->comm_ptr;
        /* First inform every process whether it is a target of RMA
         * ops from this process */
        comm_size = comm_ptr->local_size;

        MPIU_CHKLMEM_MALLOC(rma_target_proc, int *, comm_size * sizeof(int),
                            mpi_errno, "rma_target_proc");
        for (i = 0; i < comm_size; i++)
            rma_target_proc[i] = 0;

        /* keep track of no. of ops to each proc. Needed for knowing
         * whether or not to decrement the completion counter. The
         * completion counter is decremented only on the last
         * operation. */
        MPIU_CHKLMEM_MALLOC(nops_to_proc, int *, comm_size * sizeof(int),
                            mpi_errno, "nops_to_proc");
        for (i = 0; i < comm_size; i++)
            nops_to_proc[i] = 0;

        /* Note, active target uses the following ops list, and passive
         * target uses win_ptr->targets[..] */
        ops_list = &win_ptr->at_rma_ops_list;

        /* set rma_target_proc[i] to 1 if rank i is a target of RMA
         * ops from this process */
        total_op_count = 0;
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
        while (curr_ptr != NULL) {
            total_op_count++;
            rma_target_proc[curr_ptr->target_rank] = 1;
            nops_to_proc[curr_ptr->target_rank]++;
            curr_ptr = curr_ptr->next;
        }

        MPIU_CHKLMEM_MALLOC(curr_ops_cnt, int *, comm_size * sizeof(int),
                            mpi_errno, "curr_ops_cnt");
        for (i = 0; i < comm_size; i++)
            curr_ops_cnt[i] = 0;
        /* do a reduce_scatter_block (with MPI_SUM) on rma_target_proc.
         * As a result,
         * each process knows how many other processes will be doing
         * RMA ops on its window */

        /* first initialize the completion counter. */
        win_ptr->at_completion_counter += comm_size;

        mpi_errno = MPIR_Reduce_scatter_block_impl(MPI_IN_PLACE, rma_target_proc, 1,
                                                   MPI_INT, MPI_SUM, comm_ptr, &errflag);
        /* result is stored in rma_target_proc[0] */
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* Ensure ordering of load/store operations. */
        if (win_ptr->shm_allocated == TRUE) {
            OPA_read_write_barrier();
        }

        /* Set the completion counter */
        /* FIXME: MT: this needs to be done atomically because other
         * procs have the address and could decrement it. */
        win_ptr->at_completion_counter -= comm_size;
        win_ptr->at_completion_counter += rma_target_proc[0];

        i = 0;
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
        while (curr_ptr != NULL) {
            MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;

            /* The completion counter at the target is decremented only on
             * the last RMA operation. */
            if (curr_ops_cnt[curr_ptr->target_rank] == nops_to_proc[curr_ptr->target_rank] - 1) {
                flags = MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE;
            }

            source_win_handle = win_ptr->handle;
            target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];

            mpi_errno = MPIDI_CH3I_Issue_rma_op(curr_ptr, win_ptr, flags,
                                                source_win_handle, target_win_handle);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);

            i++;
            curr_ops_cnt[curr_ptr->target_rank]++;
            /* If the request is null, we can remove it immediately */
            if (!curr_ptr->request) {
                MPIDI_CH3I_RMA_Ops_free_and_next(ops_list, &curr_ptr);
            }
            else {
                nRequest++;
                curr_ptr = curr_ptr->next;
                /* The test on the difference is to reduce the number
                 * of times the partial complete routine is called. Without
                 * this, significant overhead is added once the
                 * number of requests exceeds the threshold, since the
                 * number that are completed in a call may be small. */
                if (nRequest > MPIR_CVAR_CH3_RMA_NREQUEST_THRESHOLD &&
                    nRequest - nRequestNew > MPIR_CVAR_CH3_RMA_NREQUEST_NEW_THRESHOLD) {
                    int nDone = 0;
                    mpi_errno = poke_progress_engine();
                    if (mpi_errno != MPI_SUCCESS)
                        MPIU_ERR_POP(mpi_errno);

                    mpi_errno = rma_list_gc(win_ptr, ops_list, curr_ptr, &nDone);
                    if (mpi_errno != MPI_SUCCESS)
                        MPIU_ERR_POP(mpi_errno);
                    /* if (nDone > 0) printf("nDone = %d\n", nDone); */
                    nRequest -= nDone;
                    nRequestNew = nRequest;
                }
            }
        }

        /* We replaced a loop over an array of requests with a list of the
         * incomplete requests.  The reason to do
         * that is for long lists - processing the entire list until
         * all are done introduces a potentially n^2 time.  In
         * testing with test/mpi/perf/manyrma.c , the number of iterations
         * within the "while (total_op_count) was O(total_op_count).
         *
         * Another alternative is to create a more compressed list (storing
         * only the necessary information, reducing the number of cache lines
         * needed while looping through the requests.
         */
        if (total_op_count) {
            mpi_errno = rma_list_complete(win_ptr, ops_list);
            if (mpi_errno != MPI_SUCCESS)
                MPIU_ERR_POP(mpi_errno);
        }

        /* MT: avoid processing unissued operations enqueued by other threads
           in rma_list_complete() */
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
        if (curr_ptr && !curr_ptr->request)
            goto finish_up;
        MPIU_Assert(MPIDI_CH3I_RMA_Ops_isempty(ops_list));

 finish_up:
	/* wait for all operations from other processes to finish */
        if (win_ptr->at_completion_counter) {
            MPID_Progress_start(&progress_state);
            while (win_ptr->at_completion_counter) {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_Progress_end(&progress_state);
                    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
                }
                /* --END ERROR HANDLING-- */
            }
            MPID_Progress_end(&progress_state);
        }

        if (assert & MPI_MODE_NOSUCCEED) {
            win_ptr->fence_issued = 0;
        }

        win_ptr->epoch_state = MPIDI_EPOCH_NONE;
    }

 shm_barrier:
    if (!(assert & MPI_MODE_NOSUCCEED)) {
        /* In a FENCE without MPI_MODE_NOSUCCEED (which means this FENCE
           might start a new Active epoch), if SHM is allocated, perform
           a barrier among processes on the same node, to prevent one
           process modifying another process's memory before that process
           starts an epoch. */

        if (win_ptr->shm_allocated == TRUE) {
            MPID_Comm *node_comm_ptr = win_ptr->comm_ptr->node_comm;

            /* Ensure ordering of load/store operations. */
            OPA_read_write_barrier();

            mpi_errno = MPIR_Barrier_impl(node_comm_ptr, &errflag);
            if (mpi_errno) {goto fn_fail;}

            /* Ensure ordering of load/store operations. */
            OPA_read_write_barrier();
        }
    }

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_FENCE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_post(MPID_Group * post_grp_ptr, int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *win_grp_ptr;
    int i, post_grp_size, *ranks_in_post_grp, *ranks_in_win_grp, dst, rank;
    MPID_Comm *win_comm_ptr;
    MPIU_CHKLMEM_DECL(4);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_POST);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_NONE &&
                        win_ptr->epoch_state != MPIDI_EPOCH_START,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Track access epoch state */
    if (win_ptr->epoch_state == MPIDI_EPOCH_START)
        win_ptr->epoch_state = MPIDI_EPOCH_PSCW;
    else
        win_ptr->epoch_state = MPIDI_EPOCH_POST;

    /* Even though we would want to reset the fence counter to keep
     * the user from using the previous fence to mark the beginning of
     * a fence epoch if he switched from fence to lock-unlock
     * synchronization, we cannot do this because fence_issued must be
     * updated collectively */

    post_grp_size = post_grp_ptr->size;

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    /* initialize the completion counter */
    win_ptr->at_completion_counter += post_grp_size;

    if ((assert & MPI_MODE_NOCHECK) == 0) {
        MPI_Request *req;
        MPI_Status *status;

        /* NOCHECK not specified. We need to notify the source
         * processes that Post has been called. */

        /* We need to translate the ranks of the processes in
         * post_group to ranks in win_ptr->comm_ptr, so that we
         * can do communication */

        MPIU_CHKLMEM_MALLOC(ranks_in_post_grp, int *,
                            post_grp_size * sizeof(int), mpi_errno, "ranks_in_post_grp");
        MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *,
                            post_grp_size * sizeof(int), mpi_errno, "ranks_in_win_grp");

        for (i = 0; i < post_grp_size; i++) {
            ranks_in_post_grp[i] = i;
        }

        win_comm_ptr = win_ptr->comm_ptr;

        mpi_errno = MPIR_Comm_group_impl(win_comm_ptr, &win_grp_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);


        mpi_errno = MPIR_Group_translate_ranks_impl(post_grp_ptr, post_grp_size, ranks_in_post_grp,
                                                    win_grp_ptr, ranks_in_win_grp);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        rank = win_ptr->comm_ptr->rank;

        MPIU_CHKLMEM_MALLOC(req, MPI_Request *, post_grp_size * sizeof(MPI_Request), mpi_errno,
                            "req");
        MPIU_CHKLMEM_MALLOC(status, MPI_Status *, post_grp_size * sizeof(MPI_Status), mpi_errno,
                            "status");

        /* Send a 0-byte message to the source processes */
        for (i = 0; i < post_grp_size; i++) {
            dst = ranks_in_win_grp[i];

            /* FIXME: Short messages like this shouldn't normally need a
             * request - this should consider using the ch3 call to send
             * a short message and return a request only if the message is
             * not delivered. */
            if (dst != rank) {
                MPID_Request *req_ptr;
                mpi_errno = MPID_Isend(&i, 0, MPI_INT, dst, SYNC_POST_TAG, win_comm_ptr,
                                       MPID_CONTEXT_INTRA_PT2PT, &req_ptr);
                if (mpi_errno)
                    MPIU_ERR_POP(mpi_errno);
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

        mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
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


static int recv_post_msgs(MPID_Win * win_ptr, int *ranks_in_win_grp, int local)
{
    int mpi_errno = MPI_SUCCESS;
    int start_grp_size, src, rank, i, j;
    MPI_Request *req;
    MPI_Status *status;
    MPID_Comm *comm_ptr = win_ptr->comm_ptr;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_RECV_POST_MSGS);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_RECV_POST_MSGS);

    /* Wait for 0-byte messages from processes either on the same node
     * or not (depending on the "local" parameter), so we know they
     * have entered post. */

    start_grp_size = win_ptr->start_group_ptr->size;

    rank = win_ptr->comm_ptr->rank;
    MPIU_CHKLMEM_MALLOC(req, MPI_Request *, start_grp_size * sizeof(MPI_Request), mpi_errno, "req");
    MPIU_CHKLMEM_MALLOC(status, MPI_Status *, start_grp_size * sizeof(MPI_Status), mpi_errno,
                        "status");

    j = 0;
    for (i = 0; i < start_grp_size; i++) {
        src = ranks_in_win_grp[i];

        if (src == rank)
            continue;

        if (local && win_ptr->shm_allocated == TRUE) {
            MPID_Request *req_ptr;

            MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, src, &target_vc);

            if (orig_vc->node_id == target_vc->node_id) {
                mpi_errno = MPID_Irecv(NULL, 0, MPI_INT, src, SYNC_POST_TAG,
                                       comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &req_ptr);
                if (mpi_errno)
                    MPIU_ERR_POP(mpi_errno);
                req[j++] = req_ptr->handle;
            }
        }
        else if (!local) {
            MPID_Request *req_ptr;

            MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, src, &target_vc);

            if (win_ptr->shm_allocated != TRUE ||
                orig_vc->node_id != target_vc->node_id) {
                mpi_errno = MPID_Irecv(NULL, 0, MPI_INT, src, SYNC_POST_TAG,
                                       comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &req_ptr);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                req[j++] = req_ptr->handle;
            }
        }
    }

    if (j) {
        mpi_errno = MPIR_Waitall_impl(j, req, status);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
            MPIU_ERR_POP(mpi_errno);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < j; i++) {
                if (status[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = status[i].MPI_ERROR;
                    MPIU_ERR_POP(mpi_errno);
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

  fn_fail:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_RECV_POST_MSGS);
    return mpi_errno;
}

static int fill_ranks_in_win_grp(MPID_Win * win_ptr, int *ranks_in_win_grp)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ranks_in_start_grp;
    MPID_Group *win_grp_ptr;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIU_CHKLMEM_MALLOC(ranks_in_start_grp, int *, win_ptr->start_group_ptr->size * sizeof(int),
                        mpi_errno, "ranks_in_start_grp");

    for (i = 0; i < win_ptr->start_group_ptr->size; i++)
        ranks_in_start_grp[i] = i;

    mpi_errno = MPIR_Comm_group_impl(win_ptr->comm_ptr, &win_grp_ptr);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno =
        MPIR_Group_translate_ranks_impl(win_ptr->start_group_ptr, win_ptr->start_group_ptr->size,
                                        ranks_in_start_grp, win_grp_ptr, ranks_in_win_grp);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_fail:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_FILL_RANKS_IN_WIN_GRP);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_start(MPID_Group * group_ptr, int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int *ranks_in_win_grp;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_START);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_START);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_NONE &&
                        win_ptr->epoch_state != MPIDI_EPOCH_POST,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Track access epoch state */
    if (win_ptr->epoch_state == MPIDI_EPOCH_POST)
        win_ptr->epoch_state = MPIDI_EPOCH_PSCW;
    else
        win_ptr->epoch_state = MPIDI_EPOCH_START;

    /* Even though we would want to reset the fence counter to keep
     * the user from using the previous fence to mark the beginning of
     * a fence epoch if he switched from fence to lock-unlock
     * synchronization, we cannot do this because fence_issued must be
     * updated collectively */

    win_ptr->start_group_ptr = group_ptr;
    MPIR_Group_add_ref(group_ptr);
    win_ptr->start_assert = assert;

    /* wait for messages from local processes */
    MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *, win_ptr->start_group_ptr->size * sizeof(int),
                        mpi_errno, "ranks_in_win_grp");

    mpi_errno = fill_ranks_in_win_grp(win_ptr, ranks_in_win_grp);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    /* If MPI_MODE_NOCHECK was not specified, we need to check if
       Win_post was called on the target processes on SHM window.
       Wait for a 0-byte sync message from each target process. */
    if ((win_ptr->start_assert & MPI_MODE_NOCHECK) == 0)
    {
        mpi_errno = recv_post_msgs(win_ptr, ranks_in_win_grp, 1);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* Ensure ordering of load/store operations */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

  fn_fail:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_START);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_complete(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, *nops_to_proc, new_total_op_count;
    int i, j, dst, total_op_count, *curr_ops_cnt;
    MPIDI_RMA_Op_t *curr_ptr;
    MPIDI_RMA_Ops_list_t *ops_list;
    MPID_Comm *comm_ptr;
    MPI_Win source_win_handle, target_win_handle;
    int start_grp_size, *ranks_in_win_grp, rank;
    int nRequest = 0;
    int nRequestNew = 0;
    MPIU_CHKLMEM_DECL(6);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_COMPLETE);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_PSCW &&
                        win_ptr->epoch_state != MPIDI_EPOCH_START,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Track access epoch state */
    if (win_ptr->epoch_state == MPIDI_EPOCH_PSCW)
        win_ptr->epoch_state = MPIDI_EPOCH_POST;
    else
        win_ptr->epoch_state = MPIDI_EPOCH_NONE;

    comm_ptr = win_ptr->comm_ptr;
    comm_size = comm_ptr->local_size;

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    /* Translate the ranks of the processes in
     * start_group to ranks in win_ptr->comm_ptr */

    start_grp_size = win_ptr->start_group_ptr->size;

    MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *, start_grp_size * sizeof(int),
                        mpi_errno, "ranks_in_win_grp");

    mpi_errno = fill_ranks_in_win_grp(win_ptr, ranks_in_win_grp);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    rank = win_ptr->comm_ptr->rank;

    /* If MPI_MODE_NOCHECK was not specified, we need to check if
     * Win_post was called on the target processes. Wait for a 0-byte sync
     * message from each target process */
    if ((win_ptr->start_assert & MPI_MODE_NOCHECK) == 0) {
        /* wait for messages from non-local processes */
        mpi_errno = recv_post_msgs(win_ptr, ranks_in_win_grp, 0);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }

    /* keep track of no. of ops to each proc. Needed for knowing
     * whether or not to decrement the completion counter. The
     * completion counter is decremented only on the last
     * operation. */

    /* Note, active target uses the following ops list, and passive
     * target uses win_ptr->targets[..] */
    ops_list = &win_ptr->at_rma_ops_list;

    MPIU_CHKLMEM_MALLOC(nops_to_proc, int *, comm_size * sizeof(int), mpi_errno, "nops_to_proc");
    for (i = 0; i < comm_size; i++)
        nops_to_proc[i] = 0;

    total_op_count = 0;
    curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
    while (curr_ptr != NULL) {
        nops_to_proc[curr_ptr->target_rank]++;
        total_op_count++;
        curr_ptr = curr_ptr->next;
    }

    /* We allocate a few extra requests because if there are no RMA
     * ops to a target process, we need to send a 0-byte message just
     * to decrement the completion counter. */

    MPIU_CHKLMEM_MALLOC(curr_ops_cnt, int *, comm_size * sizeof(int), mpi_errno, "curr_ops_cnt");
    for (i = 0; i < comm_size; i++)
        curr_ops_cnt[i] = 0;

    i = 0;
    curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
    while (curr_ptr != NULL) {
        MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;

        /* The completion counter at the target is decremented only on
         * the last RMA operation. */
        if (curr_ops_cnt[curr_ptr->target_rank] == nops_to_proc[curr_ptr->target_rank] - 1) {
            flags = MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE;
        }

        source_win_handle = win_ptr->handle;
        target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];

        mpi_errno = MPIDI_CH3I_Issue_rma_op(curr_ptr, win_ptr, flags,
                                            source_win_handle, target_win_handle);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        i++;
        curr_ops_cnt[curr_ptr->target_rank]++;
        /* If the request is null, we can remove it immediately */
        if (!curr_ptr->request) {
            MPIDI_CH3I_RMA_Ops_free_and_next(ops_list, &curr_ptr);
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
                mpi_errno = rma_list_gc(win_ptr, ops_list, curr_ptr, &nDone);
                if (mpi_errno != MPI_SUCCESS)
                    MPIU_ERR_POP(mpi_errno);
                nRequest -= nDone;
                nRequestNew = nRequest;
            }
        }
    }

    /* If the start_group included some processes that did not end up
     * becoming targets of  RMA operations from this process, we need
     * to send a dummy message to those processes just to decrement
     * the completion counter */

    j = i;
    new_total_op_count = total_op_count;
    for (i = 0; i < start_grp_size; i++) {
        dst = ranks_in_win_grp[i];
        if (dst == rank) {
            /* FIXME: MT: this has to be done atomically */
            win_ptr->at_completion_counter -= 1;
        }
        else if (nops_to_proc[dst] == 0) {
            MPIDI_CH3_Pkt_t upkt;
            MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
            MPIDI_VC_t *vc;
            MPID_Request *request;

            MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
            put_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE;
            put_pkt->addr = NULL;
            put_pkt->count = 0;
            put_pkt->datatype = MPI_INT;
            put_pkt->target_win_handle = win_ptr->all_win_handles[dst];
            put_pkt->source_win_handle = win_ptr->handle;

            MPIDI_Comm_get_vc_set_active(comm_ptr, dst, &vc);

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = MPIDI_CH3_iStartMsg(vc, put_pkt, sizeof(*put_pkt), &request);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            if (mpi_errno != MPI_SUCCESS) {
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
            }
            /* In the unlikely event that a request is returned (the message
             * is not sent yet), add it to the list of pending operations */
            if (request) {
                MPIDI_RMA_Op_t *new_ptr = NULL;

                mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
                if (mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }

                new_ptr->request = request;
            }
            j++;
            new_total_op_count++;
        }
    }

    if (new_total_op_count) {
        mpi_errno = rma_list_complete(win_ptr, ops_list);
        if (mpi_errno != MPI_SUCCESS)
            MPIU_ERR_POP(mpi_errno);
    }

    MPIU_Assert(MPIDI_CH3I_RMA_Ops_isempty(ops_list));

    /* free the group stored in window */
    MPIR_Group_release(win_ptr->start_group_ptr);
    win_ptr->start_group_ptr = NULL;

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
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

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_PSCW &&
                        win_ptr->epoch_state != MPIDI_EPOCH_POST,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Track access epoch state */
    if (win_ptr->epoch_state == MPIDI_EPOCH_PSCW)
        win_ptr->epoch_state = MPIDI_EPOCH_START;
    else
        win_ptr->epoch_state = MPIDI_EPOCH_NONE;

    /* wait for all operations from other processes to finish */
    if (win_ptr->at_completion_counter) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (win_ptr->at_completion_counter) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Progress_end(&progress_state);
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_WAIT);
                return mpi_errno;
            }
            /* --END ERROR HANDLING-- */
        }
        MPID_Progress_end(&progress_state);
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

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

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_PSCW &&
                        win_ptr->epoch_state != MPIDI_EPOCH_POST,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

    *flag = (win_ptr->at_completion_counter) ? 0 : 1;

    if (*flag) {
        /* Track access epoch state */
        if (win_ptr->epoch_state == MPIDI_EPOCH_PSCW)
            win_ptr->epoch_state = MPIDI_EPOCH_START;
        else
            win_ptr->epoch_state = MPIDI_EPOCH_NONE;

        /* Ensure ordering of load/store operations. */
        if (win_ptr->shm_allocated == TRUE) {
            OPA_read_write_barrier();
        }
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
    int mpi_errno = MPI_SUCCESS;
    struct MPIDI_Win_target_state *target_state;
    MPIDI_VC_t *orig_vc, *target_vc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_LOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_LOCK);

    MPIU_UNREFERENCED_ARG(assert);

    /* Even though we would want to reset the fence counter to keep
     * the user from using the previous fence to mark the beginning of
     * a fence epoch if he switched from fence to lock-unlock
     * synchronization, we cannot do this because fence_issued must be
     * updated collectively */

    if (dest == MPI_PROC_NULL)
        goto fn_exit;

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_NONE &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    target_state = &win_ptr->targets[dest];

    /* Check if a lock has already been issued */
    MPIU_ERR_CHKANDJUMP(target_state->remote_lock_state != MPIDI_CH3_WIN_LOCK_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Track access epoch state */
    if (win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL) {
        win_ptr->epoch_count++;
        win_ptr->epoch_state = MPIDI_EPOCH_LOCK;
    }

    target_state->remote_lock_state = MPIDI_CH3_WIN_LOCK_CALLED;
    target_state->remote_lock_mode = lock_type;
    target_state->remote_lock_assert = assert;

    if (dest == win_ptr->comm_ptr->rank) {
        /* The target is this process itself. We must block until the lock
         * is acquired.  Once it is acquired, local puts, gets, accumulates
         * will be done directly without queueing. */
        mpi_errno = acquire_local_lock(win_ptr, lock_type);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }
    else if (win_ptr->shm_allocated == TRUE) {
        /* Lock must be taken immediately for shared memory windows because of
         * load/store access */

        if (win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
            /* check if target is local and shared memory is allocated on window,
             * if so, we directly send lock request and wait for lock reply. */

            /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
             * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
             * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
             * which is only set to TRUE when SHM region is allocated in nemesis.
             * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
             */
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        }

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
            orig_vc->node_id == target_vc->node_id) {
            mpi_errno = send_lock_msg(dest, lock_type, win_ptr);
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }

            mpi_errno = wait_for_lock_granted(win_ptr, dest);
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
        }
    }
    else if (MPIR_CVAR_CH3_RMA_LOCK_IMMED && ((assert & MPI_MODE_NOCHECK) == 0)) {
        /* TODO: Make this mode of operation available through an assert
         * argument or info key. */
        mpi_errno = send_lock_msg(dest, lock_type, win_ptr);
        MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");
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
int MPIDI_Win_unlock(int dest, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int single_op_opt = 0;
    MPIDI_RMA_Op_t *rma_op;
    int wait_for_rma_done_pkt = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK);

    if (dest == MPI_PROC_NULL)
        goto fn_exit;

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIU_ERR_CHKANDJUMP(win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Track access epoch state */
    if (win_ptr->epoch_state == MPIDI_EPOCH_LOCK) {
        win_ptr->epoch_count--;
        if (win_ptr->epoch_count == 0)
            win_ptr->epoch_state = MPIDI_EPOCH_NONE;
    }

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    if (dest == win_ptr->comm_ptr->rank) {
        /* local lock. release the lock on the window, grant the next one
         * in the queue, and return. */
        MPIU_Assert(MPIDI_CH3I_RMA_Ops_isempty(&win_ptr->targets[dest].rma_ops_list));

        /* NOTE: We don't need to signal completion here becase a thread in the
         * same processes cannot lock the window again while it is already
         * locked. */
        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }
        win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;

        mpi_errno = MPID_Progress_poke();
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }
        goto fn_exit;
    }

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[dest].rma_ops_list);

    /* Lock was called, but the lock was not requested and there are no ops to
     * perform.  Do nothing and return. */
    if (rma_op == NULL && win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED) {
        win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;
        goto fn_exit;
    }

    /* TODO: MPI-3: Add lock->cas/fop/gacc->unlock optimization.  */
    /* TODO: MPI-3: Add lock_all->op optimization. */
    /* LOCK-OP-UNLOCK Optimization -- This optimization can't be used if we
     * have already requested the lock. */
    if (MPIR_CVAR_CH3_RMA_MERGE_LOCK_OP_UNLOCK &&
        win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED &&
        rma_op && rma_op->next == NULL /* There is only one op */  &&
        rma_op->type != MPIDI_RMA_COMPARE_AND_SWAP &&
        rma_op->type != MPIDI_RMA_FETCH_AND_OP && rma_op->type != MPIDI_RMA_GET_ACCUMULATE) {
        /* Single put, get, or accumulate between the lock and unlock. If it
         * is of small size and predefined datatype at the target, we
         * do an optimization where the lock and the RMA operation are
         * sent in a single packet. Otherwise, we send a separate lock
         * request first. */
        MPI_Aint type_size;
        MPIDI_VC_t *vc;
        MPIDI_RMA_Op_t *curr_op = rma_op;

        MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

        MPID_Datatype_get_size_macro(curr_op->origin_datatype, type_size);

        /* msg_sz typically = 65480 */
        if (MPIR_DATATYPE_IS_PREDEFINED(curr_op->target_datatype) &&
            (type_size * curr_op->origin_count <= vc->eager_max_msg_sz)) {
            single_op_opt = 1;
            /* Set the lock granted flag to 1 */
            win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
            if (curr_op->type == MPIDI_RMA_GET) {
                mpi_errno = send_lock_get(win_ptr, dest);
                wait_for_rma_done_pkt = 0;
            }
            else {
                mpi_errno = send_lock_put_or_acc(win_ptr, dest);
                wait_for_rma_done_pkt = 1;
            }
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
        }
    }

    if (single_op_opt == 0) {

        /* Send a lock packet over to the target and wait for the lock_granted
         * reply. If the user gave MODE_NOCHECK, we will piggyback the lock
         * request on the first RMA op.  Then do all the RMA ops. */

        if ((win_ptr->targets[dest].remote_lock_assert & MPI_MODE_NOCHECK) == 0) {
            if (win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED) {
                mpi_errno = send_lock_msg(dest, win_ptr->targets[dest].remote_lock_mode, win_ptr);
                if (mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }
            }
        }

        if (win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_REQUESTED) {
            mpi_errno = wait_for_lock_granted(win_ptr, dest);
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
        }

        /* Now do all the RMA operations */
        mpi_errno = do_passive_target_rma(win_ptr, dest, &wait_for_rma_done_pkt,
                                          MPIDI_CH3_PKT_FLAG_RMA_UNLOCK);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }

    /* If the lock is a shared lock or we have done the single op
     * optimization, we need to wait until the target informs us that
     * all operations are done on the target.  This ensures that third-
     * party communication can be done safely.  */
    if (wait_for_rma_done_pkt == 1) {
        /* wait until the "pt rma done" packet is received from the
         * target. This packet resets the remote_lock_state flag back to
         * NONE. */

        /* poke the progress engine until remote_lock_state flag is reset to NONE */
        if (win_ptr->targets[dest].remote_lock_state != MPIDI_CH3_WIN_LOCK_NONE) {
            MPID_Progress_state progress_state;

            MPID_Progress_start(&progress_state);
            while (win_ptr->targets[dest].remote_lock_state != MPIDI_CH3_WIN_LOCK_NONE) {
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
    }
    else {
        win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;
    }

    MPIU_Assert(MPIDI_CH3I_RMA_Ops_isempty(&win_ptr->targets[dest].rma_ops_list));

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
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPIDI_STATE_MPIDI_WIN_FLUSH_ALL);

    MPIDI_RMA_FUNC_ENTER(MPIDI_STATE_MPIDI_WIN_FLUSH_ALL);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* FIXME: Performance -- we should not process the ops separately.
     * Ideally, we should be able to use the same infrastructure that's used by
     * active target to complete all operations. */

    /* Note: Local RMA calls don't poke the progress engine.  This routine
     * should poke the progress engine when the local target is flushed to help
     * make asynchronous progress.  Currently this is handled by Win_flush().
     */
    for (i = 0; i < MPIR_Comm_size(win_ptr->comm_ptr); i++) {
        if (MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[i].rma_ops_list) == NULL)
            continue;
        if (win_ptr->targets[i].remote_lock_state != MPIDI_CH3_WIN_LOCK_NONE) {
            mpi_errno = win_ptr->RMAFns.Win_flush(i, win_ptr);
            if (mpi_errno != MPI_SUCCESS) {
                MPIU_ERR_POP(mpi_errno);
            }
        }
    }

    /* Ensure that all shared memory operations are flushed out.  The memory
     * barriers in the flush are not sufficient since we skip calling flush
     * when all operations are already completed. */
    if (win_ptr->shm_allocated == TRUE)
        OPA_read_write_barrier();

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
int MPIDI_Win_flush(int rank, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int wait_for_rma_done_pkt = 0;
    MPIDI_RMA_Op_t *rma_op;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FLUSH);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Check if win_lock was called */
    MPIU_ERR_CHKANDJUMP(win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Ensure ordering of read/write operations */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }

    /* Local flush: ops are performed immediately on the local process */
    if (rank == win_ptr->comm_ptr->rank) {
        MPIU_Assert(win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_GRANTED);
        MPIU_Assert(MPIDI_CH3I_RMA_Ops_isempty(&win_ptr->targets[rank].rma_ops_list));

        /* If flush is used as a part of polling for incoming data, we can
         * deadlock, since local RMA calls never poke the progress engine.  So,
         * make extra progress here to avoid this problem. */
        mpi_errno = MPIDI_CH3_Progress_poke();
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* NOTE: All flush and req-based operations are currently implemented in
     * terms of MPIDI_Win_flush.  When this changes, those operations will also
     * need to insert this read/write memory fence for shared memory windows. */

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[rank].rma_ops_list);

    /* If there is no activity at this target (e.g. lock-all was called, but we
     * haven't communicated with this target), don't do anything. */
    if (win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED && rma_op == NULL) {
        goto fn_exit;
    }

    /* MT: If another thread is performing a flush, wait for them to finish. */
    if (win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_FLUSH) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED) {
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

    /* Send a lock packet over to the target, wait for the lock_granted
     * reply, and perform the RMA ops. */

    if (win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED) {
        mpi_errno = send_lock_msg(rank, win_ptr->targets[rank].remote_lock_mode, win_ptr);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }

    if (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED) {
        mpi_errno = wait_for_lock_granted(win_ptr, rank);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }

    win_ptr->targets[rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_FLUSH;
    mpi_errno = do_passive_target_rma(win_ptr, rank, &wait_for_rma_done_pkt,
                                      MPIDI_CH3_PKT_FLAG_RMA_FLUSH);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* If the lock is a shared lock or we have done the single op optimization,
     * we need to wait until the target informs us that all operations are done
     * on the target.  This ensures that third-party communication can be done
     * safely.  */
    if (wait_for_rma_done_pkt == 1) {
        /* wait until the "pt rma done" packet is received from the target.
         * This packet resets the remote_lock_state flag. */

        if (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED) {
            MPID_Progress_state progress_state;

            MPID_Progress_start(&progress_state);
            while (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED) {
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
    }
    else {
        win_ptr->targets[rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
    }

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
int MPIDI_Win_flush_local(int rank, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Note: Local RMA calls don't poke the progress engine.  This routine
     * should poke the progress engine when the local target is flushed to help
     * make asynchronous progress.  Currently this is handled by Win_flush().
     */

    mpi_errno = win_ptr->RMAFns.Win_flush(rank, win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

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
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_FLUSH_LOCAL_ALL);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Note: Local RMA calls don't poke the progress engine.  This routine
     * should poke the progress engine when the local target is flushed to help
     * make asynchronous progress.  Currently this is handled by Win_flush().
     */

    mpi_errno = win_ptr->RMAFns.Win_flush_all(win_ptr);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

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
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *orig_vc, *target_vc;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_LOCK_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_LOCK_ALL);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Track access epoch state */
    win_ptr->epoch_state = MPIDI_EPOCH_LOCK_ALL;

    /* Set the target's lock state to "called" for all targets */
    /* FIXME: Don't use this O(p) approach */
    for (i = 0; i < MPIR_Comm_size(win_ptr->comm_ptr); i++) {
        MPIU_Assert(win_ptr->targets[i].remote_lock_state == MPIDI_CH3_WIN_LOCK_NONE);

        win_ptr->targets[i].remote_lock_state = MPIDI_CH3_WIN_LOCK_CALLED;
        win_ptr->targets[i].remote_lock_mode = MPI_LOCK_SHARED;
        win_ptr->targets[i].remote_lock_assert = assert;
    }

    /* Immediately lock the local process for load/store access */
    mpi_errno = acquire_local_lock(win_ptr, MPI_LOCK_SHARED);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_POP(mpi_errno);
    }

    if (win_ptr->shm_allocated == TRUE) {
        /* Immediately lock all targets for load/store access */

        for (i = 0; i < MPIR_Comm_size(win_ptr->comm_ptr); i++) {
            /* Local process is already locked */
            if (i == win_ptr->comm_ptr->rank)
                continue;

            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
                /* check if target is local and shared memory is allocated on window,
                 * if so, we directly send lock request and wait for lock reply. */

                /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
                 * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
                 * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
                 * which is only set to TRUE when SHM region is allocated in nemesis.
                 * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
                 */
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
            }

            if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
                orig_vc->node_id == target_vc->node_id) {
                mpi_errno = send_lock_msg(i, MPI_LOCK_SHARED, win_ptr);
                if (mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }
            }
        }

        for (i = 0; i < MPIR_Comm_size(win_ptr->comm_ptr); i++) {
            /* Local process is already locked */
            if (i == win_ptr->comm_ptr->rank)
                continue;

            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
            }

            if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
                orig_vc->node_id == target_vc->node_id) {
                mpi_errno = wait_for_lock_granted(win_ptr, i);
                if (mpi_errno) {
                    MPIU_ERR_POP(mpi_errno);
                }
            }
        }
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
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK_ALL);

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    /* Note: Win_unlock currently provides a fence for shared memory windows.
     * If the implementation changes, a fence is needed here. */

    for (i = 0; i < MPIR_Comm_size(win_ptr->comm_ptr); i++) {
        mpi_errno = win_ptr->RMAFns.Win_unlock(i, win_ptr);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }
    }

    /* Track access epoch state */
    win_ptr->epoch_state = MPIDI_EPOCH_NONE;

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
    MPI_Win source_win_handle = MPI_WIN_NULL, target_win_handle = MPI_WIN_NULL;
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
        MPIDI_RMA_Op_t *tail = MPIDI_CH3I_RMA_Ops_tail(&win_ptr->targets[target_rank].rma_ops_list);

        /* Check if we can piggyback the RMA done acknowlegdement on the last
         * operation in the epoch. */

        if (tail->type == MPIDI_RMA_GET ||
            tail->type == MPIDI_RMA_COMPARE_AND_SWAP ||
            tail->type == MPIDI_RMA_FETCH_AND_OP || tail->type == MPIDI_RMA_GET_ACCUMULATE) {
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
                if (curr_ptr->type == MPIDI_RMA_GET) {
                    /* Found a GET, move it to the end */
                    *wait_for_rma_done_pkt = 0;

                    MPIDI_CH3I_RMA_Ops_unlink(&win_ptr->targets[target_rank].rma_ops_list,
                                              curr_ptr);
                    MPIDI_CH3I_RMA_Ops_append(&win_ptr->targets[target_rank].rma_ops_list,
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

    if (curr_ptr != NULL) {
        target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
    }

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

            source_win_handle = win_ptr->handle;
        }

        mpi_errno = MPIDI_CH3I_Issue_rma_op(curr_ptr, win_ptr, flags, source_win_handle,
                                            target_win_handle);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        /* If the request is null, we can remove it immediately */
        if (!curr_ptr->request) {
            MPIDI_CH3I_RMA_Ops_free_and_next(&win_ptr->targets[target_rank].rma_ops_list,
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
                    rma_list_gc(win_ptr, &win_ptr->targets[target_rank].rma_ops_list, curr_ptr,
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
        mpi_errno = rma_list_complete(win_ptr, &win_ptr->targets[target_rank].rma_ops_list);
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

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state != MPIDI_EPOCH_LOCK &&
                        win_ptr->epoch_state != MPIDI_EPOCH_LOCK_ALL,
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

    MPIDI_STATE_DECL(MPID_STATE_SEND_LOCK_PUT_OR_ACC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_LOCK_PUT_OR_ACC);

    lock_type = win_ptr->targets[target_rank].remote_lock_mode;

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    if (rma_op->type == MPIDI_RMA_PUT) {
        MPIDI_Pkt_init(lock_put_unlock_pkt, MPIDI_CH3_PKT_LOCK_PUT_UNLOCK);
        lock_put_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
            MPIDI_CH3_PKT_FLAG_RMA_UNLOCK | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
        lock_put_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
        lock_put_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_put_unlock_pkt->lock_type = lock_type;

        lock_put_unlock_pkt->addr =
            (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;

        lock_put_unlock_pkt->count = rma_op->target_count;
        lock_put_unlock_pkt->datatype = rma_op->target_datatype;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_put_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_put_unlock_pkt);
    }

    else if (rma_op->type == MPIDI_RMA_ACCUMULATE) {
        MPIDI_Pkt_init(lock_accum_unlock_pkt, MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK);
        lock_accum_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
            MPIDI_CH3_PKT_FLAG_RMA_UNLOCK | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
        lock_accum_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
        lock_accum_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_accum_unlock_pkt->lock_type = lock_type;

        lock_accum_unlock_pkt->addr =
            (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;

        lock_accum_unlock_pkt->count = rma_op->target_count;
        lock_accum_unlock_pkt->datatype = rma_op->target_datatype;
        lock_accum_unlock_pkt->op = rma_op->op;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_accum_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_accum_unlock_pkt);
    }
    else if (rma_op->type == MPIDI_RMA_ACC_CONTIG) {
        MPIDI_Pkt_init(lock_accum_unlock_pkt, MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK);
        lock_accum_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
            MPIDI_CH3_PKT_FLAG_RMA_UNLOCK | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
        lock_accum_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
        lock_accum_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_accum_unlock_pkt->lock_type = lock_type;

        lock_accum_unlock_pkt->addr =
            (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;

        lock_accum_unlock_pkt->count = rma_op->target_count;
        lock_accum_unlock_pkt->datatype = rma_op->target_datatype;
        lock_accum_unlock_pkt->op = rma_op->op;

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
    MPIDI_CH3I_RMA_Ops_free(&win_ptr->targets[target_rank].rma_ops_list);

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

    MPIDI_Pkt_init(lock_get_unlock_pkt, MPIDI_CH3_PKT_LOCK_GET_UNLOCK);
    lock_get_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK | MPIDI_CH3_PKT_FLAG_RMA_UNLOCK;   /* FIXME | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK; */
    lock_get_unlock_pkt->target_win_handle = win_ptr->all_win_handles[rma_op->target_rank];
    lock_get_unlock_pkt->source_win_handle = win_ptr->handle;
    lock_get_unlock_pkt->lock_type = lock_type;

    lock_get_unlock_pkt->addr =
        (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;

    lock_get_unlock_pkt->count = rma_op->target_count;
    lock_get_unlock_pkt->datatype = rma_op->target_datatype;
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
    MPIDI_CH3I_RMA_Ops_free(&win_ptr->targets[target_rank].rma_ops_list);

  fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_LOCK_GET);
    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/* list_complete_timer/counter and list_block_timer defined above */

static inline int rma_list_complete(MPID_Win * win_ptr, MPIDI_RMA_Ops_list_t * ops_list)
{
    int ntimes = 0, mpi_errno = 0;
    MPIDI_RMA_Op_t *curr_ptr;
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    /* Process all operations until they are complete */
    while (!MPIDI_CH3I_RMA_Ops_isempty(ops_list)) {
        int nDone = 0;
        mpi_errno = rma_list_gc(win_ptr, ops_list, NULL, &nDone);
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
                MPIDI_CH3I_RMA_Ops_free_and_next(ops_list, &curr_ptr);
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
