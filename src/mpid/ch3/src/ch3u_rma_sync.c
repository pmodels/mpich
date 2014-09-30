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
    - name        : MPIR_CVAR_CH3_RMA_ACC_IMMED
      category    : CH3
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use the immediate accumulate optimization

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

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_lockqueue_alloc);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfence_clearlock);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winfence_clearlock_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfence_rs);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfence_issue);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winfence_issue_aux);
MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL(RMA, rma_winfence_issue_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfence_complete);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winfence_complete_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfence_wait);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winfence_wait_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfence_block);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winfence_block_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winpost_clearlock);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winpost_clearlock_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winpost_sendsync);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winpost_sendsync_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winstart_clearlock);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winstart_clearlock_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_wincomplete_issue);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_wincomplete_issue_aux);
MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL(RMA, rma_wincomplete_issue_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_wincomplete_complete);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_wincomplete_complete_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_wincomplete_recvsync);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_wincomplete_recvsync_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_wincomplete_block);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_wincomplete_block_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winwait_wait);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winwait_wait_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winlock_getlocallock);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winunlock_getlock);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winunlock_issue);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winunlock_complete);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winunlock_complete_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winunlock_block);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winunlock_block_aux);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_acc);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_acc_predef);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_acc_immed);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_acc_immed_op);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_cas);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_fop);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_get_accum);

MPIR_T_PVAR_ULONG2_LEVEL_DECL(RMA, rma_winfence_reqs);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winfence_reqs);
MPIR_T_PVAR_ULONG2_HIGHWATERMARK_DECL(RMA, rma_winfence_reqs);

MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_winunlock_reqs);
MPIR_T_PVAR_ULONG2_COUNTER_DECL(RMA, rma_wincomplete_reqs);

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_wincreate_allgather);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfree_rs);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_winfree_complete);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmaqueue_alloc);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmaqueue_set);

