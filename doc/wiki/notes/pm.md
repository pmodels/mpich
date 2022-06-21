# Process Management in MPICH

This page describes a process management interface that can be used by MPI
implementations and other parallel processing libraries, yet be independent of
both. We define the specific interface we are developing, called
PMI (Process Manager Interface) in the context of MPICH. We describe the
interface itself and a number of implementations. We show how an MPI
implementation built on MPICH can use PMI to make itself independent of the
environment in which it executes, and how a process management environment can
support MPI implementations based on MPICH.

## Introduction

This informal paper is intended to be useful for those using MPICH as the basis
of their own MPI implementations, or providing a process management environment
which will run MPI jobs that are linked against MPICH itself or an
MPICH-derived MPI implementation. At the time of writing, this audience
potentially includes groups at:
- IBM (for BG/L)
- Livermore (SLURM process management)
- Cray (MPI implementation for Red Storm, with YOD)
- Myricom (MPICH-GM)
- Viridian (PBSPro)
- Globus/Teragrid implementors (MPICH-G2)

Others are welcome to contribute suggestions.

This paper is incomplete in the sense that it describes an interface still
being defined. The part that is currently in use in the MPI-1 part of MPICH has
proved adequate for our needs and has several implementations.  We will refer
to it as `Part 1`. That part of the interface that is required for the dynamic
process management part of MPI-2 is still under development, and we will refer
to it here as `Part 2`. Most of this paper is about `Part 1`, which is all that
is necessary to support an MPI implementation that does not include dynamic
process management.

Below we will describe the problem, the approach we have taken so far in MPICH,
the PMI interface itself, it's implementation, and any implications for those
who are collaborating with us.


## The Problem

The problem this paper addresses is how to provide the processes of a parallel
job with the information they need in order to interact with the process
management environment where necessary, in particular to set up communication
with one another. In many cases such information is partly provided by the
systems software that actually starts processes, and also partly provided by
the processes themselves, in which case the process management system must aid
in the dissemination of this information to other processes. A classic example
occurs when a process in a TCP implementation of MPI acquires a port on which
it can be contacted, and then must notify other processes of this port so that
they can establish MPI communication with this process by connecting to
the port.

Traditionally, parallel programming libraries, such as MPI implementations,
have been integrated with process management mechanisms
(LAM, MPICH1 with `ch_p4` device, POE) in order to solve this problem. After a
preliminary exploration of separating the library from the process manager in
MPICH-1 with the `ch_p4mpd` device we have decided to update the interface and
then commit to this approach in MPICH. We are motivated by the challenges of
implementing MPI-2 in a system-independent way, but many of the ideas here
might prove useful in a non-MPI environment as well, either for other parallel
libraries (such as Global Arrays, GP-SHMEM, or GASNet) or language-based
systems (such as UPC, Co-Array Fortran, or Titanium).

The problem to be addressed has several components:

- Conveying to processes in a parallel job the information they need to
  establish communication with one another. To focus the discussion, we assume
  that such communication is needed for implementing MPI. Thus this information
  could include hosts, ports, interfaces, shared-memory keys, and other
  information.
- MPI-2 dynamic process management functions require extra support for
  implementing `MPI_Comm_Spawn`, `MPI_Comm_{Connect/Accept}`,
  `MPI_{Publish/Lookup}_name`, etc.
- The interface should be simple and straightforward, particularly in the
  absence of dynamic process management.  MPICH will implement dynamic process
  management, but some other MPI implementations may not.
- The interface must allow a scalable implementation for performance-critical
  operations. In environments with even only hundreds of processes, serial
  algorithms will be inappropriate.

