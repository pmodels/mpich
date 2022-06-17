# RMA Implementation

The MPICH ADI allows devices to provide customized support for the MPI
RMA operations; currently, all MPI RMA functions are included in the
ADI. The CH3 device provides a default implementation that relies only
on the CH3 (messaging) operations. In addition, the CH3 RMA
implementation contains multiple optimization to minimize the number of
messages used for synchronization.

## Abstract Device Interface

MPICH supports two definitions of the ADI for routines that take a
window argument. The selection between these mechanisms is controlled by
the `USE_MPID_RMA_TABLE` preprocessor macro, via the `MPIU_RMA_CALL`
macro. When `USE_MPID_RMA_TABLE` is not defined, `MPIU_RMA_CALL`
dispatches corresponding `MPID_*` functions for each RMA operation. This
is the approach used by several vendors, e.g. in the DCMF device.

When `USE_MPID_RMA_TABLE` is defined, the `MPID_Win` object will contain
an indirection table, called RMAFns, which contains a function pointer
for each operation. This is the mechanism used by CH3. By default, CH3
populates this table with corresponding MPIDI routines, and allows the
channel or netmod to override these defaults with a different
implementation.

### Window Creation Functions

In the ADI, window creation functions are all included as `MPID_*`
routines. CH3 adds a layer of indirection via the static
`MPIDI_CH3U_Win_fns` table. This table is populated when CH3 is
initialized, and implementations can be overridden in the channel or
device initialization code. As an example, CH3 defines a default
implementation of `MPI_Win_allocate_shared` (valid for `MPI_COMM_SELF`
only), which is overridden in this table by Nemesis.

## Handling of RMA Operations

One-sided communication operations are performed immediately on the
local process (this includes the lock operation) and on processes that
are accessible via shared memory windows. All other operations are
entered into the RMA operations queue. When active target is used,
synchronization state is maintained collectively and is the same at all
processes. Thus, a single queue is used. When passive target
synchronization is used, processes track the state of each target
individually and separate operations into per-target operation queues.

As of this writing, operations are queued until the ensuing
synchronization operation. Eager issue of RMA operations is something we
would like to implement in the future (see Bill Gropp's EuroMPI '12
paper). The current implementation supports eager request of the lock,
which is a first step in implementing this feature.

All one-sided operations are implemented using CH3 point-to-point
messaging and packet handlers. CH3 ensures ordered channels; this is
relied upon for ordering of accumulate operations and consistency of RMA
epochs.

## Synchronization Operations

Active target synchronization utilizes several optimizations to avoid
synchronization, discussed in "Optimizing the Synchronization Operations
in MPI One-Sided Communication."

Passive target synchronization uses the flags and window handle fields
in each RMA packet header to piggyback synchronization information.
Flush and unlock are piggybacked on any operation; pt (passive target)
done is piggybacked on gets and RMW operations; and lock is piggybacked
whenever `MODE_NOCHECK` is given. In the pre-MPI-3 design, the target
process would determine if an acknowledgement (pt done packet) should be
returned to the origin. In the new design, the ack-requested flag is set
by the origin and the target only looks at the flags to determine if an
ack is needed.