void MPIDI_CH3_RMA_Init_Pvars(void)
{
    /* rma_lockqueue_alloc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_lockqueue_alloc,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "Allocate Lock Queue element (in seconds)");

    /* rma_winfence_clearlock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfence_clearlock,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Clear prior lock (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_clearlock_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Clear prior lock");

    /* rma_winfence_rs */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfence_rs,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:ReduceScatterBlock (in seconds)");

    /* rma_winfence_issue and auxiliaries */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfence_issue,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Issue RMA ops (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_issue_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Issue RMA ops");

    MPIR_T_PVAR_HIGHWATERMARK_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_issue_aux,
        0, /* init value */
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Issue RMA ops");

    /* rma_winfence_complete */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfence_complete,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Complete RMA ops (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_complete_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Complete RMA ops");

    /* rma_winfence_wait */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfence_wait,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Wait for ops from other processes (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_wait_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Wait for ops from other processes");

    /* rma_winfence_block */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfence_block,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Wait for any progress (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_block_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Wait for any progress");


    /* rma_winpost_clearlock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winpost_clearlock,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_POST:Clear prior lock  (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winpost_clearlock_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_POST:Clear prior lock");

    /* rma_winpost_sendsync */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winpost_sendsync,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_POST:Senc sync messages (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winpost_sendsync_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_POST:Senc sync messages");

    /* rma_winstart_clearlock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winstart_clearlock,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_START:Clear prior lock (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winstart_clearlock_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_START:Clear prior lock");

    /* rma_wincomplete_issue and auxiliaries */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_wincomplete_issue,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Issue RMA ops (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_wincomplete_issue_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Issue RMA ops");

    MPIR_T_PVAR_HIGHWATERMARK_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_wincomplete_issue_aux,
        0, /* init value */
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Issue RMA ops");

    /* rma_wincomplete_complete */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_wincomplete_complete,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Complete RMA ops (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_wincomplete_complete_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Complete RMA ops");

    /* rma_wincomplete_recvsync */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_wincomplete_recvsync,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Recv sync messages (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_wincomplete_recvsync_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Recv sync messages");

    /* rma_wincomplete_block */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_wincomplete_block,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Wait for any progress (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_wincomplete_block_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Wait for any progress");

    /* rma_winwait_wait */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winwait_wait,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_WAIT:Wait for ops from other processes (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winwait_wait_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_WAIT:Wait for ops from other processes");

    /* rma_winlock_getlocallock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winlock_getlocallock,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_LOCK:Get local lock (in seconds)");

    /* rma_winunlock_getlock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winunlock_getlock,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_UNLOCK:Acquire lock (in seconds)");

    /* rma_winunlock_issue */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winunlock_issue,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_UNLOCK:Issue RMA ops (in seconds)");

    /* rma_winunlock_complete */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winunlock_complete,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_UNLOCK:Complete RMA ops (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winunlock_complete_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_UNLOCK:Complete RMA ops (in seconds)");

    /* rma_winunlock_block */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winunlock_block,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_UNLOCK:Wait for any progress (in seconds)");

    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winunlock_block_aux,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_UNLOCK:Wait for any progress");

    /* rma_rmapkt_acc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmapkt_acc,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "RMA:PKTHANDLER for Accumulate (in seconds)");

    /* rma_rmapkt_acc_predef */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmapkt_acc_predef,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "RMA:PKTHANDLER for Accumulate: predef dtype (in seconds)");

    /* rma_rmapkt_acc_immed */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmapkt_acc_immed,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "RMA:PKTHANDLER for Accum immed (in seconds)");

    /* rma_rmapkt_acc_immed_op */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmapkt_acc_immed_op,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "RMA:PKTHANDLER for Accum immed operation (in seconds)");

    /* rma_rmapkt_cas */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmapkt_cas,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "RMA:PKTHANDLER for Compare-and-swap (in seconds)");

    /* rma_rmapkt_fop */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmapkt_fop,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "RMA:PKTHANDLER for Fetch-and-op (in seconds)");

    /* rma_rmapkt_get_accum */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmapkt_get_accum,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "RMA:PKTHANDLER for Get-Accumulate (in seconds)");

    /* Level, counter and highwatermark for rma_winfence_reqs */
    MPIR_T_PVAR_LEVEL_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_reqs,
        0, /* init value */
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY | MPIR_T_PVAR_FLAG_CONTINUOUS,
        "RMA",
        "WIN_FENCE:Pending requests");

      MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_reqs,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Pending requests");

      MPIR_T_PVAR_HIGHWATERMARK_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winfence_reqs,
        0, /* init value */
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FENCE:Pending requests");

    /* rma_winunlock_reqs */
    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_winunlock_reqs,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_UNLOCK:Pending requests");

    /* rma_wincomplete_reqs */
    MPIR_T_PVAR_COUNTER_REGISTER_STATIC(
        RMA,
        MPI_UNSIGNED_LONG_LONG,
        rma_wincomplete_reqs,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_COMPLETE:Pending requests");

    /* rma_wincreate_allgather */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_wincreate_allgather,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_CREATE:Allgather (in seconds)");

    /* rma_winfree_rs */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfree_rs,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FREE:ReduceScatterBlock (in seconds)");

    /* rma_winfree_complete */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_winfree_complete,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "WIN_FREE:Complete (in seconds)");

    /* rma_rmaqueue_alloc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmaqueue_alloc,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "Allocate RMA Queue element (in seconds)");

    /* rma_rmaqueue_set */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(
        RMA,
        MPI_DOUBLE,
        rma_rmaqueue_set,
        MPI_T_VERBOSITY_MPIDEV_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_READONLY,
        "RMA",
        "Set fields in RMA Queue element (in seconds)");
}

/* These are used to use a common routine to complete lists of RMA 
   operations with a single routine, while collecting data that 
   distinguishes between different synchronization modes.  This is not
   thread-safe; the best choice for thread-safety is to eliminate this
   ability to discriminate between the different types of RMA synchronization.
*/
static MPIR_T_pvar_timer_t *list_complete_timer;  /* outer */
static unsigned long long *list_complete_counter;
static MPIR_T_pvar_timer_t *list_block_timer;     /* Inner; while waiting */

/*
 * These routines provide a default implementation of the MPI RMA operations
 * in terms of the low-level, two-sided channel operations.  A channel
 * may override these functions, on a per-window basis, by overriding
 * the MPID functions in the RMAFns section of MPID_Win object.
 */

#define SYNC_POST_TAG 100

static int send_lock_msg(int dest, int lock_type, MPID_Win *win_ptr);
static int send_unlock_msg(int dest, MPID_Win *win_ptr);
/* static int send_flush_msg(int dest, MPID_Win *win_ptr); */
static int wait_for_lock_granted(MPID_Win *win_ptr, int target_rank);
static int acquire_local_lock(MPID_Win *win_ptr, int lock_mode);
static int send_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                                   MPIDI_CH3_Pkt_flags_t flags,
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info * dtype_info, 
				   void ** dataloop, MPID_Request ** request);
static int recv_rma_msg(MPIDI_RMA_Op_t * rma_op, MPID_Win * win_ptr,
                                   MPIDI_CH3_Pkt_flags_t flags,
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info * dtype_info, 
				   void ** dataloop, MPID_Request ** request); 
static int send_contig_acc_msg(MPIDI_RMA_Op_t *, MPID_Win *,
                                          MPIDI_CH3_Pkt_flags_t flags,
					  MPI_Win, MPI_Win, MPID_Request ** );
static int send_immed_rmw_msg(MPIDI_RMA_Op_t *, MPID_Win *,
                                         MPIDI_CH3_Pkt_flags_t flags,
                                         MPI_Win, MPI_Win, MPID_Request ** );
static int do_passive_target_rma(MPID_Win *win_ptr, int target_rank,
                                            int *wait_for_rma_done_pkt,
                                            MPIDI_CH3_Pkt_flags_t sync_flags);
static int send_lock_put_or_acc(MPID_Win *, int);
static int send_lock_get(MPID_Win *, int);
static inline int poke_progress_engine(void);
static inline int rma_list_complete(MPID_Win *win_ptr,
                                      MPIDI_RMA_Ops_list_t *ops_list);
static inline int rma_list_gc(MPID_Win *win_ptr,
                                             MPIDI_RMA_Ops_list_t *ops_list,
                                             MPIDI_RMA_Op_t *last_elm, int *nDone);

static int create_datatype(const MPIDI_RMA_dtype_info *dtype_info,
                           const void *dataloop, MPI_Aint dataloop_sz,
                           const void *o_addr, int o_count,
			   MPI_Datatype o_datatype,
                           MPID_Datatype **combined_dtp);

/* Issue an RMA operation -- Before calling this macro, you must define the
 * MPIDI_CH3I_TRACK_RMA_WRITE helper macro.  This macro defines any extra action
 * that should be taken when a write (put/acc) operation is encountered. */
#define MPIDI_CH3I_ISSUE_RMA_OP(op_ptr_, win_ptr_, flags_, source_win_handle_, target_win_handle_,err_) \
    do {                                                                                        \
    switch ((op_ptr_)->type)                                                                    \
    {                                                                                           \
        case (MPIDI_RMA_PUT):                                                                   \
        case (MPIDI_RMA_ACCUMULATE):                                                            \
            MPIDI_CH3I_TRACK_RMA_WRITE(op_ptr_, win_ptr_);                                      \
            (err_) = send_rma_msg((op_ptr_), (win_ptr_), (flags_), (source_win_handle_),        \
                                                (target_win_handle_), &(op_ptr_)->dtype_info,   \
                                                &(op_ptr_)->dataloop, &(op_ptr_)->request);     \
            if (err_) { MPIU_ERR_POP(err_); }                                                   \
            break;                                                                              \
        case (MPIDI_RMA_GET_ACCUMULATE):                                                        \
            if ((op_ptr_)->op == MPI_NO_OP) {                                                   \
                /* Note: Origin arguments are ignored for NO_OP, so we don't                    \
                 * need to release a ref to the origin datatype. */                             \
                                                                                                \
                /* Convert the GAcc to a Get */                                                 \
                (op_ptr_)->type            = MPIDI_RMA_GET;                                     \
                (op_ptr_)->origin_addr     = (op_ptr_)->result_addr;                            \
                (op_ptr_)->origin_count    = (op_ptr_)->result_count;                           \
                (op_ptr_)->origin_datatype = (op_ptr_)->result_datatype;                        \
                                                                                                \
                (err_) = recv_rma_msg((op_ptr_), (win_ptr_), (flags_), (source_win_handle_),    \
                                                    (target_win_handle_), &(op_ptr_)->dtype_info,\
                                                    &(op_ptr_)->dataloop, &(op_ptr_)->request); \
            } else {                                                                            \
                MPIDI_CH3I_TRACK_RMA_WRITE(op_ptr_, win_ptr_);                                  \
                (err_) = send_rma_msg((op_ptr_), (win_ptr_), (flags_), (source_win_handle_),    \
                                                    (target_win_handle_), &(op_ptr_)->dtype_info,\
                                                    &(op_ptr_)->dataloop, &(op_ptr_)->request); \
            }                                                                                   \
            if (err_) { MPIU_ERR_POP(err_); }                                                   \
            break;                                                                              \
        case MPIDI_RMA_ACC_CONTIG:                                                              \
            MPIDI_CH3I_TRACK_RMA_WRITE(op_ptr_, win_ptr_);                                      \
            (err_) = send_contig_acc_msg((op_ptr_), (win_ptr_), (flags_),                       \
                                                       (source_win_handle_), (target_win_handle_),\
                                                       &(op_ptr_)->request );                   \
            if (err_) { MPIU_ERR_POP(err_); }                                                   \
            break;                                                                              \
        case (MPIDI_RMA_GET):                                                                   \
            (err_) = recv_rma_msg((op_ptr_), (win_ptr_), (flags_),                              \
                                                (source_win_handle_), (target_win_handle_),     \
                                                &(op_ptr_)->dtype_info,                         \
                                                &(op_ptr_)->dataloop, &(op_ptr_)->request);     \
            if (err_) { MPIU_ERR_POP(err_); }                                                   \
            break;                                                                              \
        case (MPIDI_RMA_COMPARE_AND_SWAP):                                                      \
        case (MPIDI_RMA_FETCH_AND_OP):                                                          \
            MPIDI_CH3I_TRACK_RMA_WRITE(op_ptr_, win_ptr_);                                      \
            (err_) = send_immed_rmw_msg((op_ptr_), (win_ptr_), (flags_),                        \
                                                      (source_win_handle_), (target_win_handle_),\
                                                      &(op_ptr_)->request );                    \
            if (err_) { MPIU_ERR_POP(err_); }                                                   \
            break;                                                                              \
                                                                                                \
        default:                                                                                \
            MPIU_ERR_SETANDJUMP(err_,MPI_ERR_OTHER,"**winInvalidOp");                           \
    }                                                                                           \
    } while (0)


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_fence(int assert, MPID_Win *win_ptr)
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

    /* In case this process was previously the target of passive target rma
     * operations, we need to take care of the following...
     * Since we allow MPI_Win_unlock to return without a done ack from
     * the target in the case of multiple rma ops and exclusive lock,
     * we need to check whether there is a lock on the window, and if
     * there is a lock, poke the progress engine until the operartions
     * have completed and the lock is released. */
    if (win_ptr->current_lock_type != MPID_LOCK_NONE)
    {
	MPIR_T_PVAR_TIMER_START(RMA, rma_winfence_clearlock);
	MPID_Progress_start(&progress_state);
	while (win_ptr->current_lock_type != MPID_LOCK_NONE)
	{
	    /* poke the progress engine */
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS) {
		MPID_Progress_end(&progress_state);
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
	    }
	    /* --END ERROR HANDLING-- */
	    MPIR_T_PVAR_COUNTER_INC(RMA, rma_winfence_clearlock_aux, 1);
	}
	MPID_Progress_end(&progress_state);
	MPIR_T_PVAR_TIMER_END(RMA, rma_winfence_clearlock);
    }

    /* Note that the NOPRECEDE and NOSUCCEED must be specified by all processes
       in the window's group if any specify it */
    if (assert & MPI_MODE_NOPRECEDE)
    {
        /* Error: Operations were issued and the user claimed NOPRECEDE */
        MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_FENCE,
                            mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

	win_ptr->fence_issued = (assert & MPI_MODE_NOSUCCEED) ? 0 : 1;
	goto shm_barrier;
    }
    
    if (win_ptr->fence_issued == 0)
    {
	/* win_ptr->fence_issued == 0 means either this is the very first
	   call to fence or the preceding fence had the
	   MPI_MODE_NOSUCCEED assert. 

           If this fence has MPI_MODE_NOSUCCEED, do nothing and return.
	   Otherwise just increment the fence count and return. */

	if (!(assert & MPI_MODE_NOSUCCEED)) win_ptr->fence_issued = 1;
    }
    else
    {
	int nRequest = 0;
	int nRequestNew = 0;

        /* Ensure ordering of load/store operations. */
        if (win_ptr->shm_allocated == TRUE) {
           OPA_read_write_barrier();
        }

	MPIR_T_PVAR_TIMER_START(RMA, rma_winfence_rs);
	/* This is the second or later fence. Do all the preceding RMA ops. */
	comm_ptr = win_ptr->comm_ptr;
	/* First inform every process whether it is a target of RMA
	   ops from this process */
	comm_size = comm_ptr->local_size;

	MPIU_CHKLMEM_MALLOC(rma_target_proc, int *, comm_size*sizeof(int),
			    mpi_errno, "rma_target_proc");
	for (i=0; i<comm_size; i++) rma_target_proc[i] = 0;
	
	/* keep track of no. of ops to each proc. Needed for knowing
	   whether or not to decrement the completion counter. The
	   completion counter is decremented only on the last
	   operation. */
	MPIU_CHKLMEM_MALLOC(nops_to_proc, int *, comm_size*sizeof(int),
			    mpi_errno, "nops_to_proc");
	for (i=0; i<comm_size; i++) nops_to_proc[i] = 0;

        /* Note, active target uses the following ops list, and passive
           target uses win_ptr->targets[..] */
        ops_list = &win_ptr->at_rma_ops_list;

	/* set rma_target_proc[i] to 1 if rank i is a target of RMA
	   ops from this process */
	total_op_count = 0;
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
	while (curr_ptr != NULL)
	{
	    total_op_count++;
	    rma_target_proc[curr_ptr->target_rank] = 1;
	    nops_to_proc[curr_ptr->target_rank]++;
	    curr_ptr = curr_ptr->next;
	}
	
	MPIU_CHKLMEM_MALLOC(curr_ops_cnt, int *, comm_size*sizeof(int),
			    mpi_errno, "curr_ops_cnt");
	for (i=0; i<comm_size; i++) curr_ops_cnt[i] = 0;
	/* do a reduce_scatter_block (with MPI_SUM) on rma_target_proc. 
	   As a result,
	   each process knows how many other processes will be doing
	   RMA ops on its window */  
            
	/* first initialize the completion counter. */
	win_ptr->at_completion_counter += comm_size;
            
	mpi_errno = MPIR_Reduce_scatter_block_impl(MPI_IN_PLACE, rma_target_proc, 1,
                                                   MPI_INT, MPI_SUM, comm_ptr, &errflag);
	MPIR_T_PVAR_TIMER_END(RMA, rma_winfence_rs);
	/* result is stored in rma_target_proc[0] */
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        MPIU_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* Ensure ordering of load/store operations. */
        if (win_ptr->shm_allocated == TRUE) {
            OPA_read_write_barrier();
        }

	/* Set the completion counter */
	/* FIXME: MT: this needs to be done atomically because other
	   procs have the address and could decrement it. */
        win_ptr->at_completion_counter -= comm_size;
        win_ptr->at_completion_counter += rma_target_proc[0];

    MPIR_T_PVAR_TIMER_START(RMA, rma_winfence_issue);
    MPIR_T_PVAR_COUNTER_INC(RMA, rma_winfence_issue_aux, total_op_count);
    MPIR_T_PVAR_ULONG2_HIGHWATERMARK_UPDATE(RMA, rma_winfence_issue_aux, total_op_count);

    MPIR_T_PVAR_ULONG2_HIGHWATERMARK_UPDATE(RMA, rma_winfence_reqs, MPIR_T_PVAR_LEVEL_GET(rma_winfence_reqs));
    MPIR_T_PVAR_LEVEL_SET(RMA, rma_winfence_reqs, 0); /* reset the level */
    MPIR_T_PVAR_COUNTER_INC(RMA, rma_winfence_reqs, 1);

	i = 0;
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
	while (curr_ptr != NULL)
	{
            MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;

	    /* The completion counter at the target is decremented only on 
	       the last RMA operation. */
	    if (curr_ops_cnt[curr_ptr->target_rank] ==
                nops_to_proc[curr_ptr->target_rank] - 1) {
                flags = MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE;
            }

            source_win_handle = win_ptr->handle;
	    target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];

#define MPIDI_CH3I_TRACK_RMA_WRITE(op_ptr_, win_ptr_) /* Not used by active mode */
            MPIDI_CH3I_ISSUE_RMA_OP(curr_ptr, win_ptr, flags,
                                    source_win_handle, target_win_handle, mpi_errno);
#undef MPIDI_CH3I_TRACK_RMA_WRITE

	    i++;
	    curr_ops_cnt[curr_ptr->target_rank]++;
	    /* If the request is null, we can remove it immediately */
	    if (!curr_ptr->request) {
                MPIDI_CH3I_RMA_Ops_free_and_next(ops_list, &curr_ptr);
	    }
	    else  {
		nRequest++;
		MPIR_T_PVAR_LEVEL_INC(RMA, rma_winfence_reqs, 1);
		curr_ptr    = curr_ptr->next;
		/* The test on the difference is to reduce the number
		   of times the partial complete routine is called. Without
		   this, significant overhead is added once the
		   number of requests exceeds the threshold, since the
		   number that are completed in a call may be small. */
		if (nRequest > MPIR_CVAR_CH3_RMA_NREQUEST_THRESHOLD &&
		    nRequest - nRequestNew > MPIR_CVAR_CH3_RMA_NREQUEST_NEW_THRESHOLD) {
		    int nDone = 0;
                    mpi_errno = poke_progress_engine();
                    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIR_T_PVAR_STMT(RMA, list_complete_timer=MPIR_T_PVAR_TIMER_ADDR(rma_winfence_complete));
            MPIR_T_PVAR_STMT(RMA, list_complete_counter=MPIR_T_PVAR_COUNTER_ADDR(rma_winfence_complete_aux));

                    mpi_errno = rma_list_gc(win_ptr, ops_list, curr_ptr, &nDone);
                    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		    /* if (nDone > 0) printf( "nDone = %d\n", nDone ); */
		    nRequest -= nDone;
		    nRequestNew = nRequest;
		}
	    }
	}
	MPIR_T_PVAR_TIMER_END(RMA, rma_winfence_issue);

	/* We replaced a loop over an array of requests with a list of the
	   incomplete requests.  The reason to do 
	   that is for long lists - processing the entire list until
	   all are done introduces a potentially n^2 time.  In 
	   testing with test/mpi/perf/manyrma.c , the number of iterations
	   within the "while (total_op_count) was O(total_op_count).
	   
	   Another alternative is to create a more compressed list (storing
	   only the necessary information, reducing the number of cache lines
	   needed while looping through the requests.
	*/
	if (total_op_count)
	{ 
        MPIR_T_PVAR_STMT(RMA, list_complete_timer=MPIR_T_PVAR_TIMER_ADDR(rma_winfence_complete));
        MPIR_T_PVAR_STMT(RMA, list_complete_counter=MPIR_T_PVAR_COUNTER_ADDR(rma_winfence_complete_aux));
        MPIR_T_PVAR_STMT(RMA, list_block_timer=MPIR_T_PVAR_TIMER_ADDR(rma_winfence_block));
            mpi_errno = rma_list_complete(win_ptr, ops_list);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
	}

        /* MT: avoid processing unissued operations enqueued by other threads
           in rma_list_complete() */
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
        if (curr_ptr && !curr_ptr->request)
            goto finish_up;
        MPIU_Assert(MPIDI_CH3I_RMA_Ops_isempty(ops_list));
	
 finish_up:
	/* wait for all operations from other processes to finish */
	if (win_ptr->at_completion_counter)
	{
	    MPIR_T_PVAR_TIMER_START(RMA, rma_winfence_wait);
	    MPID_Progress_start(&progress_state);
	    while (win_ptr->at_completion_counter)
	    {
		mpi_errno = MPID_Progress_wait(&progress_state);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS) {
		    MPID_Progress_end(&progress_state);
		    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
		}
		/* --END ERROR HANDLING-- */
		MPIR_T_PVAR_COUNTER_INC(RMA, rma_winfence_wait_aux, 1);
	    }
	    MPID_Progress_end(&progress_state);
	    MPIR_T_PVAR_TIMER_END(RMA, rma_winfence_wait);
	} 
	
	if (assert & MPI_MODE_NOSUCCEED)
	{
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

/* create_datatype() creates a new struct datatype for the dtype_info
   and the dataloop of the target datatype together with the user data */
#undef FUNCNAME
#define FUNCNAME create_datatype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int create_datatype(const MPIDI_RMA_dtype_info *dtype_info,
                           const void *dataloop, MPI_Aint dataloop_sz,
                           const void *o_addr, int o_count, MPI_Datatype o_datatype,
                           MPID_Datatype **combined_dtp)
{
    int mpi_errno = MPI_SUCCESS;
    /* datatype_set_contents wants an array 'ints' which is the
       blocklens array with count prepended to it.  So blocklens
       points to the 2nd element of ints to avoid having to copy
       blocklens into ints later. */
    int ints[4];
    int *blocklens = &ints[1];
    MPI_Aint displaces[3];
    MPI_Datatype datatypes[3];
    const int count = 3;
    MPI_Datatype combined_datatype;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_DATATYPE);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_DATATYPE);

    /* create datatype */
    displaces[0] = MPIU_PtrToAint(dtype_info);
    blocklens[0] = sizeof(*dtype_info);
    datatypes[0] = MPI_BYTE;
    
    displaces[1] = MPIU_PtrToAint(dataloop);
    MPIU_Assign_trunc(blocklens[1], dataloop_sz, int);
    datatypes[1] = MPI_BYTE;
    
    displaces[2] = MPIU_PtrToAint(o_addr);
    blocklens[2] = o_count;
    datatypes[2] = o_datatype;
    
    mpi_errno = MPID_Type_struct(count,
                                 blocklens,
                                 displaces,
                                 datatypes,
                                 &combined_datatype);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
   
    ints[0] = count;

    MPID_Datatype_get_ptr(combined_datatype, *combined_dtp);    
    mpi_errno = MPID_Datatype_set_contents(*combined_dtp,
				           MPI_COMBINER_STRUCT,
				           count+1, /* ints (cnt,blklen) */
				           count, /* aints (disps) */
				           count, /* types */
				           ints,
				           displaces,
				           datatypes);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Commit datatype */
    
    MPID_Dataloop_create(combined_datatype,
                         &(*combined_dtp)->dataloop,
                         &(*combined_dtp)->dataloop_size,
                         &(*combined_dtp)->dataloop_depth,
                         MPID_DATALOOP_HOMOGENEOUS);
    
    /* create heterogeneous dataloop */
    MPID_Dataloop_create(combined_datatype,
                         &(*combined_dtp)->hetero_dloop,
                         &(*combined_dtp)->hetero_dloop_size,
                         &(*combined_dtp)->hetero_dloop_depth,
                         MPID_DATALOOP_HETEROGENEOUS);
 
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_DATATYPE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_rma_msg(MPIDI_RMA_Op_t *rma_op, MPID_Win *win_ptr,
                                   MPIDI_CH3_Pkt_flags_t flags,
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info *dtype_info, 
				   void **dataloop, MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &upkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno=MPI_SUCCESS;
    int origin_dt_derived, target_dt_derived, iovcnt;
    MPI_Aint origin_type_size;
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *target_dtp=NULL, *origin_dtp=NULL;
    MPID_Request *resp_req=NULL;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_SEND_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_RMA_MSG);

    *request = NULL;

    if (rma_op->type == MPIDI_RMA_PUT)
    {
        MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
        put_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        put_pkt->flags = flags;
        put_pkt->count = rma_op->target_count;
        put_pkt->datatype = rma_op->target_datatype;
        put_pkt->dataloop_size = 0;
        put_pkt->target_win_handle = target_win_handle;
        put_pkt->source_win_handle = source_win_handle;
        
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) put_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*put_pkt);
    }
    else if (rma_op->type == MPIDI_RMA_GET_ACCUMULATE)
    {
        /* Create a request for the GACC response.  Store the response buf, count, and
           datatype in it, and pass the request's handle in the GACC packet. When the
           response comes from the target, it will contain the request handle. */
        resp_req = MPID_Request_create();
        MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

        MPIU_Object_set_ref(resp_req, 2);

        resp_req->dev.user_buf = rma_op->result_addr;
        resp_req->dev.user_count = rma_op->result_count;
        resp_req->dev.datatype = rma_op->result_datatype;
        resp_req->dev.target_win_handle = target_win_handle;
        resp_req->dev.source_win_handle = source_win_handle;

        if (!MPIR_DATATYPE_IS_PREDEFINED(resp_req->dev.datatype)) {
            MPID_Datatype *result_dtp = NULL;
            MPID_Datatype_get_ptr(resp_req->dev.datatype, result_dtp);
            resp_req->dev.datatype_ptr = result_dtp;
            /* this will cause the datatype to be freed when the
               request is freed. */
        }

        /* Note: Get_accumulate uses the same packet type as accumulate */
        MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_GET_ACCUM);
        accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accum_pkt->flags = flags;
        accum_pkt->count = rma_op->target_count;
        accum_pkt->datatype = rma_op->target_datatype;
        accum_pkt->dataloop_size = 0;
        accum_pkt->op = rma_op->op;
        accum_pkt->target_win_handle = target_win_handle;
        accum_pkt->source_win_handle = source_win_handle;
        accum_pkt->request_handle = resp_req->handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }
    else
    {
        MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
        accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accum_pkt->flags = flags;
        accum_pkt->count = rma_op->target_count;
        accum_pkt->datatype = rma_op->target_datatype;
        accum_pkt->dataloop_size = 0;
        accum_pkt->op = rma_op->op;
        accum_pkt->target_win_handle = target_win_handle;
        accum_pkt->source_win_handle = source_win_handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }

    /*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
          rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
          fflush(stdout);
    */

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype))
    {
        origin_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }
    else
    {
        origin_dt_derived = 0;
    }

    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->target_datatype))
    {
        target_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->target_datatype, target_dtp);
    }
    else
    {
        target_dt_derived = 0;
    }

    if (target_dt_derived)
    {
        /* derived datatype on target. fill derived datatype info */
        dtype_info->is_contig = target_dtp->is_contig;
        dtype_info->max_contig_blocks = target_dtp->max_contig_blocks;
        dtype_info->size = target_dtp->size;
        dtype_info->extent = target_dtp->extent;
        dtype_info->dataloop_size = target_dtp->dataloop_size;
        dtype_info->dataloop_depth = target_dtp->dataloop_depth;
        dtype_info->eltype = target_dtp->eltype;
        dtype_info->dataloop = target_dtp->dataloop;
        dtype_info->ub = target_dtp->ub;
        dtype_info->lb = target_dtp->lb;
        dtype_info->true_ub = target_dtp->true_ub;
        dtype_info->true_lb = target_dtp->true_lb;
        dtype_info->has_sticky_ub = target_dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = target_dtp->has_sticky_lb;

	MPIU_CHKPMEM_MALLOC(*dataloop, void *, target_dtp->dataloop_size, 
			    mpi_errno, "dataloop");

	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        MPIU_Memcpy(*dataloop, target_dtp->dataloop, target_dtp->dataloop_size);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);
        /* the dataloop can have undefined padding sections, so we need to let
         * valgrind know that it is OK to pass this data to writev later on */
        MPL_VG_MAKE_MEM_DEFINED(*dataloop, target_dtp->dataloop_size);

        if (rma_op->type == MPIDI_RMA_PUT)
	{
            put_pkt->dataloop_size = target_dtp->dataloop_size;
	}
        else
	{
            accum_pkt->dataloop_size = target_dtp->dataloop_size;
	}
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!target_dt_derived)
    {
        /* basic datatype on target */
        if (!origin_dt_derived)
        {
            /* basic datatype on origin */
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
            iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
            iovcnt = 2;
	    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
            mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, request);
	    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
        else
        {
            /* derived datatype on origin */
            *request = MPID_Request_create();
            MPIU_ERR_CHKANDJUMP(*request == NULL,mpi_errno,MPI_ERR_OTHER,"**nomemreq");
            
            MPIU_Object_set_ref(*request, 2);
            (*request)->kind = MPID_REQUEST_SEND;
            
            (*request)->dev.segment_ptr = MPID_Segment_alloc( );
            MPIU_ERR_CHKANDJUMP1((*request)->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

            (*request)->dev.datatype_ptr = origin_dtp;
            /* this will cause the datatype to be freed when the request
               is freed. */
            MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                              rma_op->origin_datatype,
                              (*request)->dev.segment_ptr, 0);
            (*request)->dev.segment_first = 0;
            (*request)->dev.segment_size = rma_op->origin_count * origin_type_size;

            (*request)->dev.OnFinal = 0;
            (*request)->dev.OnDataAvail = 0;

	    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
            mpi_errno = vc->sendNoncontig_fn(vc, *request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
	    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else
    {
        /* derived datatype on target */
        MPID_Datatype *combined_dtp = NULL;

        *request = MPID_Request_create();
        if (*request == NULL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomemreq");
        }

        MPIU_Object_set_ref(*request, 2);
        (*request)->kind = MPID_REQUEST_SEND;

	(*request)->dev.segment_ptr = MPID_Segment_alloc( );
        MPIU_ERR_CHKANDJUMP1((*request)->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

        /* create a new datatype containing the dtype_info, dataloop, and origin data */

        mpi_errno = create_datatype(dtype_info, *dataloop, target_dtp->dataloop_size, rma_op->origin_addr,
                                    rma_op->origin_count, rma_op->origin_datatype, &combined_dtp);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        (*request)->dev.datatype_ptr = combined_dtp;
        /* combined_datatype will be freed when request is freed */

        MPID_Segment_init(MPI_BOTTOM, 1, combined_dtp->handle,
                          (*request)->dev.segment_ptr, 0);
        (*request)->dev.segment_first = 0;
        (*request)->dev.segment_size = combined_dtp->size;

        (*request)->dev.OnFinal = 0;
        (*request)->dev.OnDataAvail = 0;

	MPIU_THREAD_CS_ENTER(CH3COMM,vc);
        mpi_errno = vc->sendNoncontig_fn(vc, *request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
	MPIU_THREAD_CS_EXIT(CH3COMM,vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        /* we're done with the datatypes */
        if (origin_dt_derived)
            MPID_Datatype_release(origin_dtp);
        MPID_Datatype_release(target_dtp);
    }

    /* This operation can generate two requests; one for inbound and one for
       outbound data. */
    if (resp_req != NULL) {
        if (*request != NULL) {
            /* If we have both inbound and outbound requests (i.e. GACC
               operation), we need to ensure that the source buffer is
               available and that the response data has been received before
               informing the origin that this operation is complete.  Because
               the update needs to be done atomically at the target, they will
               not send back data until it has been received.  Therefore,
               completion of the response request implies that the send request
               has completed.

               Therefore: refs on the response request are set to two: one is
               held by the progress engine and the other by the RMA op
               completion code.  Refs on the outbound request are set to one;
               it will be completed by the progress engine.
             */

            MPID_Request_release(*request);
            *request = resp_req;

        } else {
            *request = resp_req;
        }

        /* For error checking */
        resp_req = NULL;
    }

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_RMA_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    if (resp_req) {
        MPID_Request_release(resp_req);
    }
    if (*request)
    {
        MPIU_CHKPMEM_REAP();
        if ((*request)->dev.datatype_ptr)
            MPID_Datatype_release((*request)->dev.datatype_ptr);
        MPID_Request_release(*request);
    }
    *request = NULL;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*
 * Use this for contiguous accumulate operations
 */
#undef FUNCNAME
#define FUNCNAME send_contig_acc_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_contig_acc_msg(MPIDI_RMA_Op_t *rma_op,
					  MPID_Win *win_ptr,
                                          MPIDI_CH3_Pkt_flags_t flags,
					  MPI_Win source_win_handle, 
					  MPI_Win target_win_handle, 
					  MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &upkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno=MPI_SUCCESS;
    int iovcnt;
    MPI_Aint origin_type_size;
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    size_t len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_CONTIG_ACC_MSG);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_CONTIG_ACC_MSG);

    *request = NULL;

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);
    /* FIXME: Make this size check efficient and match the packet type */
    MPIU_Assign_trunc(len, rma_op->origin_count * origin_type_size, size_t);
    if (MPIR_CVAR_CH3_RMA_ACC_IMMED && len <= MPIDI_RMA_IMMED_INTS*sizeof(int)) {
	MPIDI_CH3_Pkt_accum_immed_t * accumi_pkt = &upkt.accum_immed;
	void *dest = accumi_pkt->data, *src = rma_op->origin_addr;
	
	MPIDI_Pkt_init(accumi_pkt, MPIDI_CH3_PKT_ACCUM_IMMED);
	accumi_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
	    win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accumi_pkt->flags = flags;
	accumi_pkt->count = rma_op->target_count;
	accumi_pkt->datatype = rma_op->target_datatype;
	accumi_pkt->op = rma_op->op;
	accumi_pkt->target_win_handle = target_win_handle;
	accumi_pkt->source_win_handle = source_win_handle;
	
	switch (len) {
	case 1: *(uint8_t *)dest  = *(uint8_t *)src;  break;
	case 2: *(uint16_t *)dest = *(uint16_t *)src; break;
	case 4: *(uint32_t *)dest = *(uint32_t *)src; break;
	case 8: *(uint64_t *)dest = *(uint64_t *)src; break;
	default:
	    MPIU_Memcpy( accumi_pkt->data, (void *)rma_op->origin_addr, len );
	}
	comm_ptr = win_ptr->comm_ptr;
	MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
	MPIU_THREAD_CS_ENTER(CH3COMM,vc);
	mpi_errno = MPIDI_CH3_iStartMsg(vc, accumi_pkt, sizeof(*accumi_pkt), request);
	MPIU_THREAD_CS_EXIT(CH3COMM,vc);
	MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
	goto fn_exit;
    }

    MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
    accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
	win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
    accum_pkt->flags = flags;
    accum_pkt->count = rma_op->target_count;
    accum_pkt->datatype = rma_op->target_datatype;
    accum_pkt->dataloop_size = 0;
    accum_pkt->op = rma_op->op;
    accum_pkt->target_win_handle = target_win_handle;
    accum_pkt->source_win_handle = source_win_handle;
    
    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) accum_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);

    /*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
          rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
          fflush(stdout);
    */

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);


    /* basic datatype on target */
    /* basic datatype on origin */
    /* FIXME: This is still very heavyweight for a small message operation,
       such as a single word update */
    /* One possibility is to use iStartMsg with a buffer that is just large 
       enough, though note that nemesis has an optimization for this */
    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
    iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
    iovcnt = 2;
    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, request);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_CONTIG_ACC_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    if (*request)
    {
        MPID_Request_release(*request);
    }
    *request = NULL;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/*
 * Initiate an immediate RMW accumulate operation
 */
