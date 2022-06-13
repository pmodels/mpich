This page will be used for planning and management of the MPI-3 RMA
reference implementation in MPICH2.

*The MPICH MPI-3 RMA reference implementation was completed on Nov. 9,
2012.*

## MPI-3 RMA Reference Implementation Timeline

1.  Memory model
    1.  Separate -\> Unified
          - No changes needed
    2.  Accumulate operation ordering
          - No changes needed (CH3 already provides ordering as long as
            we don't move these operations around in the RMA ops queue).
2.  Dynamic windows
      - Done, committed to trunk
3.  Shared memory windows
      - Basic support, committed to trunk. Not optimized for shared
        memory; ops still queue up and will go through CH3
        communication.
      - Synchronization operations must be updated for this window type
    \#\* Lock - must take the lock immediately
    \#\* Unlock - needs a memory barrier
    \#\* Flush - needs a memory barrier
      - Additional sync operations are needed when flushing shared
        memory windows.
4.  New one-sided communication operations
    1.  Compare-and-swap
          - Done, committed to trunk
    2.  Fetch-and-op
          - Done, committed to trunk
    3.  Get-accumulate
          - Done, committed to trunk.
5.  Basic synchronization operations
    1.  Ability to acquire lock before unlock
          - Done, committed to trunk.
    2.  Ability to issue ops before unlock
          - Done, committed to trunk.
    3.  Ability to remove completed operations from the RMA ops list
          - Done, committed to trunk.
    4.  Flush
          - Done, committed to trunk.
    5.  Sync
          - Done, committed to trunk. Implemented as an
            OPA_read_write_barrier().
    6.  Request-based operations
          - Done, committed to trunk.
6.  Multi-target support
    1.  Simple locking of multiple targets
          - In CR
    2.  Lock-all
          - In CR
    3.  Flush-all
          - In CR
7.  Info operations: MPI_Win_set_info and MPI_Win_get_info
    \#\* Done, committed to trunk.