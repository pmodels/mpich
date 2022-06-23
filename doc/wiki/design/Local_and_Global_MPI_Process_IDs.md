# Local And Global MPI Process IDs

For the purposes of identifying other processes in an MPI job, each MPI
process maintains a notion of a "local" and a "global" process id. These
process IDs are not related to any process ids managed by the underlying
operating system. Instead, they are used to manage the relationship
between ranks in groups (and thus in communicators) and processes.

Local pids are used for all group operations and for identifying a
particular remote process from a process. Different processes may use
different local pids to refer to the *same* remote process; local
processes only have meaning to the process that is using them. This is
the sense in which they are local; the specific values are meaningful
only on the process that has defined them.

Global pids are used in the few cases where processes must exchange
information about processes. This occurs in the implementation of the
routine `MPI_Intercomm_create`. An example of the use of these is
presented below.

There is one additional important property of local pids. The local pids
for the processes in `MPI_COMM_WORLD` have the same values as the ranks
of those processes. In other words, in an MPI-1 (or MPI-2 without
dynamic processes), lpids are the same as rank in `MPI_COMM_WORLD`. This
simplifies group operations for the typical MPI application.


## Illustration of Local and Global PIDs

Consider an MPI job with 2 processes. Because of the way the lpids are
allocated for `MPI_COMM_WORLD`, each process starts with the same set of
lpids. For the purposes of this illustration, lpid\[indx\]-\>Pi will
mean that the lpid indx refers to process Pi.

```
P0:
lpid[0]->P0
lpid[1]->P1

P1:
lpid[0]->P0
lpid[1]->P1
```

Now, let P1 spawn another process (using `MPI_COMM_SELF`); we'll call
this process P2. P1's lpid table gets a new entry

```
P1:
lpid[0]->P0
lpid[1]->P1
lpid[2]->P2
```

and P2's lpid gets the same processes, but in a different order:

```
P2:
lpid[0]->P2
lpid[1]->P1
lpid[2]->P0
```

This order is because the first values are for the `MPI_COMM_WORLD`; in
this case, P2 is the only process in the new comm world. The second
entry is the connection to the spawning process, and the other entries
describe the other processes that are connected to P2 in the MPI sense
(e.g., all of the processes known to P1). In fact, another, probably
better and valid ordering of the lpids on process P2 would concatenate
the valid lpids from the spawning process after the new comm world:

```
P2 (alternate)
lpid[0]->P2
lpid[1]->P0
lpid[2]->P1
```

Note that P0 still has only 2 lpids, because it was not involved in the
creation of P2. Note also that the same lpid value may refer to
different processes.

If there is an intercomm create operation that involves P0 as part of
the local group and P2 as the remote group, P0 will then need to add the
lpid for P2:

```
P0:
lpid[0]->P0
lpid[1]->P1
lpid[2]->P2
```

The values "P0", "P1", etc are (one possible) global process id: the
value clearly identifies a particular process, and any process may use
that value to refer to the same process (unlike the lpids, where an lpid
of 0 on P0 refers to a different process than an lpid of 0 on P2).

Because of the way in which processes are allocated in MPI, one of the
simplest global process ids is a pair, containing (comm_world \#)(rank
in that comm_world). I.e., every time a comm world is created, give it a
new integer value (typically, increment a counter). This works whenever
a single process manager handles all processes; in the case of multiple
process managers, ranges of values could be assigned (e.g., the upper 8
bits would allow 256 distinct process managers, each with 8 million
separate comm_worlds, each of which could have up to 4 billion
processes). In this scheme, we can refer to the above processes as

```
P0 -> (0,0)
P1 -> (0,1)
P2 -> (1,0)
```

## API for Manipulating Local and Global PIDs

We assume that when processes are created, they are described by two
numbers: `(process-group-id)(rank-in-group)`. There may be a mapping
from `(process-group-id)` to something specific in the process manager,
such as a string, but every member of the same process group will have
the same id. Get the gpid corresponding to the rank of a process in a
communicator. The routine `MPID_GPID_GetAllInComm` provides a single
routine to efficiently access of of the gpids in a single communicator.
In addition, if all of these gpids are in the same process group,
`singlePG` is set to true, otherwise it is set to false.

```
MPID_GPID_Get( MPID_Comm *comm, int rank,  int gpid[] )
MPID_GPID_GetAllInComm( MPID_Comm *comm, int size,
                        int gpid[], int *singlePG )
```

Given a gpid, get the corresponding lpid. As in the preceding case,
there is a single routine to handle an entire array of gpids.

```
MPID_GPID_ToLpid( int gpid[2], int *lpid )
MPID_GPID_ToLpidArray( int size, int gpid[], int lpid[] )
```

In addition, it is useful to have a way to get the lpids from a
communicator:

```
MPID_LPID_GetAllInComm( MPID_Comm *comm, int size, int lpids[] )
```

In the above routines, global pids are stored as a pair of integers,
with the first integer being the process group id and the second integer
is the rank in that process group.

## Initializing the Connection Information in a New Communicator

When creating a new intercommunicator from within
`MPI_Intercomm_create`, the "connection table" that the device uses to
relate ranks in the communicator with remote processes must be set up.
The following routine takes a pointer to a communicator structure and
sets up this connection table. The local process pids identify the remote
group.

```
int MPID_VCR_CommFromLpids( MPID_Comm *commptr, int size, const int lpids[] )
```

## Implementation of MPI_Intercomm_create

The API described above may be use to implement `MPI_Intercomm_create`.

1.  Leaders call `MPID_GPID_GetAllInComm` (`singlePG` is not 0 if all
    processes are in a single group, in which case that's the process
    group).
2.  Leaders exchange these integer arrays
3.  Leaders use `MPID_GPID_ToLpidArray` to
    1.  check for non-unique members
    2.  form the new connection table (uses lpids)
4.  Leaders broadcast the GPIDs to the other members of the
    communicator, where those processes also use `MPID_GPID_ToLpidArray`
    to form the new connection table.
5.  If necessary, let the device distribute connection information to
    the other processes in the local communicator (see below).

These steps allow the processes to exchange information that identify
the processes.

## Updating Connection Information

In addition to these steps, a device may require an additional step to
exchange information that is needed to establish connections among the
processes. For example, at this writing, the ch3 device makes use of a
"business card cache" to handle connection information for processes
that are not members of the `MPI_COMM_WORLD` of that process. These
"device hooks" provide a way for the device to insert device-specific
operations. The implementation of `MPI_Intercomm_create` calls the
following hook if it is defined:

```
MPID_ICCREATE_REMOTECOMM_HOOK( MPID_Comm *peer_comm, MPID_Comm *peer_comm,
                               int remote_size, const int remote_gpids[],
                               int local_leader );
```

In a device that doesn't have dynamic processes or that has the "second
generation" PMI interface (no business card cache), these do nothing. In
the business card cache case, this could do the following:

1.  leaders exchange the full contents of the union of all the business
    card caches in the local_comm. They can start by exchanging the
    union of process ids; if they have the same process ids, they can
    don't need to do any more.
2.  leaders broadcast the business card information that they received
    from the remote leader to their local comm.

This operation is a hook because it is used to help the device, not
provide any specific operation.