#undef FUNCNAME
#define FUNCNAME send_immed_rmw_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_immed_rmw_msg(MPIDI_RMA_Op_t *rma_op,
                                         MPID_Win *win_ptr,
                                         MPIDI_CH3_Pkt_flags_t flags,
                                         MPI_Win source_win_handle, 
                                         MPI_Win target_win_handle, 
                                         MPID_Request **request) 
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *rmw_req = NULL, *resp_req = NULL;
    MPIDI_VC_t *vc;
    MPID_Comm *comm_ptr;
    MPI_Aint len;
    MPIDI_STATE_DECL(MPID_STATE_SEND_IMMED_RMW_MSG);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_IMMED_RMW_MSG);

    *request = NULL;

    /* Create a request for the RMW response.  Store the origin buf, count, and
       datatype in it, and pass the request's handle RMW packet. When the
       response comes from the target, it will contain the request handle. */
    resp_req = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    *request = resp_req;

    /* Set refs on the request to 2: one for the response message, and one for
       the partial completion handler */
    MPIU_Object_set_ref(resp_req, 2);

    resp_req->dev.user_buf = rma_op->result_addr;
    resp_req->dev.user_count = rma_op->result_count;
    resp_req->dev.datatype = rma_op->result_datatype;
    resp_req->dev.target_win_handle = target_win_handle;
    resp_req->dev.source_win_handle = source_win_handle;

    /* REQUIRE: All datatype arguments must be of the same, builtin
                type and counts must be 1. */
    MPID_Datatype_get_size_macro(rma_op->origin_datatype, len);
    comm_ptr = win_ptr->comm_ptr;

    if (rma_op->type == MPIDI_RMA_COMPARE_AND_SWAP) {
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_cas_t *cas_pkt = &upkt.cas;

        MPIU_Assert(len <= sizeof(MPIDI_CH3_CAS_Immed_u));

        MPIDI_Pkt_init(cas_pkt, MPIDI_CH3_PKT_CAS);

        cas_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        cas_pkt->flags = flags;
        cas_pkt->datatype = rma_op->target_datatype;
        cas_pkt->target_win_handle = target_win_handle;
        cas_pkt->request_handle = resp_req->handle;

        MPIU_Memcpy( (void *) &cas_pkt->origin_data, rma_op->origin_addr, len );
        MPIU_Memcpy( (void *) &cas_pkt->compare_data, rma_op->compare_addr, len );

        MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
        MPIU_THREAD_CS_ENTER(CH3COMM,vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_pkt, sizeof(*cas_pkt), &rmw_req);
        MPIU_THREAD_CS_EXIT(CH3COMM,vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (rmw_req != NULL) {
            MPID_Request_release(rmw_req);
        }
    }

    else if (rma_op->type == MPIDI_RMA_FETCH_AND_OP) {
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_fop_t *fop_pkt = &upkt.fop;

        MPIU_Assert(len <= sizeof(MPIDI_CH3_FOP_Immed_u));

        MPIDI_Pkt_init(fop_pkt, MPIDI_CH3_PKT_FOP);

        fop_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        fop_pkt->flags = flags;
        fop_pkt->datatype = rma_op->target_datatype;
        fop_pkt->target_win_handle = target_win_handle;
        fop_pkt->request_handle = resp_req->handle;
        fop_pkt->op = rma_op->op;

        if (len <= sizeof(fop_pkt->origin_data) || rma_op->op == MPI_NO_OP) {
            /* Embed FOP data in the packet header */
            if (rma_op->op != MPI_NO_OP) {
                MPIU_Memcpy( fop_pkt->origin_data, rma_op->origin_addr, len );
            }

            MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
            MPIU_THREAD_CS_ENTER(CH3COMM,vc);
            mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_pkt, sizeof(*fop_pkt), &rmw_req);
            MPIU_THREAD_CS_EXIT(CH3COMM,vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

            if (rmw_req != NULL) {
                MPID_Request_release(rmw_req);
            }
        }
        else {
            /* Data is too big to copy into the FOP header, use an IOV to send it */
            MPID_IOV iov[MPID_IOV_LIMIT];

            rmw_req = MPID_Request_create();
            MPIU_ERR_CHKANDJUMP(rmw_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
            MPIU_Object_set_ref(rmw_req, 1);

            rmw_req->dev.OnFinal = 0;
            rmw_req->dev.OnDataAvail = 0;

            iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)fop_pkt;
            iov[0].MPID_IOV_LEN = sizeof(*fop_pkt);
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
            iov[1].MPID_IOV_LEN = len; /* count == 1 */

            MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);
            MPIU_THREAD_CS_ENTER(CH3COMM,vc);
            mpi_errno = MPIDI_CH3_iSendv(vc, rmw_req, iov, 2);
            MPIU_THREAD_CS_EXIT(CH3COMM,vc);

            MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_IMMED_RMW_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    if (*request) {
        MPID_Request_release(*request);
    }
    *request = NULL;
    if (rmw_req) {
        MPID_Request_release(rmw_req);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME recv_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int recv_rma_msg(MPIDI_RMA_Op_t *rma_op, MPID_Win *win_ptr,
                                   MPIDI_CH3_Pkt_flags_t flags,
				   MPI_Win source_win_handle, 
				   MPI_Win target_win_handle, 
				   MPIDI_RMA_dtype_info *dtype_info, 
				   void **dataloop, MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_t *get_pkt = &upkt.get;
    int mpi_errno=MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    MPID_Request *req = NULL;
    MPID_Datatype *dtp;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_RECV_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_RECV_RMA_MSG);

    /* create a request, store the origin buf, cnt, datatype in it,
       and pass a handle to it in the get packet. When the get
       response comes from the target, it will contain the request
       handle. */  
    req = MPID_Request_create();
    if (req == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomemreq");
    }

    *request = req;

    MPIU_Object_set_ref(req, 2);

    req->dev.user_buf = rma_op->origin_addr;
    req->dev.user_count = rma_op->origin_count;
    req->dev.datatype = rma_op->origin_datatype;
    req->dev.target_win_handle = MPI_WIN_NULL;
    req->dev.source_win_handle = source_win_handle;
    if (!MPIR_DATATYPE_IS_PREDEFINED(req->dev.datatype))
    {
        MPID_Datatype_get_ptr(req->dev.datatype, dtp);
        req->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
           request is freed. */  
    }

    MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
    get_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
    get_pkt->flags = flags;
    get_pkt->count = rma_op->target_count;
    get_pkt->datatype = rma_op->target_datatype;
    get_pkt->request_handle = req->handle;
    get_pkt->target_win_handle = target_win_handle;
    get_pkt->source_win_handle = source_win_handle;

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/
	    
    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (MPIR_DATATYPE_IS_PREDEFINED(rma_op->target_datatype))
    {
        /* basic datatype on target. simply send the get_pkt. */
	MPIU_THREAD_CS_ENTER(CH3COMM,vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req);
	MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    }
    else
    {
        /* derived datatype on target. fill derived datatype info and
           send it along with get_pkt. */

        MPID_Datatype_get_ptr(rma_op->target_datatype, dtp);
        dtype_info->is_contig = dtp->is_contig;
        dtype_info->max_contig_blocks = dtp->max_contig_blocks;
        dtype_info->size = dtp->size;
        dtype_info->extent = dtp->extent;
        dtype_info->dataloop_size = dtp->dataloop_size;
        dtype_info->dataloop_depth = dtp->dataloop_depth;
        dtype_info->eltype = dtp->eltype;
        dtype_info->dataloop = dtp->dataloop;
        dtype_info->ub = dtp->ub;
        dtype_info->lb = dtp->lb;
        dtype_info->true_ub = dtp->true_ub;
        dtype_info->true_lb = dtp->true_lb;
        dtype_info->has_sticky_ub = dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = dtp->has_sticky_lb;

	MPIU_CHKPMEM_MALLOC(*dataloop, void *, dtp->dataloop_size, 
			    mpi_errno, "dataloop");

	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        MPIU_Memcpy(*dataloop, dtp->dataloop, dtp->dataloop_size);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        /* the dataloop can have undefined padding sections, so we need to let
         * valgrind know that it is OK to pass this data to writev later on */
        MPL_VG_MAKE_MEM_DEFINED(*dataloop, dtp->dataloop_size);

        get_pkt->dataloop_size = dtp->dataloop_size;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)get_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)dtype_info;
        iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
        iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)*dataloop;
        iov[2].MPID_IOV_LEN = dtp->dataloop_size;

	MPIU_THREAD_CS_ENTER(CH3COMM,vc);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 3, &req);
	MPIU_THREAD_CS_EXIT(CH3COMM,vc);

        /* release the target datatype */
        MPID_Datatype_release(dtp);
    }

    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
    }

    /* release the request returned by iStartMsg or iStartMsgv */
    if (req != NULL)
    {
        MPID_Request_release(req);
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_RECV_RMA_MSG);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_post(MPID_Group *post_grp_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
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

    /* In case this process was previously the target of passive target rma
     * operations, we need to take care of the following...
     * Since we allow MPI_Win_unlock to return without a done ack from
     * the target in the case of multiple rma ops and exclusive lock,
     * we need to check whether there is a lock on the window, and if
     * there is a lock, poke the progress engine until the operations
     * have completed and the lock is therefore released. */
    if (win_ptr->current_lock_type != MPID_LOCK_NONE)
    {
	MPID_Progress_state progress_state;
	
	MPIR_T_PVAR_TIMER_START(RMA, rma_winpost_clearlock);
	/* poke the progress engine */
	MPID_Progress_start(&progress_state);
	while (win_ptr->current_lock_type != MPID_LOCK_NONE)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS) {
		MPID_Progress_end(&progress_state);
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
	    }
	    /* --END ERROR HANDLING-- */
	    MPIR_T_PVAR_COUNTER_INC(RMA, rma_winpost_clearlock_aux, 1);
	}
	MPID_Progress_end(&progress_state);
	MPIR_T_PVAR_TIMER_END(RMA, rma_winpost_clearlock);
    }
        
    post_grp_size = post_grp_ptr->size;

    /* Ensure ordering of load/store operations. */
    if (win_ptr->shm_allocated == TRUE) {
        OPA_read_write_barrier();
    }
        
    /* initialize the completion counter */
    win_ptr->at_completion_counter += post_grp_size;
        
    if ((assert & MPI_MODE_NOCHECK) == 0)
    {
        MPI_Request *req;
        MPI_Status *status;

	MPIR_T_PVAR_TIMER_START(RMA, rma_winpost_sendsync);
 
	/* NOCHECK not specified. We need to notify the source
	   processes that Post has been called. */  
	
	/* We need to translate the ranks of the processes in
	   post_group to ranks in win_ptr->comm_ptr, so that we
	   can do communication */
            
	MPIU_CHKLMEM_MALLOC(ranks_in_post_grp, int *, 
			    post_grp_size * sizeof(int),
			    mpi_errno, "ranks_in_post_grp");
	MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *, 
			    post_grp_size * sizeof(int),
			    mpi_errno, "ranks_in_win_grp");
        
	for (i=0; i<post_grp_size; i++)
	{
	    ranks_in_post_grp[i] = i;
	}
        
	win_comm_ptr = win_ptr->comm_ptr;

        mpi_errno = MPIR_Comm_group_impl(win_comm_ptr, &win_grp_ptr);
	if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	

        mpi_errno = MPIR_Group_translate_ranks_impl(post_grp_ptr, post_grp_size, ranks_in_post_grp,
                                                    win_grp_ptr, ranks_in_win_grp);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	
        rank = win_ptr->comm_ptr->rank;
	
	MPIU_CHKLMEM_MALLOC(req, MPI_Request *, post_grp_size * sizeof(MPI_Request), mpi_errno, "req");
        MPIU_CHKLMEM_MALLOC(status, MPI_Status *, post_grp_size*sizeof(MPI_Status), mpi_errno, "status");

	/* Send a 0-byte message to the source processes */
	MPIR_T_PVAR_COUNTER_INC(RMA, rma_winpost_sendsync_aux, post_grp_size);
	for (i = 0; i < post_grp_size; i++) {
	    dst = ranks_in_win_grp[i];

	    /* FIXME: Short messages like this shouldn't normally need a 
	       request - this should consider using the ch3 call to send
	       a short message and return a request only if the message is
	       not delivered. */
	    if (dst != rank) {
                MPID_Request *req_ptr;
		mpi_errno = MPID_Isend(&i, 0, MPI_INT, dst, SYNC_POST_TAG, win_comm_ptr,
                                       MPID_CONTEXT_INTRA_PT2PT, &req_ptr);
		if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                req[i] = req_ptr->handle;
	    } else {
                req[i] = MPI_REQUEST_NULL;
            }
	}
        mpi_errno = MPIR_Waitall_impl(post_grp_size, req, status);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS) MPIU_ERR_POP(mpi_errno);

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
	if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	MPIR_T_PVAR_TIMER_END(RMA, rma_winpost_sendsync);
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


