# PMI-2 API

This is a draft of a design of version 2 of the Process Management
Interface API used within MPICH. See PMI Version 2 Design Thoughts for
background on the issues and requirements behind this design.

## Basic Concepts

The basic idea is that all interaction with external process and
resource managers, as well as the exchange of any information required
to contact other processes in the same parallel job, takes place through
the process management interface or PMI.

There are four separate sets of functionalities:

1.  Creating, connecting with, and exiting parallel jobs
2.  Accessing information about the parallel job or the node on which a
    process is running
3.  Exchanging information used to connect processes together
4.  Exchanging information related to the MPI Name publishing interface

While these can be combined within a single, full-featured process
manager, in many cases, each set of services may be provided by a
different actor. For example, creating processes may be managed by a
system such as PBS or LoadLeveler. The Name publishing service may be
accomplished by reading and writing files in a shared directory.
Information about the parallel job and the node may be provided by
mpiexec, and the connection information may be handled with a scalable,
distributed tuple-space system.

There are three groupings of processes that are important in
understanding the process manager interface.

  - Process
    An MPI process; this is usually an OS process (but need not be; an
    example would be threads in a language that keep named globals
    thread-private by default).
  - Job
    This is a collection of processes managed together by a process
    manager that understands parallel applications. A job contains all
    of the processes in a single MPI_COMM_WORLD and no more. That is,
    two processes are in the same job if and only if they are in the
    same MPI_COMM_WORLD
  - Connected Jobs
    This is a collection of jobs that have established a connection
    through the use of PMI_Job_Spawn or PMI_Job_Connect. If any
    process in a job establishes a connection with any process in
    another job, then all processes in both jobs are connected. That is,
    connections are established between jobs, not processes. This is
    necessary to implement the MPI notion of connected processes.

In addition, it is desirable to allow the PMI client interface to be
implemented with a dynamically loadable library. This allows an
executable to load a version of PMI that is compatible with whatever
process management system will be running the application, without
requiring the process management systems to implement the same
communication (or *wire*) protocol. The consequence of this is that the
`pmi.h` header file is standardized across all PMI client
implementations (in PMI v1, each PMI client implementation could, like
MPI, define its own header file).

### Character Sets

