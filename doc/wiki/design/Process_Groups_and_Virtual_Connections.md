# Process Groups And Virtual Connections

The MPICH implementation uses virtual connections to describe a
communication path between two processes. Process groups describe the
collection of processes known to an MPI process.

## Overview

Communication in MPI is between processes. In MPICH, connections between
processes are described by virtual connections; in many cases, these
connections are not established until communication along that
connection is required (this is a more scalable approach). These virtual
connections are associated with a single process group. Virtual
connections are identified by their process group and a rank in that
process group. Each process group represents (all of the processes in)
an `MPI_COMM_WORLD`.

In the simplest case, corresponding to MPI-1, there is a single process
group that describes `MPI_COMM_WORLD`, and all processes belong to this
process group. In the more general case (described below), there are
multiple process groups. Each represents some `MPI_COMM_WORLD`. Process
groups in the ADI are not directly related to MPI groups. In particular,
in the MPI-1 case, there is only one process group (the group
corresponding to `MPI_COMM_WORLD`).

MPI communicators contain (for each of their process "groups") a
"virtual connection reference table" (VCRT), which is basically a
structure than contains an int number of virtual connections and a
pointer to an array of virtual connections, indexed by rank in that
communicator. There is also a reference count, allowing `MPI_Comm_dup`
to make a shallow copy (that is, refer to the VCRT rather than
duplicating it). If a new communicator is not a dup of an existing
communicator, then a new VCRT is created.

Each VCRT contains a number of references (pointers) to virtual
connections. Each virtual connection belongs to one and only one process
group (and `MPIDI_PG_t` structure). For highly-scalable systems such as
IBM's BG/L, the simplest implementation approach, an array of
connections, is not scalable or space-efficient, particularly for
`MPI_COMM_WORLD` and communicators that are dups of `MPI_COMM_WORLD`. A
different approach is needed, such as a special case for
`MPI_COMM_WORLD` and a hash or sparse array for active connections.

## Managing Dynamic Process Groups