static int recv_post_msgs(MPID_Win *win_ptr, int *ranks_in_win_grp, int local)
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
    MPIU_CHKLMEM_MALLOC(req, MPI_Request *, start_grp_size*sizeof(MPI_Request), mpi_errno, "req");
    MPIU_CHKLMEM_MALLOC(status, MPI_Status *, start_grp_size*sizeof(MPI_Status), mpi_errno, "status");

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
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
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
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS) MPIU_ERR_POP(mpi_errno);
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

static int fill_ranks_in_win_grp(MPID_Win *win_ptr, int *ranks_in_win_grp)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ranks_in_start_grp;
    MPID_Group *win_grp_ptr;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIU_CHKLMEM_MALLOC(ranks_in_start_grp, int *, win_ptr->start_group_ptr->size*sizeof(int),
			mpi_errno, "ranks_in_start_grp");

    for (i = 0; i < win_ptr->start_group_ptr->size; i++)
	ranks_in_start_grp[i] = i;

    mpi_errno = MPIR_Comm_group_impl(win_ptr->comm_ptr, &win_grp_ptr);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIR_Group_translate_ranks_impl(win_ptr->start_group_ptr, win_ptr->start_group_ptr->size,
                                                ranks_in_start_grp,
                                                win_grp_ptr, ranks_in_win_grp);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_fail:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_FILL_RANKS_IN_WIN_GRP);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Win_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
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

    /* In case this process was previously the target of passive target rma
     * operations, we need to take care of the following...
     * Since we allow MPI_Win_unlock to return without a done ack from
     * the target in the case of multiple rma ops and exclusive lock,
     * we need to check whether there is a lock on the window, and if
     * there is a lock, poke the progress engine until the operations
     * have completed and the lock is therefore released. */
    if (win_ptr->current_lock_type != MPID_LOCK_NONE)
    {
	MPID_Progress_state progress_state;
	
	MPIR_T_PVAR_TIMER_START(RMA, rma_winstart_clearlock);
	/* poke the progress engine */
	MPID_Progress_start(&progress_state);
	while (win_ptr->current_lock_type != MPID_LOCK_NONE)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS) {
		MPID_Progress_end(&progress_state);
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
	    }
	    /* --END ERROR HANDLING-- */
	    MPIR_T_PVAR_COUNTER_INC(RMA, rma_winstart_clearlock_aux, 1);
	}
	MPID_Progress_end(&progress_state);
	MPIR_T_PVAR_TIMER_END(RMA, rma_winstart_clearlock);
    }
    
    win_ptr->start_group_ptr = group_ptr;
    MPIR_Group_add_ref( group_ptr );
    win_ptr->start_assert = assert;

    /* wait for messages from local processes */
    MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *, win_ptr->start_group_ptr->size*sizeof(int),
			mpi_errno, "ranks_in_win_grp");

    mpi_errno = fill_ranks_in_win_grp(win_ptr, ranks_in_win_grp);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

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
int MPIDI_Win_complete(MPID_Win *win_ptr)
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
       start_group to ranks in win_ptr->comm_ptr */
    
    start_grp_size = win_ptr->start_group_ptr->size;

    MPIR_T_PVAR_TIMER_START(RMA, rma_wincomplete_recvsync);

    MPIU_CHKLMEM_MALLOC(ranks_in_win_grp, int *, start_grp_size*sizeof(int), 
			mpi_errno, "ranks_in_win_grp");
        
    mpi_errno = fill_ranks_in_win_grp(win_ptr, ranks_in_win_grp);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    rank = win_ptr->comm_ptr->rank;

    /* If MPI_MODE_NOCHECK was not specified, we need to check if
       Win_post was called on the target processes. Wait for a 0-byte sync
       message from each target process */
    if ((win_ptr->start_assert & MPI_MODE_NOCHECK) == 0)
    {
        /* wait for messages from non-local processes */
        mpi_errno = recv_post_msgs(win_ptr, ranks_in_win_grp, 0);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincomplete_recvsync);

    /* keep track of no. of ops to each proc. Needed for knowing
       whether or not to decrement the completion counter. The
       completion counter is decremented only on the last
       operation. */

    MPIR_T_PVAR_TIMER_START(RMA, rma_wincomplete_issue);

    /* Note, active target uses the following ops list, and passive
       target uses win_ptr->targets[..] */
    ops_list = &win_ptr->at_rma_ops_list;

    MPIU_CHKLMEM_MALLOC(nops_to_proc, int *, comm_size*sizeof(int), 
			mpi_errno, "nops_to_proc");
    for (i=0; i<comm_size; i++) nops_to_proc[i] = 0;

    total_op_count = 0;
    curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
    while (curr_ptr != NULL)
    {
	nops_to_proc[curr_ptr->target_rank]++;
	total_op_count++;
	curr_ptr = curr_ptr->next;
    }

    MPIR_T_PVAR_COUNTER_INC(RMA, rma_wincomplete_issue_aux, total_op_count);
    MPIR_T_PVAR_ULONG2_HIGHWATERMARK_UPDATE(RMA, rma_wincomplete_issue_aux, total_op_count);

    /* We allocate a few extra requests because if there are no RMA
       ops to a target process, we need to send a 0-byte message just
       to decrement the completion counter. */
        
    MPIU_CHKLMEM_MALLOC(curr_ops_cnt, int *, comm_size*sizeof(int),
			mpi_errno, "curr_ops_cnt");
    for (i=0; i<comm_size; i++) curr_ops_cnt[i] = 0;
        
    i = 0;
    curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);
    while (curr_ptr != NULL)
    {
        MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;

	/* The completion counter at the target is decremented only on 
	   the last RMA operation. */
	if (curr_ops_cnt[curr_ptr->target_rank] ==
	    nops_to_proc[curr_ptr->target_rank] - 1) {
            flags = MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE;
        }

        source_win_handle = win_ptr->handle;
	target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];

#define MPIDI_CH3I_TRACK_RMA_WRITE(op_ptr_, win_ptr_) /* Not used by active mode */
            MPIDI_CH3I_ISSUE_RMA_OP(curr_ptr, win_ptr, flags,
                                    source_win_handle, target_win_handle, mpi_errno);