The PMI interface represents most data as printable characters rather
than as raw binary. This simplifies support for systems with
heterogeneous data representations and also simplifies the "wire"
protocol. The character set for PMI v2 is
[UTF-8](http://en.wikipedia.org/wiki/UTF-8); this is a variable-length
representation that contains ASCII as a subset and for which the null
byte is always a string terminator. All character data in the PMI v2
interface is in the UTF-8 character set.

## Changes from PMI version 1

The changes from PMI version 1 that are visible in the client API are
primarily in the handling of information about the Job (node and job
attributes) and in a way to share connection information, which is
stored in a KVS space, without forcing the MPI implementation to read
and broadcast the full contents of the KVS space when MPI jobs are
either created with `MPI_Comm_spawn` or `MPI_Comm_spawn_multiple` or
connected with `MPI_Comm_connect`, `MPI_Comm_accept`, or `MPI_Join`. In
addition, PMI version 2 has simplified the KVS API to support only a
single KVS space per parallel Job. The character set has been expanded
from ASCII to UTF-8. Finally, to aid in the use of dynamically-loaded
libraries, various sizes have been fixed as part of an ABI (application
binary interface).

## PMI Version 2 Client API

The PMI client API is described in the following sections, organized by
service.

### Initializing, Finalizing, and Aborting PMI

```
int PMI2_Init( int *spawned, int *size, int *rank, int *appnum, int *version, int *subversion )
```

Initialize PMI. This call must be made before any other PMI call (except
for PMI_Initialized). It returns whether the process was spawned with
PMI_Job_Spawn, the size of MPI_COMM_WORLD for this process and the
rank in MPI_COMM_WORLD. It returns MPI_SUCCESS on success and an MPI
error code on failure.

Special note: in the "singleton" init case, this routine returns spawned
= false, size = 1, rank = 0, and appnum = 0. This allows single process
jobs that do not use the dynamic process features of MPI-2 to run
without needing to connect to a process manager.

Security: It is the responsibility of the *implementation* of the PMI
API to provide for any necessary security in the connection between the
PMI client and PMI server. For example, an implementation may choose to
use SSL for a socket connection. A different implementation may choose
to use regular sockets, but provide its own security. Yet another
implementation may not use sockets at all, and rely on direct remote
memory operations. Because security is entirely the responsibility of
the implementation, no provision has been made in `PMI_Init` to pass in
any information in support of security. If the PMI client implementation
wishes to provide options, it should take those through some outside
mechanism, such as environment variables. This is also a pragmatic
choice; there is no way for the user to provide such information through
the MPI interface (e.g., no *Info* argument to `MPI_Init`).

The `version` and `subversion` arguments contain the version of
subversion of the process manager to which `PMI2_Init` has connected.
For an PMI2 process manager, the version must have the value `2`. If the
value is less than two (i.e., the process manager provides a PMI v1
process manager interface), the values of `version` and `subversion`
alone will be set and the routine will return an error indicating
failure in connecting. The application is then free to attempt to
connect with `PMI_Init`, the PMI version 1 interface.

Implementation Notes: PMI_Init is called within MPI_Init or
MPI_Init_thread, and should not make calls to MPI functions. One
exception is for the MPI error class and code functions. PMI_Init may
use MPI_Add_error_class and MPI_Add_error_code.

A process manager must support being contacted by any version of
PMIx_Init. It is free to reject such a connection, or to reject but
provide the required version number. This allows applications to try
again with a different version of PMI (e.g., should there be a PMI
version 3, this requirement will allow applications to try with a
PMI3_Init, then a PMI2_Init). To further simplify working with
different versions of the PMI interface, the environment variable
`PMI_VERSION` should be set with the version preferred by the available
or selected process manager (if this environment variable is not set or
not available, it is up to the application, usually an MPI library, to
select a default version of the PMI API).

```
int PMI2_Finalize(void)
```

Finalize the use of PMI by the calling process.

```
int PMI2_Initialized(void)
```

Returns true if PMI has been initialized and false otherwise. If
PMI_Finalize has been called, it also returns false.

```
int PMI2_Abort(int flag, const char msg[] )
```

Tell PMI that one or more processes is aborting, and provide an optional
message. The flag is true if all processes in the job are aborting,
false if just the calling process is aborting. This will allow
fault-tolerant versions of MPICH to work with the process manager. `msg`
may contain a message string that the process manager may choose to
print. The string is null terminated and may contain newlines. The
maximum length of this string is `PMI_MAX_MSG_STRING`.

Note that this is a convenience function. The process manager must
handle the case where a process exits unexpectedly. This routine is
provided to help the process manager provide better information about
the cause of an error, and in particular, may allow the process manager
to identify the individual process that caused a job to abort.

### Creating and Connecting to Processes

```
int PMI2_Job_Spawn( int count, const char * cmds[], const char ** argvs[],
                    const int maxprocs[],
                    const int info_keyval_sizes[],
                    const MPID_Info *info_keyval_vectors[],
                    int preput_keyval_size,
                    const MPID_Info *preput_keyval_vector[],
                    char jobId[], int jobIdSize,
                    int errors[])
```

Request that the process manager spawn processes. Note that the
implementation of this call must not block any other threads; other
threads must be able to make PMI calls while one (or more) threads are
calling this routine.

Note that this operation *must not* block other threads in the same MPI
process.

```
PMI2_Job_GetId( char jobId[], int jobIdSize )
```

Return the job id for this process. The jobId is the same for all
processes in the same MPI_COMM_WORLD and it is unique across all
MPI_COMM_WORLDs. This id may contain contact information for the
process manager or local PMI server.

In the case of a singleton init, this call may need to establish a
connection with the process manager (see PMI_Init above).

```
PMI2_Job_Connect( const char jobId[], PMI2_Connect_comm_t *conn )
```

Connect to the parallel job with ID jobId. This just "registers" the
other parallel job as part of a parallel program, and is used in the
PMI_KVS_xxx routines (see below). This is not a collective call and
establishes a connection between all processes that are connected to the
calling processes (on the one side) and that are connected to the named
jobId on the other side. Processes that are already connected may call
this routine.

The type `PMI2_Connect_comm_t` is a structure with the following
definition:

```
typedef struct PMI2_Connect_comm {
     int (*read)( void *buf, int maxlen, void *ctx );
     int (*write)( const void *buf, int len, void *ctx );
     void *ctx;
     int  isMaster;
} PMI2_Connect_comm_t;
```

The members are defined as

  - `read` - Read from a connection to the leader of the job to which this
    process will be connecting. Returns 0 on success or an MPI error
    code on failure.
  - `write` - Write to a connection to the leader of the job to which this process
    will be connecting. Returns 0 on success or an MPI error code on
    failure.
  - `ctx` - An anonymous pointer to data that may be used by the read and write
    members.
  - `isMaster` - Indicates which process is the "master"; may have the values 1 (is
    the master), 0 (is not the master), or -1 (neither is designated as
    the master). The two processes must agree on which process is the
    master, or both must select -1 (neither is the master).

A typical implementation of these functions will use the read and write
calls on a pre-established file descriptor (fd) between the two leading
processes. This will be needed only if the PMI server cannot access the
KVS spaces of another job (this may happen, for example, if each mpiexec
creates the KVS spaces for the processes that it manages).

```
PMI2_Job_Disconnect( const char jobId[] )
```

Disconnect the parallel job with ID jobId from this process.

### Exchanging information

Processes in a parallel job often need a way to exchange information,
such as how to establish a connection to the remote process. These
routines establish a single "key-value" space associated with each
`MPI_COMM_WORLD`. One of these routines accepts a jobId; processes that
are connected may read the "key-value" space of another
`MPI_COMM_WORLD`.

```
PMI2_KVS_Put( const char key[], const char value[] )
```

Place a "key-value" entry into the space belonging to this
`MPI_COMM_WORLD`. If multiple PMI2_KVS_Put calls are made with the
same key between calls to PMI2_KVS_Fence, the behavior is undefined.
That is, the value returned by PMI2_KVS_Get for that key after the
PMI2_KVS_Fence is not defined.

```
PMI2_KVS_Fence(void)
```

Commit all PMI2_KVS_Put calls made before this fence. This is a
collective call across `MPI_COMM_WORLD`. It has semantics that are
similar to those for MPI_Win_fence and hence is most easily
implemented as a barrier across all of the processes in the job.
Specifically, all PMI2_KVS_Put operations performed by any process in
the same job must be visible to all processes (by using PMI2_KVS_Get)
after PMI2_KVS_Fence completes. However, a PMI implementation could
make this a lazy operation by not waiting for all processes to enter
their corresponding PMI_KVS_Fence until some process issues a
PMI2_KVS_Get. This might be appropriate for some wide-area
implementations.

```
PMI2_KVS_Get( const char jobId[], int src_pmi_id, const char key[], char value [], int maxValue, int *valLen )
```

Access a "key-value" entry. If jobId is not null, the KVS space of the
corresponding connected job is used.

Note that there is no way to delete a KVS key-value pair or a KVS space.
This is by design, as the needed functionality is quite small.

The "src_pmi_id" is a hint provided to the process manager for cases
where the process requesting for a key knows the PMI_ID of the process
that put the key. If this value is PMI2_ID_NULL, it is interpreted as
if the source PMI_ID is not known.

Most value fields will be small, but there is the possibility that a
value exceeds the provided length. The valLen field is always set with
the length of the returned value, or, if the length is longer than
maxValue, the negative of the required length is returned. Thus, if
there is any doubt that the value will fit in the space, the correct
usage is

```
err = PMI2_KVS_Get( jobId, key, value, maxValue, &valLen );
if (err) {
    /* process error */
}
else if (valLen < 0) {
    char *value2 = (char *)malloc( -valLen );
    if (!value2) { error... }
    err = PMI2_KVS_Get( jobId, key, value2, -valLen, &valLen2 );
    ...
}
```

The reason for this approach is because the PMI routines return MPI
error codes, and we want to permit both simple implementations
(`MPI_ERR_OTHER` returned for all errors) and more sophisticated ones
(MPI error codes, created with MPI_Add_error_code).

### Information about the node or job

It is often necessary to access information about the node on which the
processes are running or the parallel job. These routines allow the
MPICH program to that. The definition of "node" is up to the process
manager; it is valid (though not helpful) to view each process as
running on a different node. To simplify this interface, the maximum
size of a node or job attribute is given by PMI2_MAX_ATTRVALUE, which
is at least 1024.

These routines introduce a fourth grouping of processes: the **node**.
This model reflects the reality that all processors are now multicore,
and there are many operations that should be optimized among processes
within the same node. Typically, such nodes share memory and may consist
of one or more chips. A more general approach could define an arbitrary
number of such collections, but these two levels should be sufficient
for now.

```
PMI2_Info_GetNodeAttr( const char name[], char value[], int valueLen, int *flag, int waitFor )
```

Given an attribute name, return the corresponding value. The flag is
true if the attribute has a value and false otherwise. If the value of
waitFor is true, this routine will wait until the value becomes
available. This provides a way, when combined with
PMI2_Info_PutNodeAttr, for processes on the same node to share
information without requiring a more general barrier across the entire
job.

This routine provides an extensible way to retrieve information about
the node (such as an SMP node) on which the process is running by
agreeing on new names.

There are a few predefined names

  - `memPoolType` - If the process manager allocated a shared memory pool for the MPI
    processes in this job and on this node, return the type of that
    pool. Types include sysv, anonmmap and ntshm.

  - `memSYSVid` - Return the SYSV memory segment id if the memory pool type is sysv.
    Returned as a string.

  - `memAnonMMAPfd` - Return the FD of the anonymous mmap segment. The FD is returned as a
    string.

  - `memNTName` - Return the name of the Windows NT shared memory segment, file
    mapping object backed by system paging file. Returned as a string.

```
PMI2_Info_GetNodeAttrIntArray( const char name[], int array[], int arrayLen, int *outLen, int *flag, int waitFor )
```

This is like `PMI2_Info_GetNodeAttr`, except that the returned value is
an array of integers. The number of values returned is given in
`outLen`.

There are a few predefined names

  - `localRanksCount` - Return the number of local ranks that will be returned by the key
    `localRanks`.

  - `localRanks` - Return the ranks in MPI_COMM_WORLD of the processes that are
    running on this node.

  - `cartCoords` - Return the Cartesian coordinates of this process in the underlying
    network topology. The coordinates are indexed from zero. Value only
    if the Job attribute for physTopology includes cartesian.

```
PMI2_Info_PutNodeAttr( const char name[], const char value[] )
```

Set an attribute for the node. This call should be used sparingly, but
is available. For example, it might be used to share segment ids with
other processes on the same SMP node.

```
    PMI2_Info_GetJobAttr( const char name[], char value[], int valueLen, int *flag )
```

Get an attribute associated with the job. For example, the "universe
size" is one such attribute. In the singleton init case, this call may
need to establish a connection to the process manager. There is no
corresponding PutJobAttr.

  - `universeSize` - The size of the "universe" (defined for the MPI attribute
    `MPI_UNIVERSE_SIZE`
  - `hasNameServ` - The value hasNameServ is true if the PMI environment supports the
    name service operations (publish, lookup, and unpublish).
  - `physTopology` - Return the topology of the underlying network. The valid topology
    types include cartesian, hierarchical, complete, kautz, hypercube;
    additional types may be added as necessary. If the type is
    hierarchical, then additional attributes may be queried to determine
    the details of the topology. For example, a typical cluster has a
    hierarchical physical topology, consisting of two levels of complete
    networks - the switched Ethernet or Infiniband and the SMP nodes.
    Other systems, such as IBM BlueGene, have one level that is
    cartesian (and in virtual node mode, have a single-level physical
    topology).

Note that

  - `physTopologyLevels` - Return a string describing the topology type for each level of the
    underlying network. Only valid if the physTopology is hierarchical.
    The value is a comma-separated list of physical topology types
    (except for hierarchical). The levels are ordered starting at the
    top, with the network closest to the processes last. The lower level
    networks may connect only a subset of processes. For example, for a
    cartesian mesh of SMPs, the value is `cartesian,complete`. All
    processes are connected by the `cartesian` part of this, but for
    each `complete` network, only the processes on the same node are
    connected.
  - `cartDims` - Return a string of comma-separated values describing the dimensions
    of the Cartesian topology. This must be consistent with the value of
    `cartCoords` that may be returned by `PMI_Info_GetNodeAttrIntArray`.
    These job attributes are just a start, but they provide both an
    example of the sort of external data that is available through the
    PMI interface and how extensions can be added within the same API
    and wire protocol. For example, adding more complex network
    topologies requires only adding new keys, not new routines.
  - `isHeterogeneous` - The value isHeterogeneous is true if the processes belonging to the
    job are running on nodes with different underlying data models.

Example use of Job attributes

```
PMI2_Info_GetJobAttr( "physTopology", value, PMI2_MAX_ATTRVALUE, &flag );
if (!flag) {
    /* No topology information available, return */
    return 0;
}
if (strcmp( value, "hierarchical" ) == 0) {
    char *p=0;
    PMI2_Info_GetJobAttr( "phyTopologyLevels", value, PMI2_MAX_VALLEN, &flag );
    if (!flag) {
        /* Error, PMI should provide this info if physTopology is hierarchical */
    }
    /* Lets see if the bottom level is complete */
    p = strrchr( value, ',' );
    if (!p) { error, malformed value; }
    if ( strcmp( p + 1, "complete" ) == 0) {
        int mranks[128];
        PMI2_Info_GetNodeAttrIntArray( "localRanks", mranks, 128, &flag, 1 );
        if (flag) { remember local ranks for sub communicator processing }
    }
}
```

### Name Publishing

These routines support the MPI name publishing interface. They are very
similar to the PMI version 1 routines, though they have been renamed to
match the version 2 naming strategy, and they accept the info parameter.
Note that the name publishing interface is different than the KVS space
because it must be visible to all (qualified) processes, where the KVS
space is visible to just the connected process. As this feature is used
to allow MPI processes to connect to each other, it must allow
unconnected processes to access the data.

Name publishing is optional; a PMI server is not required to implement
these functions.

```
PMI2_Nameserv_publish( const char service_name[], const MPID_Info *info_ptr, const char port[] )
```

Publish the service_name as port.

```
PMI2_Nameserv_lookup( const char service_name[], const MPID_Info *info_ptr, char port[], int portLen )
```

Lookup the service_name and return the corresponding port

```
PMI2_Nameserv_unpublish( const char service_name[], const MPID_Info *info_ptr )
```
Delete the service_name from the name space

### Header File

The `pmi2.h` header file includes function prototypes plus the following
definitions:

```
#define PMI_VERSION 2
#define PMI_SUBVERSION 0
#define PMI2_MAX_KEYLEN 64
#define PMI2_MAX_VALLEN 1024
#define PMI2_MAX_ATTRVALUE 1024
#define PMI2_ID_NULL -1
```

## Discussion

### Thread Safety

PMI calls may block the calling thread until they complete. They must
not block other threads. In this way, they are similar to blocking MPI
calls. It is not necessary for the user (which means the MPI
implementation) to ensure that only one thread at a time makes a PMI
call (that was a restriction in PMI version 1).

### Dynamically Loaded Library Support

We want it to be easy to define a dynamically loadable PMI client
implementation that can be provided by a process management vendor. For
the most part, the PMI definition given here is easy to implement as a
dynamically loaded library. However, depending on the capabilities of
the dynamic loader, it may be desirable to export a table of functions
to the PMI client so that it can make use of routines provided by the
MPICH implementation. Such functions and global variables include:

  - MPIR_Err_create_code: Create an MPICH-style error code.
    MPIU_trmalloc, MPIU_trfree, MPIU_trcalloc:MPICH tracing memory
    allocators
    MPIU_DBG_Outevent,
    MPIU_DBG_ActiveClasses,MPIU_DBG_MaxLevel:Output a debugging
    "event"; these are used by the debug logging option

### Discussion Items

This interface eliminates all of the inquiry routines from version 1
that gave the maximum lengths of the various fields. Instead, these are
defined in the pmi.h file as fixed sized. In addition, pmi.h defines a
PMI version and subversion value.

Note that these routines take a length-of-character-array for all of the
output arrays so that the routines can make sure that the values are
never larger than the array. In the one case (PMI2_KVS_Get) where the
value may be of nearly arbitrary size, there is a way to find the size
of the value so that sufficient space may be allocated.

Also removed are the PMI error values. Instead, the PMI routines should
make use of the MPI error codes. Implementations should either use
MPI_Add_error_class or use MPI_ERR_OTHER for the error class for
PMI errors. Implementations are encouraged to create a new error class
and a number of error codes within that one class for PMI-specific error
reporting.

The routine PMI2_Info_GetNodeAttr provides a way to implement the
Intel MPI Library Extensions to PMI Protocol for identifying the ranks
on the node (the Intel document describes a change to the PMI wire
protocol; the API here provides a way to implement such extensions
without changing either the wire protocol (after this) or the use of PMI
within the MPICH code.

One of the routines defined in PMI version 1 that is not present in
version 2 is PMI_Barrier. The function of fencing off changes to the
KVS space is implemented (more clearly) with PMI2_KVS_Fence. Other
barrier operations should, if at all possible, be performed with the
MPICH device code, since the device may be better able to implement it.
Note also that PMI_Barrier was only collective over the job, not all of
the connected jobs, and hence can not be used as a barrier over all
connected processes.

### Implementation Suggestion

The following outlines a two-step process of moving to this version of
the PMI client interface.

Step 1. Implement PMI v2 in terms of the current PMI wire protocol. The
JobID is the KVS name (this is what is used right now). The augmented
KVS calls may just work with mpd (I don't know if mpd checks to see if a
process is accessing some other job's KVS space) and can be made to work
with the general utility functions used by gforker, remshell, and the
OSC mpiexec for PBS. Some features won't be supported with the v1 wire
protocol (particularly the requirements on spawn not blocking other
threads), but those are minor. The major change here is just to simplify
the MPICH code.

Step 2. Change the wire protocol. That needs to address the wire
protocol issues, including the security and thread safety issues, and it
can also simplify the processing code by removing minor variations in
command names (so that common processing code can be easily used).
Everyone will need to change their code.

If possible (with the agreement of our partners), we can drop support
for PMI version 1. However, we should make sure that the new code can
detect that it is talking to a process manager that is implementing the
old version and give a good (i.e., helpful) error message to the user.
We could provide backward compatibility, but I think we have
higher-priority tasks.

### Other Missing Pieces

Based on previous discussions, the following are some of the things that
are yet to be addressed:

  - PMI_Info arguments, that allows libraries to specify a key as
    "read-only" allowing for the process manager to optimize its
    distribution (using caching for example).
  - Error strings: Unless we plan to standardize the pmi.h header file,
    we will need a mechanism to query the PMI library for an error
    string given a PMI error code.
  - Universe size: PMI-2 missed out on the get_universe_size
    capability from PMI-1. Are we retaining the PMI-1 semantics for
    this?
  - Environment variables for constants: Values that do not change over
    the lifetime of the application (e.g., universe size) can be passed
    to the PMI library as environment variables instead of PMI keys,
    thus allowing for a slightly better performance.
  - More predefined keys including dead/alive processes and hardware
    (node/network) topology information need to be added.