An earlier, similar version of this approach was described,
[Components And Interfaces Of A Process Management System For Parallel Programs](https://www.sciencedirect.com/science/article/pii/S0167819101000977),
where the interface is called BNR, incorporated in the `ch_p4mpd` device in the
original MPICH. PMI represents an evolution of that interface and it's
implementation in MPICH.

Note that existing process managers often do a scalable job of starting
processes, and this part of existing systems can be kept. What is sometimes
lacking is a way of conveying the communication-establishment information.
Although the approach we take in the implementations below is to combine the
process startup and information exchange functionality in a single interface,
a different implementation could separate these, using an existing
process-startup mechanism and adding a new component to implement the other
parts of the interface. One approach along these lines is outlined in the
[adding](#adding)
section.

## Our Approach

The approach we have taken to the problem is to define a Process Management
Interface (PMI). The MPICH implementation of MPI will be implemented in terms
of PMI rather than in terms of any particular process management environment.
Multiple implementations of PMI will then be possible, independently of the
MPICH implementation of MPI. The key to a good design for PMI is to specify it
in a way that allows for scalable implementation without dictating any details
of the implementations. This has worked out well so far, and in the
[implementing](#implementing)
section we describe a number of quite different implementations of the
interfaces described in the 
[interface](#the-pmi-interface)
section.

## The PMI Interface

We present the interface in two parts. The first part is sufficient for the
implementation of MPI-1 and many parts of MPI-2. The second part is required
for implementing the dynamic process management part of MPI-2. MPICH is now
using the first part, with multiple PMI implementations, so we consider it
relatively final at this point. Since we have not yet implemented the dynamic
process management functions in MPICH, some evolution of Part 2 of PMI may take
place as we do so.

The fundamental idea of Part 1 is the *key-value space* (KVS), containing a set
of `(key, value)` pairs of strings. Processes acquire access to one or more
KVS's through PMI and can perform `put/get` operations on them. Synchronization
is defined in a scalable way via the barrier operation, so that processes can
be assure that the necessary puts have been done before attempting the
corresponding gets.

Thus the PMI interface (Part 1) consists of `put/get/barrier` operations
together with housekeeping operations for managing the KVS's. For
implementation of MPI-1, a single KVS, the default KVS for processes started at
the same time, is sufficient, but multiple KVS's will be useful when we
consider Part 2 and dynamic process management.

### Part 1:  Basic PMI Routines

Part 1 of the interface is invoked in performance-critical parts of the MPI
implementation, both during initialization and connection setup. Thus it is
critical that this part of the interface allow scalable implementation. We
accomplish this through the semantics of the `put/get/barrier`, since the only
synchronizing operation is the collective `barrier`, which is can have a
scalable implementation. The `commit` operation allows batching of `put`
operations for improved performance.

Part 1 has two subparts:  firstly, the functions associated with the process
group being started, and thus already implemented in some way in any MPI
implementation. Secondly, the functions associated with managing the keyval
spaces, used for communicating setup information. 

```
/* PMI Group functions */
int PMI_Init( int *spawned );  /* initialize PMI for this process group
                                  The value of spawned indicates whether
                                  this process was created by
                                  PMI_Spawn_multiple. */
int PMI_Initialized( void );   /* Return true if PMI has been initialized */
int PMI_Get_size( int *size ); /* get size of process group */
int PMI_Get_rank( int *rank ); /* get rank in process group */
int PMI_Barrier( void );       /* barrier across processes in process group */
int PMI_Finalize( void );      /* finalize PMI for this process group */

/* PMI Keyval Space functions */
int PMI_KVS_Get_my_name( char *kvsname );       /* get name of keyval space */
int PMI_KVS_Get_name_length_max( void );        /* maximum name size */
int PMI_KVS_Get_key_length_max( void );         /* maximum key size */
int PMI_KVS_Get_value_length_max( void );       /* maximum value size */
int PMI_KVS_Create( char *kvsname );            /* make a new one, get name */
int PMI_KVS_Destroy( const char *kvsname );     /* finish with one */
int PMI_KVS_Put( const char *kvsname, const char *key,
                const char *value);             /* put key and data */
int PMI_KVS_Commit( const char *kvsname );      /* block until all pending put
                                                   operations from this process
                                                   are complete.  This is a
                                                    process local operation. */
int PMI_KVS_Get( const char *kvsname, const char *key, char *value); 
                                /* get value associated with key */

int PMI_KVS_iter_first(const char *kvsname, char *key, char *val);
int PMI_KVS_iter_next(const char *kvsname, char *key, char *val);
                                /* loop through the pairs in the kvs */
                             
```

A scalable implementation of Part 1 of PMI could probably use existing software
for the group functions, and add some new functionality to support the
KVS-related functions. One possible implementation is suggested below in the
[adding](#adding)
section.

#### Notes
- The above routines (Part 1) are all that is needed for an implementation of
  MPI-1 and most of MPI-2. Part 2 is only needed to support the MPI functions
  defined in the Dynamic Process Management section of the MPI Standard.
- Similarly, multiple KVS's are only really needed in the dynamic process
  management case.  An initial implementation could omit `PMI_KVS_Create` and
  `PMI_KVS_Destroy`. The iterators `int PMI_KVS_iter_first` and
  `int PMI_KVS_iter_next` are used to transfer KVS's in grid environments, and
  could also be omitted from some implementations.
- The `spawned` argument to `PMI_Init` is necessary to implement MPI-2
  functionality, in particular `MPI_Get_parent`. In a PMI implementation that
  does not support dynamic process management, it can always just return a
  pointer to 0.
- The `PMI_Commit` exists so that in case `PMI_Put` is an expensive operation,
  involving communication with an external process, several `PMI_Put`s can be
  batched locally and only sent off when the `PMI_Commit` is done.
- The notion of KVS is reminiscent of Linda, in which processes execute `read`
  and `write` operations on a shared `tuple space`. Why not use the Linda
  interface? The reason is scalability.
  Linda implements a blocking `read`, in which the calling process blocks until
  data with the requested key is put into the tuple space by another process.
  While this is a convenient synchronizing operation, and could in theory be
  used here, it would not be scalable. Note that in PMI, there is no
  point-to-point communication. The only synchronization operation is the
  `PMI_Barrier`, which can have a variety of scalable implementations,
  depending on the environment.

**To Do**: Provide minimum sizes for the various strings. Here is a proposal.

The minimum sizes of the names and values stored will depend on the MPI
implementation. The following limits will work with most implementations:

```
- [kvsname]16
- [key]32
- [value]64
- [Number of keys]Number of processes in an MPI program (size of
  `MPI_COMM_WORLD` in an MPI-1 program)
- [Number of groups]Number of separate `MPI_COMM_WORLD`s
  managed by the process manager.  For an single MPI-1 code, this is one.
```

### Part 2:  Advanced PMI Routines

This part of PMI is still under development.  If one assumes that the dynamic
process management functions in MPI-2 are not performance critical, then the
requirements for efficiency and scalability of these operations are less
crucial, although we expect `MPI_Comm_Spawn` to be scalably implemented, at
least to compete with an original `mpiexec`.

```
/* PMI Process Creation functions */

int PMI_Spawn_multiple(int count, const char *cmds[], const char **argvs[], 
                       const int *maxprocs, const void *info, int *errors, 
                       int *same_domain, const void *preput_info);

int PMI_Spawn(const char *cmd, const char *argv[], const int maxprocs,
              char *spawned_kvsname, const int kvsnamelen );

/* parse PMI implementation specific values into an info object that can
   then be passed to PMI_Spawn_multiple.  Remove PMI implementation
   specific arguments from argc and argv
*/

int PMI_Args_to_info(int *argcp, char ***argvp, void *infop);

/* Other PMI functions to be defined as necessary for other parts of
   dynamic process management */

```

## Typical Usage

In this section we give an example of typical usage of the PMI interface in
MPICH. In the `CH3_TCP` device used on Linux clusters, TCP is used to support
MPI communication. Connections between processes are established by the normal
socket `connect`/`accept` mechanism. For this to work, before the first
`MPI_SEND` from one process to another, one process must have acquired a port
from the operating system and be listening on it with the normal
`socket`/`bind`/`listen` sequence. The other process, typically on a separate
host, will execute the corresponding `socket`/`connect` sequence, at which time
the first process will issue an `accept`, establishing the TCP connection.
Since we don't use reserved ports, the first process must advertise in some way
the port it is listening on. Since for the sake of scalability and rapid
startup we don't establish these connections until they are needed, the
`connect` operation is not executed until the socket is needed, typically the
first time a process issues an `MPI_SEND`. At this point the MPICH
implementation must determine from the MPI rank of the destination process
which host, and which port on that host, to connect to in order to establish
the connection.

PMI is the mechanism by which the first process advertises its host and
listening port, keyed by rank, and the the second process finds out this
information. Thus the sequence of events during `MPI_Init` goes like this:

- During `MPI_Init`, each process calls `PMI_Init`, in order to perform
  whatever initialization is needed by the PMI implementation.
- Still in `MPI_Init`, each process calls `PMI_Get_rank` to find out its rank
  in the MPI job.
- Each process executes `gethostname` to find out its host and `socket`/`bind`
  to obtain a port.
- Each process creates a key from its rank and a value for that key from its
  host and port. We actually use two pairs, using keys `P<rank>-hostname` and
  `P<rank>-port`.
- Each process does a `PMI_KVS_Put` to put its `(key, value)` pairs into the
  default KVS. It may deposit other information with other calls to
  `PMI_KVS_Put`. It does a `PMI_Commit` to flush all of the `(key, value)`
  pairs to the KVS.
- All processes execute `PMI_Barrier` to synchronize. It is assumed that this
  operation is implemented in a scalable way. Note that MPI communication is
  not available yet, so this is not an `MPI_Barrier`. We are still inside
  `MPI_Init`.

At this point the only non-local communication that has taken place is the
barrier. Now, each process can exit from `MPI_INIT`, having made available the
information that only it knows (the listening port).

When a process executes any form of `MPI_SEND`, the implementation can check to
see if a connection to the destination process already exists, and if not, use
the rank to create the appropriate key and do `PMI_KVS_Get` to find the host
and port of the destination process and do the `connect`. We currently keep
these sockets open for the rest of the job, but there is nothing to preclude
closing and reopening them as needed.

The above sequence is of course not the only way to use the PMI interface, but
it constitutes a typical example of its use. Note that even if more information
is conveyed in this way, the actual size of the KVS is not anticipated to be
large. Room for a few `(key, value)` pairs for each process is all that is
necessary.

## Implementing PMI

In this section we describe some implementations of PMI that we are using,
together with a design for one we are not. Although this note is about the
interface itself and not any specific implementation, it might be useful to
understand the alternatives that we have explored and are in current use. Also,
the very existence of multiple implementations demonstrates that PMI is a real
interface with a purpose, not just a design for part of MPICH. All implementations are distributed with
MPICH, as described at the end of the 
[implications](#implications)
section.

It is useful to think of a PMI implementation as having two parts: a *client*
side and a *server* side. The client side is the direct implementation of the
PMI functions defined above, linked together with the MPI library in the
application's executable. In some cases this part of the implementation
communicates with other processes not part of the application;  we call these
processes the server side of the PMI implementation. As we shall see, the
server side may not exist at all, or be part of the client side; multiple
architectures for PMI implementations already exist. In the following
subsection we describe some existing client-side PMI implementations. In the
next section we describe some server-side implementations. Currently most of
these are part of the
[MPICH distribution](https://www.mpich.org/).

### Client Side

We currently are using three separate implementations of the client side of PMI.

- `[uni]` - Ss a stub used for debugging. It assumes that there is only one
  process and so needs to provide no services.
- `[simple]` - Is our primary implementation for Unix systems. It assumes that
  a socket (the PMI socket) has been created that can be used to exchange
  commands with the server side of the implementation. It is used by multiple
  server implementations, as described below.

### Server Side

One can think of the server side of a PMI implementation as that part of a
process management system that supports the client. Currently several are in
use or under development.

- `[forker]` - Implements the server side of the `simple` client side described
  above. It is used primarily for debugging, although it can also be used in
  production on SMP's. It consists of an `mpiexec` script that simply forks the
  parallel processes after setting up the PMI socket. Thus all processes must 
  be on the same machine.
- `[Remshell]` - Uses `rsh` or `ssh` to start the processes from the `mpiexec`
  process, then they connect back to exchange the keyval information. This
  illustrates the combination of an old (`rsh`) process startup mechanism with
  a new data-exchange mechanism.

### Combined Client and Server

The
[MPICH-G2](https://www.sciencedirect.com/science/article/pii/S0743731503000029)
implementation of MPI illustrates yet another approach. MPICH-G2 is built on
MPICH1 and thus uses the BNR interface, but the underlying principles are the
same. In MPICH-G2, the `put` operations are local, and the `barrier` operation
is a global all-to-all-exchange, implemented in a scalable way. Then the `get`s
can be done without further communication. 

### Adding a PMI Module to an Existing Process Starter
<a id="adding"></a>

In the implementations listed above, we have combined the PMI implementation,
particularly the server side, with a process startup mechanism being
implemented at the same time. Some systems, such as SLURM, may already have
scalable methods in place for starting processes and might be looking for the
simplest way to add PMI capabilities. Although the best approach is likely to
be to incorporate PMI server-side capabilities into the process starter, it may
be that the following approach, though less scalable, might be serviceable:

- At the time each process of the MPI job is started, it is passed it's rank
  and the size of the job in an environment variable, since these are things
  the process manager knows. This could be used to implement `PMI_Get_rank` and
  `PMI_Get_size`. (Actually these values would probably be read from the
  environment during `PMI_Init` and cached.)
- At the time the job is started, a separate `KVS server` process would be
  forked to hold all KVS data.
- All processes would send their `PMI_KVS_Put` data to this server. Use of UDP
  rather than TCP would probably help with the obvious scalability problem that
  this server would receive data from each process in the job at approximately
  the same time.
- The `PMI_Barrier` would be implemented in the server with a simple counter.
- Data requested by `PMI_KVS_Get` would come from the server.
- A variation would be to have all the data broadcast at the time of the
  barrier, so that subsequent gets would be local.

This mechanism is not intrinsically scalable to thousands of nodes, which is
why we are not using it. However, it might scale farther than a few hundred
nodes, and be a rather straightforward addition to an existing process startup
mechanism.

## Resource Registration
There are some resources that a program may need to allocate that the program
cannot guarantee will be released when the program exits, particularly if the
program exits as the result of an error or an uncatchable signal. These
resources include other processes, SYSV shared memory segments and semaphores,
and temporary files. The routines in this section allow the program to notify
the process manager of these resources and provide a general way for the
process manager to free them when the program exits.  

If the process manager does not provide these functions, then there
are several options:

- The calls can be ignored. The program will do its best to free these
  resources when it exits. This may include setting a cleanup handler on the
  catchable signals that normally cause an abort. Note that in this case the
  registration routine must return an error so that the application knows that
  it must handle this itself.
- The calls can be directed to an alternate process, called a `watchdog`, that
  will free the resources if the watched process terminates abnormally.

Note that this interface provides a way for process managers to permit a
process to create new processes, since the processes will be registered with
the process manager.

The following is still in rough draft form
```
int PMI_Resource_register( const char *name, (void *()(void*))at_exit, 
                           void *at_exit_extra_data,
                          (void *()(void *))at_abort, 
                           void *at_abort_extra_data );

int PMI_Resource_release_begin( const char *name, int timeout );
int PMI_Resource_release_end( const char *name );
```

The functions in `PMI_Resource_register` may need to be command names or an
enumerated list of known resources.  

The release functions are split to allow the process to indicate that it is
about to release a resource and a timeout at which time the watchdog may
consider the process to have failed.  For example, when removing a SYSV shared
memory segment, the following code would be used:

```
PMI_Resource_release_begin( "myipc", 10 );
shmctl( memid, IPC_RMID, NULL );
PMI_Resource_release_end( "myipc" );
```

This interface still contains a small race condition: the time between when the
resource is created and when it is registered. This is a very narrow race, so
it may not be important to close it (and with registration, much more likely
resource leaks have been closed). However, a two-phase registration process
could be considered, that would register the intent to create a resource. In
the case of failure to complete the second part of the two-phase registration,
the watchdog could try to hunt down the newly allocated resource.  

## Topology Information
The process manager often has some information about the process topology. For
example, it is likely to know about multiprocessor nodes and may know about
parallel machine layout. The routines in this section provide a way for the
process manager to communicate that information to the program. As with the
other PMI services, if the process manager cannot provide this service, several
alternatives exist, including returning an `PMI_ERR_UNSUPPORTED` and using a
separate service to provide this information.

```
int PMI_Topo_type( PMI_Group group, int *kind );
int PMI_Topo_cluster_info( PMI_Group group, 
                            int *levels, int my_cluster[], 
                            int my_rank[] );
int PMI_Topo_mesh_info( PMI_Group group, int ndims, int dims[] );
```
These routines provide information on the specified PMI group. `PMI_Topo_type`
gets the type of topology. The current choices are `PMI_TOPO_CLUSTER`,
`PMI_TOPO_MESH`, and `PMI_TOPO_NONE`. The other routines provide information
about the cluster and mesh topology. Other topologies can be added as needed.
These cover most current systems.

## Resource Allocation on Behalf of Parallel Jobs

In some cases, resources must be allocated before a process is created. For'
example, if several processes on the same SMP node are to share an anonymous
`mmap` (for shared memory), this memory must be allocated before the processes
are created (strictly, before all but the first process is created, if the
first process creates the others). The purpose of the routines in this section
is to allow a startup program, such as `mpiexec`, to describe these
requirements to the process manager before  any processes are started.

> Question: it may be that the only routine here is used to answer the
> question, did you give me the resource? This leaves unanswered the question
> of, "how does a device let an mpiexec know that it needs a particular
> resource?"

## Implications for Collaborators
<a id="implications"></a>

We hope that this brief discussion has made it easier to understand what
options and opportunities exist for implementors of parallel programming 
libraries or process management environments that will interact with MPICH or
MPICH-derived MPI implementations.

MPI and other library implementors are recommended to use the PMI functions to
exchange data with other processes related to the setting up of the primary
communication mechanism. MPICH does this already for setting up TCP connections
in the CH3 implementation of the Abstract Device Interface (ADI-3). If one
links with the `simple` implementation of the client side of the PMI
implementation in MPICH, then MPI jobs can be started by any process management
environment that implements the server side.

Process management systems, such as PBS, YOD, or SLURM, have two options in the
short run.

In the long run implementations may prefer to implement both sides themselves,
meaning that one would link one's application with a PBS, SLURM, or Myricom 
specific object file implementing the client side.

The PMI-related code described here is available in the current MPICH 
distribution, in the `src/pmi/{simple,uni}` (client side) and `src/pm/forker`
(server side) subdirectories. Different process managers (the server side) and
different PMI implementations can be chosen when MPICH is configured. The
default is as if one had specified

```
     configure --with-pmi=simple
```

Please send questions and comments to mpich-discuss@mcs.anl.gov.

# Appendix
## Wire Protocol for the Simple PMI Implementation
- `PMI_PORT` - Environment variable