#undef MPIDI_CH3I_TRACK_RMA_WRITE

	i++;
	curr_ops_cnt[curr_ptr->target_rank]++;
	/* If the request is null, we can remove it immediately */
	if (!curr_ptr->request) {
            MPIDI_CH3I_RMA_Ops_free_and_next(ops_list, &curr_ptr);
	}
	else  {
	    nRequest++;
	    MPIR_T_PVAR_COUNTER_INC(RMA, rma_wincomplete_reqs, 1);
	    curr_ptr    = curr_ptr->next;
	    if (nRequest > MPIR_CVAR_CH3_RMA_NREQUEST_THRESHOLD &&
		nRequest - nRequestNew > MPIR_CVAR_CH3_RMA_NREQUEST_NEW_THRESHOLD) {
		int nDone = 0;
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIR_T_PVAR_STMT(RMA, list_complete_timer=MPIR_T_PVAR_TIMER_ADDR(rma_wincomplete_complete));
            MPIR_T_PVAR_STMT(RMA, list_complete_counter=MPIR_T_PVAR_COUNTER_ADDR(rma_wincomplete_complete_aux));
                mpi_errno = rma_list_gc(win_ptr, ops_list, curr_ptr, &nDone);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		nRequest -= nDone;
		nRequestNew = nRequest;
	    }
	}
    }
    MPIR_T_PVAR_TIMER_END(RMA, rma_wincomplete_issue);
        
    /* If the start_group included some processes that did not end up
       becoming targets of  RMA operations from this process, we need
       to send a dummy message to those processes just to decrement
       the completion counter */
        
    j = i;
    new_total_op_count = total_op_count;
    for (i=0; i<start_grp_size; i++)
    {
	dst = ranks_in_win_grp[i];
	if (dst == rank) {
	    /* FIXME: MT: this has to be done atomically */
	    win_ptr->at_completion_counter -= 1;
	}
	else if (nops_to_proc[dst] == 0)
	{
	    MPIDI_CH3_Pkt_t upkt;
	    MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
	    MPIDI_VC_t * vc;
	    MPID_Request *request;
	    
	    MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
            put_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE;
	    put_pkt->addr = NULL;
	    put_pkt->count = 0;
	    put_pkt->datatype = MPI_INT;
	    put_pkt->target_win_handle = win_ptr->all_win_handles[dst];
	    put_pkt->source_win_handle = win_ptr->handle;
	    
	    MPIDI_Comm_get_vc_set_active(comm_ptr, dst, &vc);
	    
	    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
	    mpi_errno = MPIDI_CH3_iStartMsg(vc, put_pkt, sizeof(*put_pkt), &request);
	    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg" );
	    }
	    /* In the unlikely event that a request is returned (the message
	       is not sent yet), add it to the list of pending operations */
	    if (request) {
                MPIDI_RMA_Op_t *new_ptr = NULL;

                MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
                mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
                MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

		MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);
		new_ptr->request  = request;
		MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);
	    }
	    j++;
	    new_total_op_count++;
	}
    }

    if (new_total_op_count)
    {
        MPIR_T_PVAR_STMT(RMA, list_complete_timer=MPIR_T_PVAR_TIMER_ADDR(rma_wincomplete_complete));
        MPIR_T_PVAR_STMT(RMA, list_complete_counter=MPIR_T_PVAR_COUNTER_ADDR(rma_wincomplete_complete_aux));
        MPIR_T_PVAR_STMT(RMA, list_block_timer=MPIR_T_PVAR_TIMER_ADDR(rma_wincomplete_block));
        mpi_errno = rma_list_complete(win_ptr, ops_list);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
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
int MPIDI_Win_wait(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

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
    if (win_ptr->at_completion_counter)
    {
	MPID_Progress_state progress_state;
	
	MPIR_T_PVAR_TIMER_START(RMA, rma_winwait_wait);
	MPID_Progress_start(&progress_state);
	while (win_ptr->at_completion_counter)
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		MPID_Progress_end(&progress_state);
		MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_WIN_WAIT);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
	    MPIR_T_PVAR_COUNTER_INC(RMA, rma_winwait_wait_aux, 1);
	}
	MPID_Progress_end(&progress_state);
	MPIR_T_PVAR_TIMER_END(RMA, rma_winwait_wait);
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
int MPIDI_Win_test(MPID_Win *win_ptr, int *flag)
{
    int mpi_errno=MPI_SUCCESS;

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
int MPIDI_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr)
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

    if (dest == MPI_PROC_NULL) goto fn_exit;

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

    target_state->remote_lock_state  = MPIDI_CH3_WIN_LOCK_CALLED;
    target_state->remote_lock_mode   = lock_type;
    target_state->remote_lock_assert = assert;

    if (dest == win_ptr->comm_ptr->rank) {
        /* The target is this process itself. We must block until the lock
         * is acquired.  Once it is acquired, local puts, gets, accumulates
         * will be done directly without queueing. */
        mpi_errno = acquire_local_lock(win_ptr, lock_type);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }
    else if (win_ptr->shm_allocated == TRUE) {
        /* Lock must be taken immediately for shared memory windows because of
         * load/store access */

        if (win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
            /* check if target is local and shared memory is allocated on window,
               if so, we directly send lock request and wait for lock reply. */

            /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
               the same node. However, in ch3:sock, even if origin and target are on the same node, they do
               not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
               which is only set to TRUE when SHM region is allocated in nemesis.
               In future we need to figure out a way to check if origin and target are in the same "SHM comm".
            */
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
            MPIDI_Comm_get_vc(win_ptr->comm_ptr, dest, &target_vc);
        }

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED || orig_vc->node_id == target_vc->node_id) {
            mpi_errno = send_lock_msg(dest, lock_type, win_ptr);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

            mpi_errno = wait_for_lock_granted(win_ptr, dest);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }
    }
    else if (MPIR_CVAR_CH3_RMA_LOCK_IMMED && ((assert & MPI_MODE_NOCHECK) == 0)) {
        /* TODO: Make this mode of operation available through an assert
           argument or info key. */
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
int MPIDI_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    int single_op_opt = 0;
    MPIDI_RMA_Op_t *rma_op;
    int wait_for_rma_done_pkt = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_WIN_UNLOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_WIN_UNLOCK);

    if (dest == MPI_PROC_NULL) goto fn_exit;

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
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
        win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;

	mpi_errno = MPID_Progress_poke();
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
	goto fn_exit;
    }
        
    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[dest].rma_ops_list);

    /* Lock was called, but the lock was not requested and there are no ops to
     * perform.  Do nothing and return. */
    if (rma_op == NULL &&
        win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED)
    {
        win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;
        goto fn_exit;
    }

    /* TODO: MPI-3: Add lock->cas/fop/gacc->unlock optimization.  */
    /* TODO: MPI-3: Add lock_all->op optimization. */
    /* LOCK-OP-UNLOCK Optimization -- This optimization can't be used if we
       have already requested the lock. */
    if ( MPIR_CVAR_CH3_RMA_MERGE_LOCK_OP_UNLOCK &&
         win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED &&
         rma_op && rma_op->next == NULL /* There is only one op */ &&
         rma_op->type != MPIDI_RMA_COMPARE_AND_SWAP &&
         rma_op->type != MPIDI_RMA_FETCH_AND_OP &&
         rma_op->type != MPIDI_RMA_GET_ACCUMULATE )
    {
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
	     (type_size * curr_op->origin_count <= vc->eager_max_msg_sz) ) {
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
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	}
    }
        
    if (single_op_opt == 0) {

        /* Send a lock packet over to the target and wait for the lock_granted
           reply. If the user gave MODE_NOCHECK, we will piggyback the lock
           request on the first RMA op.  Then do all the RMA ops. */

        if ((win_ptr->targets[dest].remote_lock_assert & MPI_MODE_NOCHECK) == 0)
        {
            if (win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED) {
                mpi_errno = send_lock_msg(dest, win_ptr->targets[dest].remote_lock_mode,
                                                     win_ptr);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            }
        }

        if (win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_REQUESTED) {
            mpi_errno = wait_for_lock_granted(win_ptr, dest);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }

	/* Now do all the RMA operations */
        mpi_errno = do_passive_target_rma(win_ptr, dest, &wait_for_rma_done_pkt,
                                                     MPIDI_CH3_PKT_FLAG_RMA_UNLOCK);
	if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }
        
    /* If the lock is a shared lock or we have done the single op
       optimization, we need to wait until the target informs us that
       all operations are done on the target.  This ensures that third-
       party communication can be done safely.  */
    if (wait_for_rma_done_pkt == 1) {
        /* wait until the "pt rma done" packet is received from the 
           target. This packet resets the remote_lock_state flag back to
           NONE. */

        /* poke the progress engine until remote_lock_state flag is reset to NONE */
        if (win_ptr->targets[dest].remote_lock_state != MPIDI_CH3_WIN_LOCK_NONE)
	{
	    MPID_Progress_state progress_state;
	    
	    MPID_Progress_start(&progress_state);
            while (win_ptr->targets[dest].remote_lock_state != MPIDI_CH3_WIN_LOCK_NONE)
	    {
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
int MPIDI_Win_flush_all(MPID_Win *win_ptr)
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
            if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
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
int MPIDI_Win_flush(int rank, MPID_Win *win_ptr)
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
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* NOTE: All flush and req-based operations are currently implemented in
       terms of MPIDI_Win_flush.  When this changes, those operations will also
       need to insert this read/write memory fence for shared memory windows. */

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[rank].rma_ops_list);

    /* If there is no activity at this target (e.g. lock-all was called, but we
     * haven't communicated with this target), don't do anything. */
    if (win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED
        && rma_op == NULL)
    {
        goto fn_exit;
    }

    /* MT: If another thread is performing a flush, wait for them to finish. */
    if (win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_FLUSH)
    {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED)
        {
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
       reply, and perform the RMA ops. */

    if (win_ptr->targets[rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED) {
        mpi_errno = send_lock_msg(rank, win_ptr->targets[rank].remote_lock_mode, win_ptr);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }

    if (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED) {
        mpi_errno = wait_for_lock_granted(win_ptr, rank);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }

    win_ptr->targets[rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_FLUSH;
    mpi_errno = do_passive_target_rma(win_ptr, rank, &wait_for_rma_done_pkt,
                                                 MPIDI_CH3_PKT_FLAG_RMA_FLUSH);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* If the lock is a shared lock or we have done the single op optimization,
       we need to wait until the target informs us that all operations are done
       on the target.  This ensures that third-party communication can be done
       safely.  */
    if (wait_for_rma_done_pkt == 1) {
        /* wait until the "pt rma done" packet is received from the target.
           This packet resets the remote_lock_state flag. */

        if (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED)
        {
            MPID_Progress_state progress_state;

            MPID_Progress_start(&progress_state);
            while (win_ptr->targets[rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_GRANTED)
            {
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
int MPIDI_Win_flush_local(int rank, MPID_Win *win_ptr)
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
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

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
int MPIDI_Win_flush_local_all(MPID_Win *win_ptr)
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
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

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
int MPIDI_Win_lock_all(int assert, MPID_Win *win_ptr)
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

        win_ptr->targets[i].remote_lock_state  = MPIDI_CH3_WIN_LOCK_CALLED;
        win_ptr->targets[i].remote_lock_mode   = MPI_LOCK_SHARED;
        win_ptr->targets[i].remote_lock_assert = assert;
    }

    /* Immediately lock the local process for load/store access */
    mpi_errno = acquire_local_lock(win_ptr, MPI_LOCK_SHARED);
    if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

    if (win_ptr->shm_allocated == TRUE) {
        /* Immediately lock all targets for load/store access */

        for (i = 0; i < MPIR_Comm_size(win_ptr->comm_ptr); i++) {
            /* Local process is already locked */
            if (i == win_ptr->comm_ptr->rank) continue;

            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
                /* check if target is local and shared memory is allocated on window,
                   if so, we directly send lock request and wait for lock reply. */

                /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
                   the same node. However, in ch3:sock, even if origin and target are on the same node, they do
                   not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
                   which is only set to TRUE when SHM region is allocated in nemesis.
                   In future we need to figure out a way to check if origin and target are in the same "SHM comm".
                */
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
            }

            if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED || orig_vc->node_id == target_vc->node_id) {
                mpi_errno = send_lock_msg(i, MPI_LOCK_SHARED, win_ptr);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            }
        }

        for (i = 0; i < MPIR_Comm_size(win_ptr->comm_ptr); i++) {
            /* Local process is already locked */
            if (i == win_ptr->comm_ptr->rank) continue;

            if (win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
                MPIDI_Comm_get_vc(win_ptr->comm_ptr, i, &target_vc);
            }

            if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED || orig_vc->node_id == target_vc->node_id) {
                mpi_errno = wait_for_lock_granted(win_ptr, i);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
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
int MPIDI_Win_unlock_all(MPID_Win *win_ptr)
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
        if (mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
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
#define FUNCNAME MPIDI_Win_sync
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Win_sync(MPID_Win *win_ptr)
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
#define FUNCNAME do_passive_target_rma
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_passive_target_rma(MPID_Win *win_ptr, int target_rank,
                                            int *wait_for_rma_done_pkt, MPIDI_CH3_Pkt_flags_t sync_flags)
{
    int mpi_errno = MPI_SUCCESS, nops;
    MPIDI_RMA_Op_t *curr_ptr;
    MPI_Win source_win_handle = MPI_WIN_NULL, target_win_handle = MPI_WIN_NULL;
    int nRequest=0, nRequestNew=0;
    MPIDI_STATE_DECL(MPID_STATE_DO_PASSIVE_TARGET_RMA);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_DO_PASSIVE_TARGET_RMA);

    MPIU_Assert(win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_GRANTED ||
                win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_FLUSH ||
                (win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED &&
                 win_ptr->targets[target_rank].remote_lock_assert & MPI_MODE_NOCHECK));

    if (win_ptr->targets[target_rank].remote_lock_mode == MPI_LOCK_EXCLUSIVE &&
        win_ptr->targets[target_rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_CALLED &&
        win_ptr->targets[target_rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_FLUSH) {
        /* Exclusive lock already held -- no need to wait for rma done pkt at
           the end.  This is because the target won't grant another process
           access to the window until all of our operations complete at that
           target.  Thus, there is no third-party communication issue.
           However, flush still needs to wait for rma done, otherwise result
           may be unknown if user reads the updated location from a shared window of
           another target process after this flush. */
        *wait_for_rma_done_pkt = 0;
    }
    else if (MPIDI_CH3I_RMA_Ops_isempty(&win_ptr->targets[target_rank].rma_ops_list)) {
        /* The ops list is empty -- NOTE: we assume this is because the epoch
           was flushed.  Any issued ops are already remote complete; done
           packet is not needed for safe third party communication. */
        *wait_for_rma_done_pkt = 0;
    }
    else {
        MPIDI_RMA_Op_t *tail = MPIDI_CH3I_RMA_Ops_tail(&win_ptr->targets[target_rank].rma_ops_list);

        /* Check if we can piggyback the RMA done acknowlegdement on the last
           operation in the epoch. */

        if (tail->type == MPIDI_RMA_GET ||
            tail->type == MPIDI_RMA_COMPARE_AND_SWAP ||
            tail->type == MPIDI_RMA_FETCH_AND_OP ||
            tail->type == MPIDI_RMA_GET_ACCUMULATE)
        {
            /* last operation sends a response message. no need to wait
               for an additional rma done pkt */
            *wait_for_rma_done_pkt = 0;
        }
        else {
            /* Check if there is a get operation, which can be be performed
               moved to the end to piggyback the RMA done acknowledgement.  Go
               through the list and move the first get operation (if there is
               one) to the end. */
            
            *wait_for_rma_done_pkt = 1;
            curr_ptr = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);
            
            while (curr_ptr != NULL) {
                if (curr_ptr->type == MPIDI_RMA_GET) {
		    /* Found a GET, move it to the end */
                    *wait_for_rma_done_pkt = 0;

                    MPIDI_CH3I_RMA_Ops_unlink(&win_ptr->targets[target_rank].rma_ops_list, curr_ptr);
                    MPIDI_CH3I_RMA_Ops_append(&win_ptr->targets[target_rank].rma_ops_list, curr_ptr);
                    break;
                }
                else {
                    curr_ptr    = curr_ptr->next;
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_winunlock_issue);

    curr_ptr = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    if (curr_ptr != NULL) {
        target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
    }

    while (curr_ptr != NULL)
    {
        MPIDI_CH3_Pkt_flags_t flags = MPIDI_CH3_PKT_FLAG_NONE;

        /* Assertion: (curr_ptr != NULL) => (nops > 0) */
        MPIU_Assert(nops > 0);
        MPIU_Assert(curr_ptr->target_rank == target_rank);

        /* Piggyback the lock operation on the first op */
        if (win_ptr->targets[target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED)
        {
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
            } else if (sync_flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
                flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH;
            } else {
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

        /* Track passive target write operations.  This is used during Win_free
         * to ensure that all writes to a given target have completed at that
         * process before the window is freed. */
#define MPIDI_CH3I_TRACK_RMA_WRITE(op_, win_ptr_) \
        do { (win_ptr_)->pt_rma_puts_accs[(op_)->target_rank]++; } while (0)

        MPIDI_CH3I_ISSUE_RMA_OP(curr_ptr, win_ptr, flags, source_win_handle,
                                target_win_handle, mpi_errno);
#undef MPIDI_CH3I_TRACK_RMA_WRITE

	/* If the request is null, we can remove it immediately */
	if (!curr_ptr->request) {
            MPIDI_CH3I_RMA_Ops_free_and_next(&win_ptr->targets[target_rank].rma_ops_list, &curr_ptr);
	}
	else  {
	    nRequest++;
	    MPIR_T_PVAR_COUNTER_INC(RMA, rma_winunlock_reqs, 1);
	    curr_ptr    = curr_ptr->next;
	    if (nRequest > MPIR_CVAR_CH3_RMA_NREQUEST_THRESHOLD &&
		nRequest - nRequestNew > MPIR_CVAR_CH3_RMA_NREQUEST_NEW_THRESHOLD) {
		int nDone = 0;
                mpi_errno = poke_progress_engine();
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		MPIR_T_PVAR_STMT(RMA, list_complete_timer=MPIR_T_PVAR_TIMER_ADDR(rma_winunlock_complete));
		MPIR_T_PVAR_STMT(RMA, list_complete_counter=MPIR_T_PVAR_COUNTER_ADDR(rma_winunlock_complete_aux));
                mpi_errno = rma_list_gc(win_ptr,
                                                  &win_ptr->targets[target_rank].rma_ops_list,
                                                  curr_ptr, &nDone);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		/* if (nDone > 0) printf( "nDone = %d\n", nDone ); */
		nRequest -= nDone;
		nRequestNew = nRequest;
	    }
	}
    }
    MPIR_T_PVAR_TIMER_END(RMA, rma_winunlock_issue);
    
    if (nops)
    {
        MPIR_T_PVAR_STMT(RMA, list_complete_timer=MPIR_T_PVAR_TIMER_ADDR(rma_winunlock_complete));
        MPIR_T_PVAR_STMT(RMA, list_block_timer=MPIR_T_PVAR_TIMER_ADDR(rma_winunlock_block));
        MPIR_T_PVAR_STMT(RMA, list_complete_counter=MPIR_T_PVAR_COUNTER_ADDR(rma_winunlock_complete_aux));
        mpi_errno = rma_list_complete(win_ptr, &win_ptr->targets[target_rank].rma_ops_list);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else if (sync_flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
        /* No communication operations were left to process, but the RMA epoch
           is open.  Send an unlock message to release the lock at the target.  */
        mpi_errno = send_unlock_msg(target_rank, win_ptr);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        *wait_for_rma_done_pkt = 1;
    }
    /* NOTE: Flush -- If RMA ops are issued eagerly, Send_flush_msg should be
       called here and wait_for_rma_done_pkt should be set. */

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
#define FUNCNAME send_lock_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_lock_msg(int dest, int lock_type, MPID_Win *win_ptr) {
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_t *lock_pkt = &upkt.lock;
    MPID_Request *req=NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_LOCK_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_LOCK_MSG);

    MPIU_Assert(win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
    lock_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    lock_pkt->source_win_handle = win_ptr->handle;
    lock_pkt->lock_type = lock_type;

    win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_REQUESTED;
    win_ptr->targets[dest].remote_lock_mode = lock_type;

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_pkt, sizeof(*lock_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* release the request returned by iStartMsg */
    if (req != NULL) {
        MPID_Request_release(req);
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_LOCK_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME acquire_local_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int acquire_local_lock(MPID_Win *win_ptr, int lock_type) {
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ACQUIRE_LOCAL_LOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ACQUIRE_LOCAL_LOCK);

    /* poke the progress engine until the local lock is granted */
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0)
    {
        MPID_Progress_state progress_state;

        MPIR_T_PVAR_TIMER_START(RMA, rma_winlock_getlocallock);
        MPID_Progress_start(&progress_state);
        while (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0)
        {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Progress_end(&progress_state);
                MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
            }
            /* --END ERROR HANDLING-- */
        }
        MPID_Progress_end(&progress_state);
        MPIR_T_PVAR_TIMER_END(RMA, rma_winlock_getlocallock);
    }

    win_ptr->targets[win_ptr->comm_ptr->rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
    win_ptr->targets[win_ptr->comm_ptr->rank].remote_lock_mode = lock_type;

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ACQUIRE_LOCAL_LOCK);
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
static int wait_for_lock_granted(MPID_Win *win_ptr, int target_rank) {
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

        MPIR_T_PVAR_TIMER_START(RMA, rma_winunlock_getlock);
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
        MPIR_T_PVAR_TIMER_END(RMA, rma_winunlock_getlock);
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
#define FUNCNAME send_unlock_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_unlock_msg(int dest, MPID_Win *win_ptr) {
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_unlock_t *unlock_pkt = &upkt.unlock;
    MPID_Request *req=NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_UNLOCK_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_UNLOCK_MSG);

    MPIU_Assert(win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_GRANTED);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    /* Send a lock packet over to the target. wait for the lock_granted
     * reply. Then do all the RMA ops. */

    MPIDI_Pkt_init(unlock_pkt, MPIDI_CH3_PKT_UNLOCK);
    unlock_pkt->target_win_handle = win_ptr->all_win_handles[dest];

    /* Reset the local state of the target to unlocked */
    win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, unlock_pkt, sizeof(*unlock_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* Release the request returned by iStartMsg */
    if (req != NULL) {
        MPID_Request_release(req);
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_UNLOCK_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* Commented out function to squash a warning, but retaining the code
 * for later use. */
#if 0
#undef FUNCNAME
#define FUNCNAME send_flush_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_flush_msg(int dest, MPID_Win *win_ptr) {
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_flush_t *flush_pkt = &upkt.flush;
    MPID_Request *req=NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_FLUSH_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_FLUSH_MSG);

    MPIU_Assert(win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_FLUSH);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(flush_pkt, MPIDI_CH3_PKT_FLUSH);
    flush_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    flush_pkt->source_win_handle = win_ptr->handle;

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
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
#endif


#undef FUNCNAME
#define FUNCNAME send_lock_put_or_acc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_lock_put_or_acc(MPID_Win *win_ptr, int target_rank)
{
    int mpi_errno=MPI_SUCCESS, lock_type, origin_dt_derived, iovcnt;
    MPIDI_RMA_Op_t *rma_op;
    MPID_Request *request=NULL;
    MPIDI_VC_t * vc;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Comm *comm_ptr;
    MPID_Datatype *origin_dtp=NULL;
    MPI_Aint origin_type_size;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_put_unlock_t *lock_put_unlock_pkt = 
	&upkt.lock_put_unlock;
    MPIDI_CH3_Pkt_lock_accum_unlock_t *lock_accum_unlock_pkt = 
	&upkt.lock_accum_unlock;
        
    MPIDI_STATE_DECL(MPID_STATE_SEND_LOCK_PUT_OR_ACC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_LOCK_PUT_OR_ACC);

    lock_type = win_ptr->targets[target_rank].remote_lock_mode;

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    win_ptr->pt_rma_puts_accs[rma_op->target_rank]++;

    if (rma_op->type == MPIDI_RMA_PUT) {
        MPIDI_Pkt_init(lock_put_unlock_pkt, MPIDI_CH3_PKT_LOCK_PUT_UNLOCK);
        lock_put_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
            MPIDI_CH3_PKT_FLAG_RMA_UNLOCK | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK;
        lock_put_unlock_pkt->target_win_handle = 
            win_ptr->all_win_handles[rma_op->target_rank];
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
        lock_accum_unlock_pkt->target_win_handle = 
            win_ptr->all_win_handles[rma_op->target_rank];
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
        lock_accum_unlock_pkt->target_win_handle = 
            win_ptr->all_win_handles[rma_op->target_rank];
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
	printf( "expected short accumulate...\n" );
	/* */
    }

    comm_ptr = win_ptr->comm_ptr;
    MPIDI_Comm_get_vc_set_active(comm_ptr, rma_op->target_rank, &vc);

    if (!MPIR_DATATYPE_IS_PREDEFINED(rma_op->origin_datatype))
    {
        origin_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }
    else
    {
        origin_dt_derived = 0;
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!origin_dt_derived)
    {
	/* basic datatype on origin */

        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
        iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
        iovcnt = 2;

	MPIU_THREAD_CS_ENTER(CH3COMM,vc);
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &request);
	MPIU_THREAD_CS_EXIT(CH3COMM,vc);
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
        }
    }
    else
    {
	/* derived datatype on origin */

        iovcnt = 1;

        request = MPID_Request_create();
        if (request == NULL) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomemreq");
        }

        MPIU_Object_set_ref(request, 2);
        request->kind = MPID_REQUEST_SEND;
	    
        request->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
           is freed. */ 

	request->dev.segment_ptr = MPID_Segment_alloc( );
        MPIU_ERR_CHKANDJUMP1(request->dev.segment_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype,
                          request->dev.segment_ptr, 0);
        request->dev.segment_first = 0;
        request->dev.segment_size = rma_op->origin_count * origin_type_size;
	    
        request->dev.OnFinal = 0;
        request->dev.OnDataAvail = 0;

        mpi_errno = vc->sendNoncontig_fn(vc, request, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            MPID_Datatype_release(request->dev.datatype_ptr);
            MPID_Request_release(request);
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|loadsendiov");
        }
        /* --END ERROR HANDLING-- */        
    }

    if (request != NULL) {
	if (!MPID_Request_is_complete(request))
        {
	    MPID_Progress_state progress_state;
	    
            MPID_Progress_start(&progress_state);
	    while (!MPID_Request_is_complete(request))
            {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
		    MPID_Progress_end(&progress_state);
		    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rma_msg");
                }
                /* --END ERROR HANDLING-- */
            }
	    MPID_Progress_end(&progress_state);
        }
        
        mpi_errno = request->status.MPI_ERROR;
        if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rma_msg");
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
static int send_lock_get(MPID_Win *win_ptr, int target_rank)
{
    int mpi_errno=MPI_SUCCESS, lock_type;
    MPIDI_RMA_Op_t *rma_op;
    MPID_Request *rreq=NULL, *sreq=NULL;
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *dtp;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_get_unlock_t *lock_get_unlock_pkt = 
	&upkt.lock_get_unlock;

    MPIDI_STATE_DECL(MPID_STATE_SEND_LOCK_GET);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_LOCK_GET);

    lock_type = win_ptr->targets[target_rank].remote_lock_mode;

    rma_op = MPIDI_CH3I_RMA_Ops_head(&win_ptr->targets[target_rank].rma_ops_list);

    /* create a request, store the origin buf, cnt, datatype in it,
       and pass a handle to it in the get packet. When the get
       response comes from the target, it will contain the request
       handle. */  
    rreq = MPID_Request_create();
    if (rreq == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomemreq");
    }

    MPIU_Object_set_ref(rreq, 2);

    rreq->dev.user_buf = rma_op->origin_addr;
    rreq->dev.user_count = rma_op->origin_count;
    rreq->dev.datatype = rma_op->origin_datatype;
    rreq->dev.target_win_handle = MPI_WIN_NULL;
    rreq->dev.source_win_handle = win_ptr->handle;

    if (!MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype))
    {
        MPID_Datatype_get_ptr(rreq->dev.datatype, dtp);
        rreq->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
           request is freed. */  
    }

    MPIDI_Pkt_init(lock_get_unlock_pkt, MPIDI_CH3_PKT_LOCK_GET_UNLOCK);
    lock_get_unlock_pkt->flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK |
        MPIDI_CH3_PKT_FLAG_RMA_UNLOCK; /* FIXME | MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK; */
    lock_get_unlock_pkt->target_win_handle = 
        win_ptr->all_win_handles[rma_op->target_rank];
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

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_get_unlock_pkt, sizeof(*lock_get_unlock_pkt), &sreq);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
    }

    /* release the request returned by iStartMsg */
    if (sreq != NULL)
    {
        MPID_Request_release(sreq);
    }

    /* now wait for the data to arrive */
    if (!MPID_Request_is_complete(rreq))
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
	while (!MPID_Request_is_complete(rreq))
        {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
		MPID_Progress_end(&progress_state);
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rma_msg");
            }
            /* --END ERROR HANDLING-- */
        }
	MPID_Progress_end(&progress_state);
    }
    
    mpi_errno = rreq->status.MPI_ERROR;
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rma_msg");
    }
            
    /* if origin datatype was a derived datatype, it will get freed when the 
       rreq gets freed. */ 
    MPID_Request_release(rreq);

    /* Free MPIDI_RMA_Ops_list - the lock packet should still be in place, so
     * we have to free two elements. */
    MPIDI_CH3I_RMA_Ops_free(&win_ptr->targets[target_rank].rma_ops_list);

 fn_fail:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_LOCK_GET);
    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/*
 * Utility routines
 */
/* ------------------------------------------------------------------------ */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_granted_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Send_lock_granted_pkt(MPIDI_VC_t *vc, MPID_Win *win_ptr,
                                     MPI_Win source_win_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_granted_t *lock_granted_pkt = &upkt.lock_granted;
    MPID_Request *req = NULL;
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);

    /* send lock granted packet */
    MPIDI_Pkt_init(lock_granted_pkt, MPIDI_CH3_PKT_LOCK_GRANTED);
    lock_granted_pkt->source_win_handle = source_win_handle;
    lock_granted_pkt->target_rank = win_ptr->comm_ptr->rank;

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,
                     (MPIU_DBG_FDEST, "sending lock granted pkt on vc=%p, source_win_handle=%#08x",
                      vc, lock_granted_pkt->source_win_handle));

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_granted_pkt, sizeof(*lock_granted_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    if (mpi_errno) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
    }

    if (req != NULL)
    {
        MPID_Request_release(req);
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);

    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/* 
 * The following routines are the packet handlers for the packet types 
 * used above in the implementation of the RMA operations in terms
 * of messages.
 */
/* ------------------------------------------------------------------------ */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Put( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
			      MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_put_t * put_pkt = &pkt->put;
    MPID_Request *req = NULL;
    MPI_Aint type_size;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received put pkt");

    if (put_pkt->count == 0)
    {
	/* it's a 0-byte message sent just to decrement the
	   completion counter. This happens only in
	   post/start/complete/wait sync model; therefore, no need
	   to check lock queue. */
	if (put_pkt->target_win_handle != MPI_WIN_NULL) {
            MPID_Win_get_ptr(put_pkt->target_win_handle, win_ptr);
            mpi_errno = MPIDI_CH3_Finish_rma_op_target(NULL, win_ptr, TRUE, put_pkt->flags, MPI_WIN_NULL);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	}
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
	*rreqp = NULL;
        goto fn_exit;
    }

    MPIU_Assert(put_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(put_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, put_pkt->flags);

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
                
    req->dev.user_buf = put_pkt->addr;
    req->dev.user_count = put_pkt->count;
    req->dev.target_win_handle = put_pkt->target_win_handle;
    req->dev.source_win_handle = put_pkt->source_win_handle;
    req->dev.flags = put_pkt->flags;
	
    if (MPIR_DATATYPE_IS_PREDEFINED(put_pkt->datatype))
    {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
        req->dev.datatype = put_pkt->datatype;
	    
        MPID_Datatype_get_size_macro(put_pkt->datatype,
                                     type_size);
        req->dev.recv_data_sz = type_size * put_pkt->count;
		    
        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                                  &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
        /* FIXME:  Only change the handling of completion if
           post_data_receive reset the handler.  There should
           be a cleaner way to do this */
        if (!req->dev.OnDataAvail) {
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutAccumRespComplete;
        }
        
        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t) + data_len;

        if (complete) 
        {
            mpi_errno = MPIDI_CH3_ReqHandler_PutAccumRespComplete(vc, req, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            if (complete)
            {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
    }
    else
    {
        /* derived datatype */
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT);
        req->dev.datatype = MPI_DATATYPE_NULL;
	    
        req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
            MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
        if (! req->dev.dtype_info) {
            MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_RMA_dtype_info");
        }

        req->dev.dataloop = MPIU_Malloc(put_pkt->dataloop_size);
        if (! req->dev.dataloop) {
            MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
				 put_pkt->dataloop_size);
        }

        /* if we received all of the dtype_info and dataloop, copy it
           now and call the handler, otherwise set the iov and let the
           channel copy it */
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + put_pkt->dataloop_size)
        {
            /* copy all of dtype_info and dataloop */
            MPIU_Memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            MPIU_Memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info), put_pkt->dataloop_size);

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + put_pkt->dataloop_size;
          
            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_PutRespDerivedDTComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT"); 
            if (complete)
            {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
        else
        {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *)req->dev.dtype_info);
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = put_pkt->dataloop_size;
            req->dev.iov_count = 2;

            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutRespDerivedDTComplete;
        }
        
    }
	
    *rreqp = req;

    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER,"**ch3|postrecv",
                      "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
    }
    

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Get( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
			      MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_get_t * get_pkt = &pkt->get;
    MPID_Request *req = NULL;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received get pkt");

    MPIU_Assert(get_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(get_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, get_pkt->flags);

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    req = MPID_Request_create();
    req->dev.target_win_handle = get_pkt->target_win_handle;
    req->dev.source_win_handle = get_pkt->source_win_handle;
    req->dev.flags = get_pkt->flags;

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;
    
    if (MPIR_DATATYPE_IS_PREDEFINED(get_pkt->datatype))
    {
	/* basic datatype. send the data. */
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
	
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP); 
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->dev.OnFinal     = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->kind = MPID_REQUEST_SEND;
	
	MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
	get_resp_pkt->request_handle = get_pkt->request_handle;
	
	iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
	
	iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)get_pkt->addr;
	MPID_Datatype_get_size_macro(get_pkt->datatype, type_size);
	iov[1].MPID_IOV_LEN = get_pkt->count * type_size;

        MPIU_THREAD_CS_ENTER(CH3COMM,vc);
	mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, 2);
        MPIU_THREAD_CS_EXIT(CH3COMM,vc);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(req, 0);
	    MPIDI_CH3_Request_destroy(req);
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
	}
	/* --END ERROR HANDLING-- */
	
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
	*rreqp = NULL;
    }
    else
    {
	/* derived datatype. first get the dtype_info and dataloop. */
	
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetRespDerivedDTComplete;
	req->dev.OnFinal     = 0;
	req->dev.user_buf = get_pkt->addr;
	req->dev.user_count = get_pkt->count;
	req->dev.datatype = MPI_DATATYPE_NULL;
	req->dev.request_handle = get_pkt->request_handle;
	
	req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
	    MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
	if (! req->dev.dtype_info) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_RMA_dtype_info");
	}
	
	req->dev.dataloop = MPIU_Malloc(get_pkt->dataloop_size);
	if (! req->dev.dataloop) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
				 get_pkt->dataloop_size);
	}
	
        /* if we received all of the dtype_info and dataloop, copy it
           now and call the handler, otherwise set the iov and let the
           channel copy it */
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + get_pkt->dataloop_size)
        {
            /* copy all of dtype_info and dataloop */
            MPIU_Memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            MPIU_Memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info), get_pkt->dataloop_size);

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + get_pkt->dataloop_size;
          
            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_GetRespDerivedDTComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET"); 
            if (complete)
                *rreqp = NULL;
        }
        else
        {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dtype_info;
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = get_pkt->dataloop_size;
            req->dev.iov_count = 2;
	
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            *rreqp = req;
        }
        
    }
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);
    return mpi_errno;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Accumulate( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				     MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_accum_t * accum_pkt = &pkt->accum;
    MPID_Request *req = NULL;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received accumulate pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_acc);
    MPIU_Assert(accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, accum_pkt->flags);

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    *rreqp = req;

    req->dev.user_count = accum_pkt->count;
    req->dev.op = accum_pkt->op;
    req->dev.real_user_buf = accum_pkt->addr;
    req->dev.target_win_handle = accum_pkt->target_win_handle;
    req->dev.source_win_handle = accum_pkt->source_win_handle;
    req->dev.flags = accum_pkt->flags;

    if (accum_pkt->type == MPIDI_CH3_PKT_GET_ACCUM) {
        req->dev.resp_request_handle = accum_pkt->request_handle;
    } else {
        req->dev.resp_request_handle = MPI_REQUEST_NULL;
    }

    if (MPIR_DATATYPE_IS_PREDEFINED(accum_pkt->datatype))
    {
       MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_acc_predef);
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP);
	req->dev.datatype = accum_pkt->datatype;

	MPIR_Type_get_true_extent_impl(accum_pkt->datatype, &true_lb, &true_extent);
	MPID_Datatype_get_extent_macro(accum_pkt->datatype, extent); 

        /* Predefined types should always have zero lb */
        MPIU_Assert(true_lb == 0);

	tmp_buf = MPIU_Malloc(accum_pkt->count * 
			      (MPIR_MAX(extent,true_extent)));
	if (!tmp_buf) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
			 accum_pkt->count * MPIR_MAX(extent,true_extent));
	}
	
	req->dev.user_buf = tmp_buf;
	
	MPID_Datatype_get_size_macro(accum_pkt->datatype, type_size);
	req->dev.recv_data_sz = type_size * accum_pkt->count;

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                                  &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
        /* FIXME:  Only change the handling of completion if
           post_data_receive reset the handler.  There should
           be a cleaner way to do this */
        if (!req->dev.OnDataAvail) {
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutAccumRespComplete;
        }
        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

        if (complete)
        {
            mpi_errno = MPIDI_CH3_ReqHandler_PutAccumRespComplete(vc, req, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            if (complete)
            {
                *rreqp = NULL;
                MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_acc_predef);
                goto fn_exit;
            }
        }
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_acc_predef);
    }
    else
    {
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumRespDerivedDTComplete;
	req->dev.datatype = MPI_DATATYPE_NULL;
                
	req->dev.dtype_info = (MPIDI_RMA_dtype_info *) 
	    MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
	if (! req->dev.dtype_info) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_RMA_dtype_info");
	}
	
	req->dev.dataloop = MPIU_Malloc(accum_pkt->dataloop_size);
	if (! req->dev.dataloop) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
				 accum_pkt->dataloop_size);
	}
	
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + accum_pkt->dataloop_size)
        {
            /* copy all of dtype_info and dataloop */
            MPIU_Memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            MPIU_Memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info), accum_pkt->dataloop_size);

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + accum_pkt->dataloop_size;
          
            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_AccumRespDerivedDTComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_ACCUMULATE"); 
            if (complete)
            {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
        else
        {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dtype_info;
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = accum_pkt->dataloop_size;
            req->dev.iov_count = 2;
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
        }
        
    }

    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**ch3|postrecv",
			     "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
    }

 fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_acc);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}

