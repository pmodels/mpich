**Note: Please do not delete words with strikethrough.**

-----

1.  Scalability issues:
    1.  <s>Global pool for lock requests </s>
    2.  <s> Global pool for data in RMA operation piggybacked with LOCK
        </s>
    3.  <s> Streaming ACC operation </s>
    4.  <s> Scalability problem with derived datatypes </s>
        1.  <s> Send/recv has the same problem. </s>
        2.  <s> Fixing it needs flow control in mpich which brings a lot
            of overhead, so here we will not fix this. </s>
    5.  <s> Allow user to pass MPI info hints to indicate if they will
        use passive target on this window, if not, we do not need to
        allocate lock request / data block pool. </s>
    6.  <s> Manage internal requests? --- currently it is bounded by a
        CVAR. </s>
    7.  Tracking RMA failures: [New RMA failures in MPICH-3.2
        release](https://wiki.mpich.org/mpich/index.php/RMA_Failures)
2.  Synchronization issues:
    1.  Current PSCW might not be correct. we might need to do SEND/RECV
        in Win_complete and Win_wait.
3.  Performance profiling
    1.  OSU benchmark: simple latency and bandwidth
        1.  Poking progress engine worsen the performance, should
            optimize the progress engine
    2.  Graph500
        1.  Poking progress engine in operation routines improve the
            performance
        2.  MXM RC is worse then UD on Fusion
            1.  Commit 1555ef7 MPICH-specific initialization of mxm
            2.  Routine mxm_invoke_callback() spend a lot of time
            3.  Reproduce it in a simple program
        3.  Needs to test more data sets
4.  **Documentation**
5.  Channel hooks and Netmod hooks
    1.  Window creation: done
    2.  Synchronization: add hardware flush (performance is not good),
        other synchronizations are not done
        1.  Issues (TODO):
            1.  PUT_SYNC + FENCE does not guarantee remote completion
                in SHM environment (i.e., NOLOCAL), we may need handle
                it in MPICH or MXM provides an unified interface for
                remote completion.
            2.  MXM HW flush does not call MPI progress engine, so when
                it is blocking in HW flush we cannot make progress on
                other SW parts (may deadlock in multithread ?).
        2.  Performance improvement (DONE):
            1.  MXM HW flush internally issues PUT_SYNC out only when
                there are PUT/GET operations which are issued out and
                waiting for remote completion, otherwise do nothing.
                This optimization can avoid unnecessary HW flush when
                there is no operation waiting for remote completion.
                1.  PUT is always waiting for remote completion before
                    HW flush.
                2.  GET is waiting for remote completion until local
                    completion
        3.  Possible optimization (TODO):
            1.  HW flush can be ignored if SW flush is also issued
                (i.e., SW / HW OPs mixed case), because MXM guarantees
                the ordering of all OPs including Send/Receive and RDMA
                issued on the same connection. CH3 may give netmod a
                hint (i.e., force_flag) in HW flush, so netmod can
                ignore HW flush if strict ordering is supported as that
                in MXM.
            2.  In GET-only case, we do not need issue HW flush in MXM,
                instead we can wait for a counter decremented by GET
                local completion.
    3.  RMA operations: PUT/GET done
        1.  Possible optimization (TODO):
            1.  In MXM RMA, only need return MPI_Request for Rput/Rget.
    4.  Rebase on mpich/master: done
    5.  Code might needs to be cleaned up
    6.  Epoch check in win_free
        1.  Add epoch check in nemesis win_free (DONE)
        2.  Add a error test to check epoch check in win_free for
            allocate_shm window (see /errors/rma/win_sync_free_at)
            (TODO)
    7.  Merge shm_win_free into nemesis_win_free so that it is
        called after netmod win_free (TODO)
6.  Miscellaneous TODOs:
    1.  Add new PVARs for RMA:
        1.  Do this at last
    2.  Support for multi-level request handlers
    3.  Support for network-specific AM orderings
    4.  window does not store infos passed by user
    5.  Design document
7.  Very detailed TODOs:
    1.  <s> Completely remote immed_len in pkt struct? Does origin type
        and target type must be the same if basic datatype is specified?
        </s>
    2.  <s> FOP operation and CAS operation, specify IMMED data size??
        </s>
    3.  <s> Find packet header limit, do not make them too big </s>
    4.  Make sure aggressive cleanup functions work correctly in all
        situations
    5.  Make sure runtime works correctly when pool is very small
    6.  Make sure graph500 runs correctly with validation