The dynamic process features in MPI-2 make it important to know when a
particular process no longer has any connection to processes in other
process groups (i.e., groups other than that process's `COMM_WORLD`).
This is particularly necessary for the implementation of
`MPI_COMM_DISCONNECT`. To implement `MPI_COMM_DISCONNECT`, we need to
know when a process group is no longer connected; that is, there is no
communicator that is referencing any of the virtual connections in the
process group. (The definition of `MPI_COMM_DISCONNECT` is actually more
restrictive than this, and we'll see where that comes in below.) To know
this, we keep reference counts on each of these objects. The following
table indicates the actions that change the reference count for each
object:

| Object | Incr Ref                      | Decr Ref           | Action on 0 ref |
| ------ | ----------------------------- | ------------------ | --------------- |
| Comm   | Nonblocking comm              | Completion of same | decr VCRT       |
| VCRT   | Add to comm                   | When comm freed    | decr VC         |
| VC     | Add to VCRT                   | When VCRT freed    | decr PG         |
| PG     | VC goes from 0 to nonzero ref | VC goes to 0 ref   | free PG         |

MPI-2 says that `MPI_COMM_FREE` will not disconnect processes but
`MPI_COMM_DISCONNECT` will. We implement this by setting the reference
count of a VC to 2 when it is initialized, rather than 1.
`MPI_COMM_DISCONNECT` will check the reference count, and if the value
is 1 (after the usual release), it will decrement it once more, then
decrement the process groups' reference count. This isn't quite as
strong as is needed for the MPI-2 rules (see page 106, section 5.5.4),
but should meet the needs of the typical client-server applications.

Note that the reference counts of the VCs start at zero, not at 1 (or
two). The reason for this is to support connect/accept, followed by
disconnect. If the connect/accept makes use of communicators other than
`MPI_COMM_WORLD` (e.g., `MPI_COMM_SELF`), then an `MPI_COMM_DISCONNECT`
of the intercommunicator returned by
`MPI_COMM_CONNECT/MPI_COMM_ACCEPT` needs to completely free the
process group. However, during the `MPI_COMM_CONNECT/MPI_COMM_ACCEPT`
setup, each of the processes participating in the connect/accept will
need to create process groups for the other's `MPI_COMM_WORLD`s (if they
are, in fact, different). Thus, not all VCs in the "other" process
groups will be referred to by any communicator, and their reference
counts must be zero. (The process group must contain the information on
these VCs in case the programmer uses the intercommm from connect/accept
as the "peer" communicator in an `MPI_INTERCOMM_MERGE` involving each
processes `MPI_COMM_WORLD`.)

There is one additional complication that we may leave until later. In
order to ensure that all connected processes can be accessed (e.g., by
the `MPI_INTERCOMM_MERGE` step mentioned above), when two processes
connect, they will need to exchange information on all processes groups
to which they are connected. The question is then: how do we decide when
we are disconnected from those process groups? The answer is to keep
track of which process groups are "connected" in the MPI sense to others
and allow an `MPI_COMM_DISCONNECT` operation to act on them as well. A
simpler (not entirely correct but close enough for most uses) strategy
is to simply keep track of the which process groups have any active VCs;
on an `MPI_Finalize` or Abort, those process groups are ignored.

The communication routines refer to the virtual connections; they do not
refer to the process groups nor to the VCRTs. This suggests that the
VCRT and Process Group operations be managed together, with all of the
routines in a single file. The VCs are exported, as they are used in the
communication routines and because communcation-method-specific
information may be included in their definitions (more on this below).

### Old text on connection reference count

The key issues are (a) when is a process group disconnected (and hence
can be freed) and (b) how should the connection information (typically
the KVS space) be provided?

We define a process group as disconnected when none of the virtual
connections is referenced by a communicator and any virtual connection
that was used was freed with `MPI_COMM_DISCONNECT` (this matches the MPI
definition). We let the reference count on a process group be the number
of distinct virtual connections that are in use in a communicator.

Thus, there are the following rules for changing a reference to a
virtual connection:

1.  If the reference count of a virtual connection in a process group
    goes from 0 to 1, increment the reference count of the process
    group.
2.  If the reference count of a virtual connection in a process group
    goes to 0, decrement the reference count of the process group. If
    the reference count of the process group is now zero, destroy the
    process group.
3.  Note that creating a reference to a virtual connection in a new
    communicator from an old communicator cannot change the reference
    count in the process group.
4.  Note that the virtual connections for the `MPI_COMM_WORLD` to which
    the process belongs always have a reference count of at least one,
    since `MPI_COMM_WORLD` cannot be deleted.

Note that the initial reference count on a process group may be smaller
than the size of the process group, since the process group may be
larger than the communicator that is being constructed with that process
group (think of the spawned process when the parent uses `MPI_COMM_SELF`
in the `MPI_Comm_spawn` call).

## Managing the Virtual Connections

To instantiate a virtual connection, the implementation can use the
"KVS" (Key-value space) that the process manager associates with this
process group to retrieve connection information. For example, in the
CH3 implementation of the ADI interface, each process inserts into the
KVS space a "business card" that contains connection information (often
a IP address and port for socket communication; it could be a shared
memory segment id for shared-memory communication). Because the calling
process is a member of the same process group, it can access this
information with `PMI_KVS_Get`.

When dynamic processes are added, the situation is more complex. The
result of an `MPI_Comm_spawn`, for example, connects the calling process
to processes in another process group. There are several properties of
these process groups:

1.  Each process group represents an `MPI_COMM_WORLD`.
2.  With the exception of the process group to which a process belongs
    (the process group representing the `MPI_COMM_WORLD` to which that
    process belongs), process groups may come and go (become
    "disconnected" in MPI-terms).
3.  The PMI KVS space is only available on the processes' own process
    group.

For the management of connection information, there are really two
cases:

1.  The process group of this process's ``MPI_COMM_WORLD``. In this case,
    the `PMI_KVS_Get` routine is used. (This is not an ADI requirement
    but it is a CH3 requirement).
2.  All other process groups. In this case, `PMI_KVS_Get` can not be
    used. Instead, a copy of all connection information is shipped with
    the description of the process group.

Providing and accessing the connection information is accomplished with
the following routines (in `ch3/src/mpidi_pg.c`):

- `MPIDI_PG_SetConnInfo` - This is a collective call over the processes
  in `MPI_COMM_WORLD` that takes the rank of the calling process and its
  connection information, and makes it available for the other processes.
  It is implemented using the PMI_KVS_Put and PMI_Barrier calls.
- `MPIDI_PG_GetConnString` - This is an independent (i.e., not collective)
  call that returns the connection string for the process identified by a
  process group and a rank in that process group. This may use `PMI_KVS_Get`
  for processes in the same `MPI_COMM_WORLD` or another mechanism for
  processes not in the same `MPI_COMM_WORLD` (specifically, a cache of
  connection names that is maintained as part of the process group
  structure, at least until PMI is enhanced).

## Initializing a Virtual Connection

A virtual connection contains data used by both the ch3 common code and
by the particular communication channel. Thus, both ch3 and the channel
must participate in initializing a VC. Similarly, when a VC is freed,
the channel may wish to perform some additional operations (such as
freeing resources or closing sockets). To support this, the following
routines are defined:

- `MPIDI_VC_Init` - The common (ch3) initialization. This includes such fields
  as the `sendq`, which the common code checks to see if a connection has
  pending send operations.
- `MPIDI_CH3_VC_Init` - The channel-specific initialization
- `MPIDI_CH3_VC_Free` - The channel-specific routine to free a VC

In addition, when a VC is created (always as part of a process group),
certain fields may need to be initialized (to support the
`MPIDI_VC_Init` and `MPIDI_CH3_VC_Init` routines); this action is
performed with `MPIDI_VC_Create`. old text below here Virtual
Connections and Reference Counts There are two ways to change the
reference count for a virtual connection (VC). One is for the VC to
become part of a communicator (even if it is not active. In fact, this
reference count is changed only when a new VCRT (VC reference table) is
created; thus, an `MPI_Comm_dup` may not change the reference count for
each VC; instead, the reference count for the VCRT is incremented.
(Note, it isn't clear whether the current code does this.)

The other way that should (but does not as of February 25, 2006) change
the reference count is the initiation of a non-blocking operation, such
as a connection or close event.

Note that with one (possible) exception, all virtual connections belong
to some VCRT. The one (possible) exception is the VC that is used during
the connect/accept process (this is also used within the implementation
of Spawn).

## Scalability Comments

The current approach to managing process group descriptions and
connection information is clearly not scalable. However, a
generalization of the approach in this section, replacing specific data
structures with more dynamic access of the information, provides a path
to a scalable implementation.

## Virtual Connection Interface

Here are the basics:

  - `MPID_VCR` is a pointer to `struct MPIDI_VC`
  - `MPID_VCRT` is a pointer to `struct MPIDI_VCRT`

(note the inconsistent use of reference (R) in the names.) Recall that
an `MPIDI_VC` is a single virtual connection while a `MPIDI_VCRT` is a
table of connections. Tables and connections are each reference counted;
tables are used within communicators, and connections are used during
communication.

The state of the code, as of release 1.0.3, is roughly this. There are a
number of routines that are used to initialize a VC:

```
MPIDI_CH3_InitSock
MPIDI_CH3_VC_Init
MPIDI_VC_Init (ch3_progress)
```

In addition, routines that modify VC are in `ch3u_port`. Routines in the
file `mpidi_pg` allocates a vct and inits the members with
`MPIDI_VC_Init`. `mpid_finalize.c` knows too much about virtual
connections. In addition, many of the routines are in `mpid_vc`, of
course. `util/sock/ch3u_connect_sock.c` allocates vc structures (note
that one of these might be temporary, used to exchange setup
information) `util/sock/ch3u_init_sock` defines `MPIDI_VC_InitSock`.

A grep of `MPID_VC*` finds matches in these files:

- `mpi/comm/commutil.c`
- `comm_create.c`
- `comm_group.c`
- `comm_remote_group`
- `comm_split`
- `intercomm_create`
- `intercomm_merge`

The items matched include:

```
MPID_VCRT_Add_ref (commutil)
MPID_VCRT_Release
MPID_VCR_Get_lpid - add to VC accessor functions
MPID_VCRT_Create, MPID_VCRT_Get_ptr (?)
MPID_VCR_Dup
MPIDI_Comm_get_vc( comm, rank, &vc )
```

as well as routines in the CH3 ADI: `mpid/ch3/src/ch3u_port.c`,
`mpid_init` (`VCRT_Create`, `Get_ptr`)

## Thoughts on a New Design

Use routines whose names and semantics follow MPI objects, particularly
create, free:

```
MPIDI_VC_Create
MPIDI_VC_Free
MPIDI_VC_Dup
MPIDI_VCT_DupFromSubset
```

VC's, once initialized as part of the PG, should have function pointers
for operations, certainly for all low-performance-demanding operations
(such as connection/disconnection/delete/init).

The `VC_Create` routine is \*only\* called when a process group is
created (possible exception - a special VC used in connect/accept,
though that should simply belong to the created new process group). It
should call the channel-provided routine that will set the function
pointers - could in fact have one more level of indirection - into method
connections (vc may have a pointer to a struct of per-method functions
for managing connections).

(Don't do this until we are sure that we need this generality. May be
needed when going to ssm).

Steps:

1.  debug print stuff should be restricted to `mpidi_vc.c`, not in some
    other file (currently in mpidi_printf)
2.  Remove use of `MPID_VCRT_Add_ref` and change Release to Free. (used
    in `commutil.c`, `ch3u_port.c`)? Use this to solve the vc/vct issue?
    Does a dup of vcrt incr ref of each vc, or not? Still better
    (probably) to make this a VCRT_Dup rather than add_ref.

Here's a use in `comm_split` of the VCs:

```
MPID_VCRT_Create( new_size, &newcomm_ptr->vcrt );
MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
for (i=0; ivcr[keytable[i].color], &newcomm_ptr->vcr[i] );
    if (keytable[i].color == comm_ptr->rank) {
        newcomm_ptr->rank = i;
    }
}
```

This should have a more scalable definition.

Here's an example from `mpid/ch3/ch3u_port.c`, where the the new
intercomm is being initialized. This should probably change to a
`MPIDI_PG_Dup_vcr`( `pg, rank, &vc` ) that handles the case where the vc
ref count is 0 (thus requiring an incr to the pg ref count).

```
for (i=0; i < intercomm->remote_size; i++)
{
    MPIDI_VC_t *vc;
    MPIDI_PG_Get_vcr(remote_pg[remote_translation[i].pg_index],
                     remote_translation[i].pg_rank, &vc);
    MPID_VCR_Dup(vc, &intercomm->vcr[i]);
}
```

## Historical Comments

Through release 1.0.3, the process groups, virtual connections, and KVS
spaces were managed as loosely associated entities. For example, the KVS
information was managed in a separate cache, unrelated to the process
groups. Thus, should a process group become unconnected, the KVS
information remained (it could be identified and freed with the
associated kvsname, though this was not implemented). As another
example, the reference counts on process groups were not handled
systematically, leading to errors in deciding when a process group
should be freed. Solving these problems led to the organization
described in this note.