/* Special accumulate for short data items entirely within the packet */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Accumulate_Immed
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Accumulate_Immed( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					   MPIDI_msg_sz_t *buflen, 
					   MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_accum_immed_t * accum_pkt = &pkt->accum_immed;
    MPID_Win *win_ptr;
    MPI_Aint extent;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE_IMMED);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE_IMMED);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received accumulate immedidate pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_acc_immed);
    MPIU_Assert(accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, accum_pkt->flags);

    /* return the number of bytes processed in this function */
    /* data_len == 0 (all within packet) */
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp  = NULL;
    
    MPID_Datatype_get_extent_macro(accum_pkt->datatype, extent); 
    
	MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_acc_immed_op);
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
    /* Data is already present */
    if (accum_pkt->op == MPI_REPLACE) {
        /* no datatypes required */
        int len;
        MPIU_Assign_trunc(len, (accum_pkt->count * extent), int);
        /* FIXME: use immediate copy because this is short */
        MPIUI_Memcpy( accum_pkt->addr, accum_pkt->data, len );
    }
    else {
        if (HANDLE_GET_KIND(accum_pkt->op) == HANDLE_KIND_BUILTIN) {
            MPI_User_function *uop;
            /* get the function by indexing into the op table */
            uop = MPIR_OP_HDL_TO_FN(accum_pkt->op);
            (*uop)(accum_pkt->data, accum_pkt->addr,
                   &(accum_pkt->count), &(accum_pkt->datatype));
        }
        else {
            MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OP, "**opnotpredefined",
                                 "**opnotpredefined %d", accum_pkt->op );
        }
    }
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
	MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_acc_immed_op);

    /* There are additional steps to take if this is a passive
       target RMA or the last operation from the source */

    /* Here is the code executed in PutAccumRespComplete after the
       accumulation operation */
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);

    mpi_errno = MPIDI_CH3_Finish_rma_op_target(vc, win_ptr, TRUE,
                                               accum_pkt->flags,
                                               accum_pkt->source_win_handle);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

 fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_acc_immed);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE_IMMED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;

}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_CAS
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_CAS( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
                              MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_cas_resp_t *cas_resp_pkt = &upkt.cas_resp;
    MPIDI_CH3_Pkt_cas_t *cas_pkt = &pkt->cas;
    MPID_Win *win_ptr;
    MPID_Request *req;
    MPI_Aint len;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received CAS pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_cas);
    MPIU_Assert(cas_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(cas_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, cas_pkt->flags);

    /* return the number of bytes processed in this function */
    /* data_len == 0 (all within packet) */
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp  = NULL;

    MPIDI_Pkt_init(cas_resp_pkt, MPIDI_CH3_PKT_CAS_RESP);
    cas_resp_pkt->request_handle = cas_pkt->request_handle;

    /* Copy old value into the response packet */
    MPID_Datatype_get_size_macro(cas_pkt->datatype, len);
    MPIU_Assert(len <= sizeof(MPIDI_CH3_CAS_Immed_u));

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    MPIU_Memcpy( (void *)&cas_resp_pkt->data, cas_pkt->addr, len );

    /* Compare and replace if equal */
    if (MPIR_Compare_equal(&cas_pkt->compare_data, cas_pkt->addr, cas_pkt->datatype)) {
        MPIU_Memcpy(cas_pkt->addr, &cas_pkt->origin_data, len);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    /* Send the response packet */
    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_resp_pkt, sizeof(*cas_resp_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);

    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (req != NULL) {
        if (!MPID_Request_is_complete(req)) {
            /* sending process is not completed, set proper OnDataAvail
               (it is initialized to NULL by lower layer) */
            req->dev.target_win_handle = cas_pkt->target_win_handle;
            req->dev.flags = cas_pkt->flags;
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetAccumRespComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
               operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(req);
            goto fn_exit;
        }
        else
            MPID_Request_release(req);
    }


    /* There are additional steps to take if this is a passive 
       target RMA or the last operation from the source */

    mpi_errno = MPIDI_CH3_Finish_rma_op_target(NULL, win_ptr, TRUE,
                                               cas_pkt->flags,
                                               MPI_WIN_NULL);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_cas);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);
    return mpi_errno;
