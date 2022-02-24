# ADI RMA Interface

## Notes on the ADI RMA interface

The ADI interface for MPI RMA operations is much like the point-to-point
interface: the top-level routines validate arguments and convert handles
to poniters and then pass those values to ADI routines with names
beginning with MPID instead of MPI. For example, `MPI_Put` calls
`MPID_Put`; `MPI_Win_lock` calls `MPID_Win_lock`.

In some cases, the device may want to select the routines to use for RMA
operations on a `MPI_Win` basis.

If the device asserts

```
USE_MPID_RMA_TABLE
```

(by defining it in mpidpre.h), then MPICH will include a table of
function pointers in each `MPI_Win` object, and will invoke the
appropriate function through that pointer rather than calling the "MPID"
routine. In other words, for `MPI_Put`, if `USE_MPID_RMA_TABLE` has been
defined, then the "Put" entry in the function table will be invoked
instead of calling `MPID_Put`. The device need not provide `MPID_Put` (or
any of the other MPID versions of the RMA routines in that case), with
the exception of `MPID_Win_create`.

The `MPID_Win_create` function is required to completely initialize the
table; the MPI routines will not check that the table has been
initialized or that the each table entry is non-null.

### Motivation

One sided operations should be especially fast. Consider the case of a
system consisting of a cluster of SMP nodes; with each SMP containing
many processors. Current systems of this kind include Sun Enterprise
systems; as core counts go up, many commodity processor clusters will
involve nodes with 32 or more processors. An MPI implementation may
create an `MPI_Win` object with processes are are within a single SMP
node. In this case, the ADI can optimize for direct memory access. By
using a function table in the `MPI_Win` object, the implementation can
easily customize for this case without using a separate set of branches
within the ADI routines. Of course, the ADI routine itself could use the
function table (and a previous version of the ch3 ADI did that), but for
highest performance, it makes sense to eliminate as many function calls
as possible for operations like `PUT` and `GET`.

### Implementation

To allow the top-level routines (e.g., `MPI_Put`) to use either a
function table or simply call `MPID_xxx` (e.g., `MPID_Put`), the macro

```
MPIU_RMA_CALL(winptr,functionName(args))
```

is used. This converts into either

```
MPID_functionName(args)
```

if `USE_MPID_RMA_TABLE` has **not** been defined, or

```
win_ptr-\>rmaFns.functionName(args)
```

if `USE_MPID_RMA_TABLE` has been defined.

### Motivation for the Macro approach

If performance was not critical, there would be no reason to do anything
but use the function table. However, on some systems, the overhead can
be noticeable (though not significant), and the macro makes it easy to
provide for both approaches in the same source code.

### Notes on the old ch3 implementation

The ch3 implementation made use of a global function table to allow for
the `MPI_Win-within-an-SMP` case. Because it was a global table, each
`MPID_Win_create` took the table as an argument and could change it.
This wasn't correct, however, as the following example shows. Assume
that the initial state of the global table is to use a set of general
purpose routines, and there is an optimization for all-within-an-SMP.
Let the communicators `commOneSMP` be a communicator of processes within
an SMP and commManyNode be a communicator involving multiple SMPs.

Consider this code:

```
MPI_Win_create( ..., commManyNode, \&winMany );
MPI_Win_create( ..., commSMPNode, \&winSMP );
/* Changes the global table to support RMA within an SMP */

MPI_Put( ..., winMany );
/* Fails because the global function table points to functions that
only work within an SMP */
```

Clearly, the operations must be defined on a per-MPI Window basis, and
that is why the function table is part of the `MPID_Win` structure in the
current design.

In addition to the routines that worked on an MPI Window object, the
previous ch3 design had function table entries for `Alloc_mem` and
`Free_mem`. The corresponding MPI routines, `MPI_Alloc_mem` and
`MPI_Free_mem`, do not take `MPI_Win` objects. Thus, these cannot be
adapted to reflect a particular set of processes, and it does not make
sense to use a function pointer table with this routines. The ADI
routines for these two are `MPID_Alloc_mem` and `MPID_Free_mem`
respectively.

One additional difference between the old ch3 version and the current
version is that `Win_test` (for `MPID_Win_test`) is in the current
function table. The old ch3 version did not provide any way to select a
different implementation of this routine; the only implementation
invoked the progress engine and they checked the `MPID_Win` structure.

### Changes to the Channel Interface

The ch3 ADI channel interface included the routine

```
MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *ftable )
```

This routine was responsible for initializine the global function table,
if the device wanted to make any changes to that table. As pointed out
above, this is incorrect, as changing the global table could cause
failures.

Instead, we need an optional routine that can initialize the per-window
function table. If the channel asserts

```
USE_CHANNEL_RMA_TABLE
```

then the function

```
int MPIDI_CH3_RMAWinFnsInit( MPID_Win *win_ptr )
```

will be called.

The macro `USE_CHANNEL_RMA_TABLE` is used for backward compatibility -
channels that don't provide this initialization function will simply use
the ch3 default functions. As most channels currently define
`MPIDI_CH3_RMAFnsInit` as a function that returns `MPI_SUCCESS` but does
nothing else, this change does not affect many channels.

One exception is the dllchan - it must export the proper function in the
event that one of the channels provides a `MPIDI_CH3_RMAWinFnsInit`.
