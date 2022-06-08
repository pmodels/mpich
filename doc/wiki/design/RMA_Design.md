# Old RMA Design

The MPICH ADI allows devices to provide customized support for the MPI
RMA operations. The CH3 Channel provides a default implementation that
relies only on the CH3 operations, along with provisions for channel
extensions. In addition, the CH3 RMA implementation contains features to
minimize the number of messages used for synchronization (see (EuroMPI
paper)).

A redesign and extension of the CH3 RMA implementation will be (is)
described below. This will optimize for both short (latency-bound) and
long (bandwidth bound) RMA operations.

Additional details of the MPI-3 RMA development process are available at
the [MPI-3 RMA Implementation Timeline](MPI-3_RMA_Implementation_Timeline "wikilink").

## Design Objectives

The RMA design has many competing requirements. The obvious top-level
requirements are

1.  Full MPI-3 support, including new creation routines, optional
    ordering, flush, and new read-modify-write operations
2.  High performance for latency-bound operations (e.g., single word
    put, accumulate, or get)
3.  High performance for bandwidth-bound operations (e.g., multiple puts
    of 10's of KB of data).
4.  Exploit available hardware support, including shared memory and
    networks supporting RDMA. This includes put/get/accumulate into
    local memory
5.  Scalable algorithms and data; in particular, MPI Window data needs
    to be scalable (can also support the new MPI-3 window creation
    routines).
6.  MPI-2 and MPI-3 synchronization options.
7.  Extendable to different hardware systems

## Implications

High performance for latency-bound operations requires both that there
be a short code path for these and that network transactions be
minimized. In turn, this implies that there is specialization for such
things as contiguous datatypes and combined lock/operation/unlock for
passive target. It also means following the principle that decisions are
made once; for example, once it is determined that a transfer is
contiguous, that shouldn't be tested again, and that the number of data
copies should be minimized.

High performance for both bandwidth-bound and large numbers of short
operations requires that these operations be initiated as early as
possible. MPI-3 (through the new request interface) requires that the
user be able to individually wait on these operations. Datatype caching
should be performed as well for non-contiguous datatypes.

Support for fast operation within an SMP will require that the SMP path,
like in the nemesis channel, is given highest priority.

## Evaluation of the current MPICH RMA Support

The implementation for MPI-2 RMA in MPICH is the responsibility of the
device; in the ch3 device, the current support uses the channel
communication functions, along with the receive handler functions, to
implement RMA. This provides a two-sided implementation that does not
provide a good route to accessing lower-level interconnect features. In
addition, the implementation was designed to support the general case,
with only a few optimizations added much later to support a few special
cases (particularly for short accumulates). Also, the approach uses lazy
synchronization, which provides better performance for latency-bound
operations and for short groups of RMA operations but does not provide
good support for bandwidth bound operations or for
communication/computation overlap. Good points include general
correctness, fairly detailed internal instrumentation.

## Miscellaneous Thoughts

Consider designing for multicore SMP nodes. This means that some data is
stored within shared memory, and could include MPI_Win data, cached
datatypes, and for the new shared-memory windows, the lock state. We
should also consider matching the RMA operations with the number of
communication channels; thus the processing of RMA operations might not
be conducted by the same cores or same number of cores (e.g., on BG/Q,
should this be handled by the 17th core?).

To compete with PGAS languages, the code paths must be very short, with
optimizations for the latency-bound cases.

More specifically, here are a list of items to consider:

1.  Design for put/get/accumulate on local memory
2.  Design to allow datatype transport as a stand along module,
    particularly to enable scatter/gather support in hardware
3.  Design to allow MPI_Win to used shared memory on an SMP node (or
    chip)
4.  Allow debug support to check memory bounds
5.  Coordinate with the use of shared memory support in the
    implementation of the MPI collectives
6.  In lazy mode, the queued data should be formatted as communication
    packets to avoid unnecessary copies
7.  Support info arguments to control algorithms, such as the eager/lazy
    sync choices.
8.  Limit the number of unsatisfied requests (another info parameter) -
    don't assume that a zillion requests can be outstanding.
9.  Include instrumentation as part of design rather than retrofitting
    as it is now. Include number of pending requests, time waiting to
    sync
10. MPI_Win storage - if all values (e.g., displacement sizes) for all
    windows are the same, store a single value. If mostly the same,
    compress. Also consider carefully which items are needed locally.
    Provide for caching some data and acquiring as needed, particularly
    for cases where each item is likely to be different (e.g., base
    window address except in the symmetric allocation case).

## Optimizations

In current implementation of MPICH, the origin process locally queues up
all RMA operations and issues them to the target in 

- `MPI_WIN_FLUSH`
- `MPI_WIN_FLUSH_LOCAL`
- `MPI_WIN_FLUSH_ALL`
- `MPI_WIN_FLUSH_LOCAL_ALL`
- `MPI_WIN_UNLOCK`

There are two main optimizations in passive target RMA:

1.  Single operation optimization. If only one RMA operation is queued
    in operation list, origin will combine `LOCK` message, data message
    and `UNLOCK` message together into one packet and send out.
2.  Eliminate the acknowledgement message in some cases. For
    `MPI_WIN_FLUSH/MPI_WIN_FLUSH_ALL/MPI_WIN_UNLOCK`, after all
    operations are issued out, the origin process needs to poke the
    progress engine to wait for the acknowledgement message from the
    target process to ensure that all RMA operations are completed
    remotely before routine is returned. However, MPICH implementation
    distinguishes the following three situations in which the
    acknowledgment message can be eliminated or combined with other
    messages:
    1.  Current lock is EXCLUSIVE and the origin process has already
        acquired the lock. In this situation, unless the origin process
        unlocks the target, there is no third-party communication
        happens concurrently at the target, therefore the origin process
        does not need to wait for the remote completion. Note that for
        single operation optimization, origin process always waits for
        the remote completion even it is `EXCLUSIVE` lock, because the lock is
        not acquired when sending out the message.
    2.  The operation list is empty and the origin is in `MPI_WIN_FLUSH`
        or `MPI_WIN_FLUSH_ALL`. In this situation, there is truly no
        RMA operation issued before, therefore the origin does not need
        to care about remote completion. Note that in current MPICH, if
        the operation list is empty and the origin process is in
        `MPI_WIN_UNLOCK`, the acknowledgement is also eliminated. This
        is just because `MPI_WIN_FLUSH` and `MPI_WIN_FLUSH_LOCAL` are
        currently implemented in the same way, in other words, both of
        them guarantee the remote completion, therefore `MPI_WIN_UNLOCK`
        does not wait for the remote completion. In a better
        implementation, `MPI_WIN_FLUSH_LOCAL` should only guarantee the
        local completion, and `MPI_WIN_UNLOCK` should wait for the
        remote completion if only `MPI_WIN_FLUSH_LOCAL` happens before.
    3.  At least one GET operation can be found in the operation list.
        In this situation, MPICH will reorder that `GET` operation to the
        tail of the list, so that the acknowledgement can be attached to
        the returning data of that `GET` operation. Note that
        `GET_ACCUMULATE`, `FETCH_AND_OP`, and `COMPARE_AND_SWAP` cannot be
        used for this purpose because they together with `ACCUMULATE` are
        ordered. Also note that in this case, the acknowledgement
        message is not eliminated, but just can be piggyback with `GET`
        response packet.