fn_fail:
    goto fn_exit;

}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_CASResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_CASResp( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
                                  MPIDI_CH3_Pkt_t *pkt,
                                  MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_cas_resp_t *cas_resp_pkt = &pkt->cas_resp;
    MPID_Request *req;
    MPI_Aint len;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_CASRESP);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_CASRESP);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received CAS response pkt");

    MPID_Request_get_ptr(cas_resp_pkt->request_handle, req);
    MPID_Datatype_get_size_macro(req->dev.datatype, len);
    
    MPIU_Memcpy( req->dev.user_buf, (void *)&cas_resp_pkt->data, len );

    MPIDI_CH3U_Request_complete( req );
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_CASRESP);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_FOP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_FOP( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
                              MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_fop_t *fop_pkt = &pkt->fop;
    MPID_Request *req;
    MPID_Win *win_ptr;
    int data_complete = 0;
    MPI_Aint len;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received FOP pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_fop);
    MPIU_Assert(fop_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(fop_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, fop_pkt->flags);

    req = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    MPIU_Object_set_ref(req, 1); /* Ref is held by progress engine */
    *rreqp = NULL;

    req->dev.user_buf = NULL; /* will be set later */
    req->dev.user_count = 1;
    req->dev.datatype = fop_pkt->datatype;
    req->dev.op = fop_pkt->op;
    req->dev.real_user_buf = fop_pkt->addr;
    req->dev.target_win_handle = fop_pkt->target_win_handle;
    req->dev.request_handle = fop_pkt->request_handle;
    req->dev.flags = fop_pkt->flags;

    MPID_Datatype_get_size_macro(req->dev.datatype, len);
    MPIU_Assert(len <= sizeof(MPIDI_CH3_FOP_Immed_u));

    /* Set up the user buffer and receive data if needed */
    if (len <= sizeof(fop_pkt->origin_data) || fop_pkt->op == MPI_NO_OP) {
        req->dev.user_buf = fop_pkt->origin_data;
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        data_complete = 1;
    }
    else {
        /* Data won't fit in the header, allocate temp space and receive it */
        MPIDI_msg_sz_t data_len;
        void *data_buf;

        data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
        data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
        req->dev.recv_data_sz = len; /* count == 1 for FOP */

        MPIU_CHKPMEM_MALLOC(req->dev.user_buf, void *, len, mpi_errno, "**nomemreq");

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &data_complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");

        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_FOPComplete;
        
        if (! data_complete) {
            *rreqp = req;
        }

        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    }

    if (data_complete) {
        int fop_complete = 0;
        mpi_errno = MPIDI_CH3_ReqHandler_FOPComplete(vc, req, &fop_complete);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        *rreqp = NULL;
    }

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_fop);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_FOPResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_FOPResp( MPIDI_VC_t *vc ATTRIBUTE((unused)),
                                  MPIDI_CH3_Pkt_t *pkt,
                                  MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_fop_resp_t *fop_resp_pkt = &pkt->fop_resp;
    MPID_Request *req;
    int complete = 0;
    MPI_Aint len;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received FOP response pkt");

    MPID_Request_get_ptr(fop_resp_pkt->request_handle, req);
    MPID_Datatype_get_size_macro(req->dev.datatype, len);

    if (len <= sizeof(fop_resp_pkt->data)) {
        MPIU_Memcpy( req->dev.user_buf, (void *)fop_resp_pkt->data, len );
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        complete = 1;
    }
    else {
        /* Data was too big to embed in the header */
        MPIDI_msg_sz_t data_len;
        void *data_buf;
        
        data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
        data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
        req->dev.recv_data_sz = len; /* count == 1 for FOP */
        *rreqp = req;

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf,
                                                  &data_len, &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                             "**ch3|postrecv", "**ch3|postrecv %s",
                             "MPIDI_CH3_PKT_GET_RESP");

        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    }

    if (complete) {
        MPIDI_CH3U_Request_complete( req );
        *rreqp = NULL;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Get_AccumResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Get_AccumResp( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
                                        MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &pkt->get_accum_resp;
    MPID_Request *req;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET_ACCUM_RESP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET_ACCUM_RESP);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received Get-Accumulate response pkt");
    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_get_accum);

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Request_get_ptr(get_accum_resp_pkt->request_handle, req);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;

    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
    MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                         "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_ACCUM_RESP");
    if (complete) {
        MPIDI_CH3U_Request_complete(req);
        *rreqp = NULL;
    }
    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_get_accum);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET_ACCUM_RESP);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Lock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
			       MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_t * lock_pkt = &pkt->lock;
    MPID_Win *win_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock pkt");
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_pkt->target_win_handle, win_ptr);
    
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, 
					lock_pkt->lock_type) == 1)
    {
	/* send lock granted packet. */
        mpi_errno = MPIDI_CH3I_Send_lock_granted_pkt(vc, win_ptr,
					     lock_pkt->source_win_handle);
    }

    else {
	/* queue the lock information */
	MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;
	
	/* Note: This code is reached by the fechandadd rma tests */
	/* FIXME: MT: This may need to be done atomically. */
	
	/* FIXME: Since we need to add to the tail of the list,
	   we should maintain a tail pointer rather than traversing the 
	   list each time to find the tail. */
	curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	prev_ptr = curr_ptr;
	while (curr_ptr != NULL)
	{
	    prev_ptr = curr_ptr;
	    curr_ptr = curr_ptr->next;
	}
	
       MPIR_T_PVAR_TIMER_START(RMA, rma_lockqueue_alloc);
	new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
	MPIR_T_PVAR_TIMER_END(RMA, rma_lockqueue_alloc);
	if (!new_ptr) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_Win_lock_queue");
	}
	if (prev_ptr != NULL)
	    prev_ptr->next = new_ptr;
	else 
	    win_ptr->lock_queue = new_ptr;
        
	new_ptr->next = NULL;  
	new_ptr->lock_type = lock_pkt->lock_type;
	new_ptr->source_win_handle = lock_pkt->source_win_handle;
	new_ptr->vc = vc;
	new_ptr->pt_single_op = NULL;
    }
    
    *rreqp = NULL;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockPutUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockPutUnlock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, 
					MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_put_unlock_t * lock_put_unlock_pkt = 
	&pkt->lock_put_unlock;
    MPID_Win *win_ptr = NULL;
    MPID_Request *req = NULL;
    MPI_Aint type_size;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    int (*fcn)( MPIDI_VC_t *, struct MPID_Request *, int * );
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock_put_unlock pkt");
    
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    
    req->dev.datatype = lock_put_unlock_pkt->datatype;
    MPID_Datatype_get_size_macro(lock_put_unlock_pkt->datatype, type_size);
    req->dev.recv_data_sz = type_size * lock_put_unlock_pkt->count;
    req->dev.user_count = lock_put_unlock_pkt->count;
    req->dev.target_win_handle = lock_put_unlock_pkt->target_win_handle;
    
    MPID_Win_get_ptr(lock_put_unlock_pkt->target_win_handle, win_ptr);
    
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, 
                                        lock_put_unlock_pkt->lock_type) == 1)
    {
	/* do the put. for this optimization, only basic datatypes supported. */
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutAccumRespComplete;
	req->dev.user_buf = lock_put_unlock_pkt->addr;
	req->dev.source_win_handle = lock_put_unlock_pkt->source_win_handle;
        req->dev.flags = lock_put_unlock_pkt->flags;
    }
    
    else {
	/* queue the information */
	MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;
	
	MPIR_T_PVAR_TIMER_START(RMA, rma_lockqueue_alloc);
	new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
	MPIR_T_PVAR_TIMER_END(RMA, rma_lockqueue_alloc);
	if (!new_ptr) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_Win_lock_queue");
	}
	
	new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
	if (new_ptr->pt_single_op == NULL) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_PT_single_op");
	}
	
	/* FIXME: MT: The queuing may need to be done atomically. */

	curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	prev_ptr = curr_ptr;
	while (curr_ptr != NULL)
	{
	    prev_ptr = curr_ptr;
	    curr_ptr = curr_ptr->next;
	}
	
	if (prev_ptr != NULL)
	    prev_ptr->next = new_ptr;
	else 
	    win_ptr->lock_queue = new_ptr;
        
	new_ptr->next = NULL;  
	new_ptr->lock_type = lock_put_unlock_pkt->lock_type;
	new_ptr->source_win_handle = lock_put_unlock_pkt->source_win_handle;
	new_ptr->vc = vc;
	
	new_ptr->pt_single_op->type = MPIDI_RMA_PUT;
	new_ptr->pt_single_op->flags = lock_put_unlock_pkt->flags;
	new_ptr->pt_single_op->addr = lock_put_unlock_pkt->addr;
	new_ptr->pt_single_op->count = lock_put_unlock_pkt->count;
	new_ptr->pt_single_op->datatype = lock_put_unlock_pkt->datatype;
	/* allocate memory to receive the data */
	new_ptr->pt_single_op->data = MPIU_Malloc(req->dev.recv_data_sz);
	if (new_ptr->pt_single_op->data == NULL) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
				 req->dev.recv_data_sz);
	}

	new_ptr->pt_single_op->data_recd = 0;

	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PT_SINGLE_PUT);
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SinglePutAccumComplete;
	req->dev.user_buf = new_ptr->pt_single_op->data;
	req->dev.lock_queue_entry = new_ptr;
    }

    fcn = req->dev.OnDataAvail;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                              &complete);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETFATALANDJUMP1(mpi_errno,MPI_ERR_OTHER,
                                  "**ch3|postrecv", "**ch3|postrecv %s",
                                  "MPIDI_CH3_PKT_LOCK_PUT_UNLOCK");
    }
    req->dev.OnDataAvail = fcn;
    *rreqp = req;

    if (complete)
    {
        mpi_errno = fcn(vc, req, &complete);
        if (complete)
        {
            *rreqp = NULL;
        }
    }

    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETFATALANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
				  "**ch3|postrecv", "**ch3|postrecv %s", 
				  "MPIDI_CH3_PKT_LOCK_PUT_UNLOCK");
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockGetUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockGetUnlock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_get_unlock_t * lock_get_unlock_pkt = 
	&pkt->lock_get_unlock;
    MPID_Win *win_ptr = NULL;
    MPI_Aint type_size;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock_get_unlock pkt");
    
    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_get_unlock_pkt->target_win_handle, win_ptr);
    
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, 
                                        lock_get_unlock_pkt->lock_type) == 1)
    {
	/* do the get. for this optimization, only basic datatypes supported. */
	MPIDI_CH3_Pkt_t upkt;
	MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
	MPID_Request *req;
	MPID_IOV iov[MPID_IOV_LIMIT];
	
	req = MPID_Request_create();
	req->dev.target_win_handle = lock_get_unlock_pkt->target_win_handle;
	req->dev.source_win_handle = lock_get_unlock_pkt->source_win_handle;
        req->dev.flags = lock_get_unlock_pkt->flags;
	
	MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP); 
	req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->dev.OnFinal     = MPIDI_CH3_ReqHandler_GetSendRespComplete;
	req->kind = MPID_REQUEST_SEND;

        /* here we increment the Active Target counter to guarantee the GET-like
           operation are completed when counter reaches zero. */
        win_ptr->at_completion_counter++;
	
	MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
	get_resp_pkt->request_handle = lock_get_unlock_pkt->request_handle;
	
	iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
	iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
	
	iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)lock_get_unlock_pkt->addr;
	MPID_Datatype_get_size_macro(lock_get_unlock_pkt->datatype, type_size);
	iov[1].MPID_IOV_LEN = lock_get_unlock_pkt->count * type_size;
	
	mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, 2);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Object_set_ref(req, 0);
	    MPIDI_CH3_Request_destroy(req);
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
	}
	/* --END ERROR HANDLING-- */
    }

    else {
	/* queue the information */
	MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;
	
	/* FIXME: MT: This may need to be done atomically. */

	curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	prev_ptr = curr_ptr;
	while (curr_ptr != NULL)
	{
	    prev_ptr = curr_ptr;
	    curr_ptr = curr_ptr->next;
	}
	
	MPIR_T_PVAR_TIMER_START(RMA, rma_lockqueue_alloc);
	new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
	MPIR_T_PVAR_TIMER_END(RMA, rma_lockqueue_alloc);
	if (!new_ptr) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_Win_lock_queue");
	}
	new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
	if (new_ptr->pt_single_op == NULL) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
				 "MPIDI_PT_Single_op");
	}
	
	if (prev_ptr != NULL)
	    prev_ptr->next = new_ptr;
	else 
	    win_ptr->lock_queue = new_ptr;
        
	new_ptr->next = NULL;  
	new_ptr->lock_type = lock_get_unlock_pkt->lock_type;
	new_ptr->source_win_handle = lock_get_unlock_pkt->source_win_handle;
	new_ptr->vc = vc;
	
	new_ptr->pt_single_op->type = MPIDI_RMA_GET;
	new_ptr->pt_single_op->flags = lock_get_unlock_pkt->flags;
	new_ptr->pt_single_op->addr = lock_get_unlock_pkt->addr;
	new_ptr->pt_single_op->count = lock_get_unlock_pkt->count;
	new_ptr->pt_single_op->datatype = lock_get_unlock_pkt->datatype;
	new_ptr->pt_single_op->data = NULL;
	new_ptr->pt_single_op->request_handle = lock_get_unlock_pkt->request_handle;
	new_ptr->pt_single_op->data_recd = 1;
    }
    
    *rreqp = NULL;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockAccumUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockAccumUnlock( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
					  MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_accum_unlock_t * lock_accum_unlock_pkt = 
	&pkt->lock_accum_unlock;
    MPID_Request *req = NULL;
    MPID_Win *win_ptr = NULL;
    MPIDI_Win_lock_queue *curr_ptr = NULL, *prev_ptr = NULL, *new_ptr = NULL;
    MPI_Aint type_size;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock_accum_unlock pkt");
    
    /* no need to acquire the lock here because we need to receive the 
       data into a temporary buffer first */
    
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    
    req->dev.datatype = lock_accum_unlock_pkt->datatype;
    MPID_Datatype_get_size_macro(lock_accum_unlock_pkt->datatype, type_size);
    req->dev.recv_data_sz = type_size * lock_accum_unlock_pkt->count;
    req->dev.user_count = lock_accum_unlock_pkt->count;
    req->dev.target_win_handle = lock_accum_unlock_pkt->target_win_handle;
    req->dev.flags = lock_accum_unlock_pkt->flags;
    
    /* queue the information */
    
    MPIR_T_PVAR_TIMER_START(RMA, rma_lockqueue_alloc);
    new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
    MPIR_T_PVAR_TIMER_END(RMA, rma_lockqueue_alloc);
    if (!new_ptr) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
			     "MPIDI_Win_lock_queue");
    }
    
    new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
    if (new_ptr->pt_single_op == NULL) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
			     "MPIDI_PT_single_op");
    }
    
    MPID_Win_get_ptr(lock_accum_unlock_pkt->target_win_handle, win_ptr);
    
    /* FIXME: MT: The queuing may need to be done atomically. */
    
    curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
    prev_ptr = curr_ptr;
    while (curr_ptr != NULL)
    {
	prev_ptr = curr_ptr;
	curr_ptr = curr_ptr->next;
    }
    
    if (prev_ptr != NULL)
	prev_ptr->next = new_ptr;
    else 
	win_ptr->lock_queue = new_ptr;
    
    new_ptr->next = NULL;  
    new_ptr->lock_type = lock_accum_unlock_pkt->lock_type;
    new_ptr->source_win_handle = lock_accum_unlock_pkt->source_win_handle;
    new_ptr->vc = vc;
    
    new_ptr->pt_single_op->type = MPIDI_RMA_ACCUMULATE;
    new_ptr->pt_single_op->flags = lock_accum_unlock_pkt->flags;
    new_ptr->pt_single_op->addr = lock_accum_unlock_pkt->addr;
    new_ptr->pt_single_op->count = lock_accum_unlock_pkt->count;
    new_ptr->pt_single_op->datatype = lock_accum_unlock_pkt->datatype;
    new_ptr->pt_single_op->op = lock_accum_unlock_pkt->op;
    /* allocate memory to receive the data */
    new_ptr->pt_single_op->data = MPIU_Malloc(req->dev.recv_data_sz);
    if (new_ptr->pt_single_op->data == NULL) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
			     req->dev.recv_data_sz);
    }
    
    new_ptr->pt_single_op->data_recd = 0;
    
    MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PT_SINGLE_ACCUM);
    req->dev.user_buf = new_ptr->pt_single_op->data;
    req->dev.lock_queue_entry = new_ptr;
    
    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len,
                                              &complete);
    /* FIXME:  Only change the handling of completion if
       post_data_receive reset the handler.  There should
       be a cleaner way to do this */
    if (!req->dev.OnDataAvail) {
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SinglePutAccumComplete;
    }
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER,"**ch3|postrecv",
                      "**ch3|postrecv %s", "MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK");
    }
    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

    if (complete)
    {
        mpi_errno = MPIDI_CH3_ReqHandler_SinglePutAccumComplete(vc, req, &complete);
        if (complete)
        {
            *rreqp = NULL;
        }
    }
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_GetResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_GetResp( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
				  MPIDI_CH3_Pkt_t *pkt,
				  MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &pkt->get_resp;
    MPID_Request *req;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received get response pkt");

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *)pkt + sizeof(MPIDI_CH3_Pkt_t);
    
    MPID_Request_get_ptr(get_resp_pkt->request_handle, req);
    
    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;
    
    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf,
                                              &data_len, &complete);
    MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv", "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_RESP");
    if (complete)
    {
        MPIDI_CH3U_Request_complete(req);
        *rreqp = NULL;
    }
    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockGranted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockGranted( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				      MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_lock_granted_t * lock_granted_pkt = &pkt->lock_granted;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received lock granted pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_granted_pkt->source_win_handle, win_ptr);
    /* set the remote_lock_state flag in the window */
    win_ptr->targets[lock_granted_pkt->target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
    
    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();	

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_PtRMADone
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_PtRMADone( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
				    MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    MPIDI_CH3_Pkt_pt_rma_done_t * pt_rma_done_pkt = &pkt->pt_rma_done;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_PTRMADONE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_PTRMADONE);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received shared lock ops done pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(pt_rma_done_pkt->source_win_handle, win_ptr);
    MPIU_Assert(win_ptr->targets[pt_rma_done_pkt->target_rank].remote_lock_state != MPIDI_CH3_WIN_LOCK_NONE);

    if (win_ptr->targets[pt_rma_done_pkt->target_rank].remote_lock_state == MPIDI_CH3_WIN_LOCK_FLUSH)
        win_ptr->targets[pt_rma_done_pkt->target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
    else
        win_ptr->targets[pt_rma_done_pkt->target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();	

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_PTRMADONE);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Unlock( MPIDI_VC_t *vc ATTRIBUTE((unused)),
                                 MPIDI_CH3_Pkt_t *pkt,
                                 MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_unlock_t * unlock_pkt = &pkt->unlock;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_UNLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_UNLOCK);
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received unlock pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    MPID_Win_get_ptr(unlock_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    MPIDI_CH3_Progress_signal_completion();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_UNLOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Flush
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Flush( MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
                                MPIDI_msg_sz_t *buflen, MPID_Request **rreqp )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_flush_t * flush_pkt = &pkt->flush;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSH);
    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received flush pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    /* This is a flush request packet */
    if (flush_pkt->target_win_handle != MPI_WIN_NULL) {
        MPID_Request *req=NULL;

        MPID_Win_get_ptr(flush_pkt->target_win_handle, win_ptr);

        flush_pkt->target_win_handle = MPI_WIN_NULL;
        flush_pkt->target_rank = win_ptr->comm_ptr->rank;

        MPIU_THREAD_CS_ENTER(CH3COMM,vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
        MPIU_THREAD_CS_EXIT(CH3COMM,vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

        /* Release the request returned by iStartMsg */
        if (req != NULL) {
            MPID_Request_release(req);
        }
    }

    /* This is a flush response packet */
    else {
        MPID_Win_get_ptr(flush_pkt->source_win_handle, win_ptr);
        win_ptr->targets[flush_pkt->target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
        MPIDI_CH3_Progress_signal_completion();
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* ------------------------------------------------------------------------ */
/* list_complete_timer/counter and list_block_timer defined above */

static inline int poke_progress_engine(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    mpi_errno = MPID_Progress_poke();
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    MPID_Progress_end(&progress_state);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static inline int rma_list_complete( MPID_Win *win_ptr,
                                       MPIDI_RMA_Ops_list_t *ops_list )
{
    int ntimes = 0, mpi_errno=0;
    MPIDI_RMA_Op_t *curr_ptr;
    MPID_Progress_state progress_state;

    MPIR_T_PVAR_TIMER_START_VAR(RMA, list_complete_timer);
    MPID_Progress_start(&progress_state);
    /* Process all operations until they are complete */
    while (!MPIDI_CH3I_RMA_Ops_isempty(ops_list)) {
        int nDone = 0;
        mpi_errno = rma_list_gc(win_ptr, ops_list, NULL, &nDone);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
	ntimes++;
        
	/* Wait for something to arrive*/
	/* In some tests, this hung unless the test ensured that 
	   there was an incomplete request. */
        curr_ptr = MPIDI_CH3I_RMA_Ops_head(ops_list);

        /* MT: avoid processing unissued operations enqueued by other
           threads in MPID_Progress_wait() */
        if (curr_ptr && !curr_ptr->request) {
            /* This RMA operation has not been issued yet. */
            break;
        }
	if (curr_ptr && !MPID_Request_is_complete(curr_ptr->request) ) {
	    MPIR_T_PVAR_TIMER_START_VAR(RMA, list_block_timer);
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS) {
		MPID_Progress_end(&progress_state);
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**winnoprogress");
	    }
	    /* --END ERROR HANDLING-- */
	    MPIR_T_PVAR_TIMER_END_VAR(RMA, list_block_timer);
	}
    } /* While list of rma operation is non-empty */
    MPID_Progress_end(&progress_state);
    MPIR_T_PVAR_COUNTER_INC_VAR(RMA, list_complete_counter, ntimes);
    MPIR_T_PVAR_TIMER_END_VAR(RMA, list_complete_timer);

 fn_fail:
    return mpi_errno;
}

/* This routine is used to do garbage collection work on completed RMA
   requests so far. It is used to clean up the RMA requests that are
   not completed immediately when issuing out but are completed later
   when poking progress engine, so that they will not waste internal
   resources.
*/
static inline int rma_list_gc( MPID_Win *win_ptr,
                                              MPIDI_RMA_Ops_list_t *ops_list,
                                              MPIDI_RMA_Op_t *last_elm,
					      int *nDone )
{
    int mpi_errno=0;
    MPIDI_RMA_Op_t *curr_ptr;
    int nComplete = 0;
    int nVisit = 0;

    MPIR_T_PVAR_TIMER_START_VAR(RMA, list_complete_timer);

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
	       as many as possible until we find an incomplete
	       or null request */
	    do {
		nComplete++;
		mpi_errno = curr_ptr->request->status.MPI_ERROR;
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS) {
		    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rma_msg");
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
	    while (curr_ptr && curr_ptr != last_elm && 
		   MPID_Request_is_complete(curr_ptr->request)) ;
            if ((MPIR_CVAR_CH3_RMA_GC_NUM_TESTED >= 0 &&
                 nVisit >= MPIR_CVAR_CH3_RMA_GC_NUM_TESTED) ||
                (MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED >= 0 &&
                 nComplete >= MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED)) {
                /* MPIR_CVAR_CH3_RMA_GC_NUM_TESTED: Once we tested certain
                   number of requests, we stop checking the rest of the
                   operation list and break out the loop. */
                /* MPIR_CVAR_CH3_RMA_GC_NUM_COMPLETED: Once we found
                   certain number of completed requests, we stop checking
                   the rest of the operation list and break out the loop. */
	    break;
            }
	}
	else {
	    /* proceed to the next entry.  */
	    curr_ptr    = curr_ptr->next;
            nVisit++;
            if (MPIR_CVAR_CH3_RMA_GC_NUM_TESTED >= 0 &&
                nVisit >= MPIR_CVAR_CH3_RMA_GC_NUM_TESTED) {
                /* MPIR_CVAR_CH3_RMA_GC_NUM_TESTED: Once we tested certain
                   number of requests, we stop checking the rest of the
                   operation list and break out the loop. */
                break;
            }
	}
    } while (curr_ptr && curr_ptr != last_elm);
        
    /* if (nComplete) printf( "Completed %d requests\n", nComplete ); */
    MPIR_T_PVAR_COUNTER_INC_VAR(RMA, list_complete_counter, 1);
    MPIR_T_PVAR_TIMER_END_VAR(RMA, list_complete_timer);

    *nDone = nComplete;

 fn_fail:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Start_rma_op_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Start_rma_op_target(MPID_Win *win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_START_RMA_OP_TARGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_START_RMA_OP_TARGET);

    /* Lock with NOCHECK is piggybacked on this message.  We should be able to
     * immediately grab the lock.  Otherwise, there is a synchronization error. */
    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK &&
        flags & MPIDI_CH3_PKT_FLAG_RMA_NOCHECK)
    {
        int lock_acquired;
        int lock_mode;

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_SHARED) {
            lock_mode = MPI_LOCK_SHARED;
        } else if (flags & MPIDI_CH3_PKT_FLAG_RMA_EXCLUSIVE) {
            lock_mode = MPI_LOCK_EXCLUSIVE;
        } else {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RMA_SYNC, "**ch3|rma_flags");
        }

        lock_acquired = MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_mode);
        MPIU_ERR_CHKANDJUMP(!lock_acquired, mpi_errno, MPI_ERR_RMA_SYNC, "**ch3|nocheck_invalid");
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_START_RMA_OP_TARGET);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finish_rma_op_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finish_rma_op_target(MPIDI_VC_t *vc, MPID_Win *win_ptr, int is_rma_update,
                                   MPIDI_CH3_Pkt_flags_t flags, MPI_Win source_win_handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_FINISH_RMA_OP_TARGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_FINISH_RMA_OP_TARGET);

    /* This function should be called by the target process after each RMA
       operation is completed, to update synchronization state. */

    /* If this is a passive target RMA update operation, increment counter.  This is
       needed in Win_free to ensure that all ops are completed before a window
       is freed. */
    if (win_ptr->current_lock_type != MPID_LOCK_NONE && is_rma_update)
        win_ptr->my_pt_rma_puts_accs++;

    /* Last RMA operation from source. If active target RMA, decrement window
       counter. */
    if (flags & MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE) {
        MPIU_Assert(win_ptr->current_lock_type == MPID_LOCK_NONE);

        win_ptr->at_completion_counter -= 1;
        MPIU_Assert(win_ptr->at_completion_counter >= 0);

        /* Signal the local process when the op counter reaches 0. */
        if (win_ptr->at_completion_counter == 0)
            MPIDI_CH3_Progress_signal_completion();
    }

    /* If passive target RMA, release lock on window and grant next lock in the
       lock queue if there is any.  If requested by the origin, send an ack back
       to indicate completion at the target. */
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
        MPIU_Assert(win_ptr->current_lock_type != MPID_LOCK_NONE);

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK) {
            MPIU_Assert(source_win_handle != MPI_WIN_NULL && vc != NULL);
            mpi_errno = MPIDI_CH3I_Send_pt_rma_done_pkt(vc, win_ptr, source_win_handle);
            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
        }

        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* The local process may be waiting for the lock.  Signal completion to
           wake it up, so it can attempt to grab the lock. */
        MPIDI_CH3_Progress_signal_completion();
    }
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
        /* Ensure store instructions have been performed before flush call is
         * finished on origin process. */
        OPA_read_write_barrier();

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK) {
            MPIDI_CH3_Pkt_t upkt;
            MPIDI_CH3_Pkt_flush_t *flush_pkt = &upkt.flush;
            MPID_Request *req = NULL;

            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received piggybacked flush request");
            MPIU_Assert(source_win_handle != MPI_WIN_NULL && vc != NULL);

            MPIDI_Pkt_init(flush_pkt, MPIDI_CH3_PKT_FLUSH);
            flush_pkt->source_win_handle = source_win_handle;
            flush_pkt->target_win_handle = MPI_WIN_NULL;
            flush_pkt->target_rank = win_ptr->comm_ptr->rank;

            MPIU_THREAD_CS_ENTER(CH3COMM,vc);
            mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
            MPIU_THREAD_CS_EXIT(CH3COMM,vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

            /* Release the request returned by iStartMsg */
            if (req != NULL) {
                MPID_Request_release(req);
            }
        }
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_FINISH_RMA_OP_TARGET);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* ------------------------------------------------------------------------ */
/* 
 * For debugging, we provide the following functions for printing the 
 * contents of an RMA packet
 */
/* ------------------------------------------------------------------------ */
#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_Put( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_PUT\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->put.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->put.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->put.datatype));
    MPIU_DBG_PRINTF((" dataloop_size. 0x%08X\n", pkt->put.dataloop_size));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->put.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->put.source_win_handle));
    /*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->put.win_ptr));*/
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_Get( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->get.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->get.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->get.datatype));
    MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->get.dataloop_size));
    MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request_handle));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->get.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->get.source_win_handle));
    /*
      MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request));
      MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->get.win_ptr));
    */
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_GetResp( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET_RESP\n"));
    MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request_handle));
    /*MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request));*/
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_Accumulate( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_ACCUMULATE\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->accum.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->accum.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->accum.datatype));
    MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->accum.dataloop_size));
    MPIU_DBG_PRINTF((" op ........... 0x%08X\n", pkt->accum.op));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->accum.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->accum.source_win_handle));
    /*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->accum.win_ptr));*/
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_Accum_Immed( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_ACCUM_IMMED\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->accum_immed.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->accum_immed.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->accum_immed.datatype));
    MPIU_DBG_PRINTF((" op ........... 0x%08X\n", pkt->accum_immed.op));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->accum_immed.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->accum_immed.source_win_handle));
    /*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->accum.win_ptr));*/
    fflush(stdout);
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_Lock( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK\n"));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock.source_win_handle));
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_LockPutUnlock( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_PUT_UNLOCK\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_put_unlock.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_put_unlock.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_put_unlock.datatype));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_put_unlock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_put_unlock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_put_unlock.source_win_handle));
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_LockAccumUnlock( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_accum_unlock.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_accum_unlock.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_accum_unlock.datatype));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_accum_unlock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_accum_unlock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_accum_unlock.source_win_handle));
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_LockGetUnlock( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_GET_UNLOCK\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_get_unlock.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_get_unlock.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_get_unlock.datatype));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_get_unlock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_get_unlock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_get_unlock.source_win_handle));
    MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->lock_get_unlock.request_handle));
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_PtRMADone( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_PT_RMA_DONE\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_accum_unlock.source_win_handle));
    return MPI_SUCCESS;
}
int MPIDI_CH3_PktPrint_LockGranted( FILE *fp, MPIDI_CH3_Pkt_t *pkt )
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_GRANTED\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_granted.source_win_handle));
    return MPI_SUCCESS;
}
#endif
