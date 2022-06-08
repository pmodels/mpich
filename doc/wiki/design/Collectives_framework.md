# Collectives Framework

The framework for collective operations, 
[pull request #2663](https://github.com/pmodels/mpich/pull/2663),
is designed to allow implementation of collective operations in
different ways in different directories inside the
`src/mpi/coll/algorithms directory`. An example of a directory is `tree`.
In this directory, collective operations are implemented using trees.
Other examples of algorithms (not yet published) are `recexchg` (recursive
exchange), `dissem` (dissemination, brucks based), `ring` (ring based), etc.
We also have `shm_gr` (gather-release based shared memory optimized
collective algorithms). Other examples, could be hybrid (for example,
combination of tree and recursive exchange), etc.

As before, for each collective operation there is a `<coll_operation>.c`
file in the coll directory which implements the `MPI_<coll>` function. In
this file, the selection criteria is used to select and then make a call
to the appropriate collective algorithm. The original MPIR collective
algorithms have been migrated inside the algorithms/default directory
and are always called by default. Therefore, the MPICH runtime does not
change at all for its users. For each collective operation, there is a
corresponding CVAR variable to select a specific collective algorithm.
For example, for broadcast, there is a CVAR named `MPIR_CVAR_USE_BCAST`
which can take value 0, 1, 2, etc. 0 corresponds to the default
broadcast which is the original MPIR broadcast, and 1 and 2 correspond
to the knomial and kary tree based broadcast algorithms, respectively.
Depending upon the algorithm, there are additional CVARs to specify the
branching factor and pipelining segment size, etc.

A salient feature of some of the algorithms in the collectives framework
is that they are implemented independent of the transport. We define a
transport API, and the algorithms are implemented using this transport
API (discussed below). The benefit of this approach is that the
algorithms need not be reimplemented for every transport. Instead, only
the transport API needs to be implemented for a new transport and the
algorithms can simply be imported. Also, the same algorithm
implementation is used by both blocking and non-blocking collectives.
This is achieved by having a `COLL_kick_sched` (for blocking
collectives) and `COLL_kick_sched_nb` (for non-blocking collectives)
functions to kick a schedule in a blocking or a non-blocking fashion,
respectively.

## Transport API

Transport API consists of operations such as send, receive, reduce, data
copy, memory allocation/deallocation, etc. The transport API and the
transport types are defined in `src/mpi/coll/transports/stub/transport.h`
and `src/mpi/coll/transports/stub/transport_types.h`, respectively. The
Transport API allows the algorithm implementor to specify explicit
dependencies amongst tasks in the schedule of the collective operation.
The transport API is of the form:

```
int TSP_task ([task_args], TSP_sched_t *sched, int n_invtcs, int *invtcs)
```

where, `task_args` are arguments required to perform the task, `sched` is
the schedule to which this task is being added,

`n_invtcs` is number of tasks that this task depends on to complete
before it can be issued,

`invtcs` is the array of task ids that should complete before this task
can be issued

The return value is a unique id of this new task in the schedule.

Let us consider a simple example of the usage of the transport API - A
receive operation followed by a send operation that depends on the
receive to complete –

```
recv_id = TSP_recv(recv_arguments, sched, 0, NULL);  // This task has no dependency
send_id = TSP_send(send_arguments, sched, 1, &recv_id); // It has a dependency on the previous receive task to complete
```

The transport can use the dependency information provided by the
algorithm writer to internally develop a dependency graph and use it for
execution of the schedule.

## MPICH Transport

An example of the transports is the MPICH transport, which uses MPIR
function calls to implement the transport. It maintains a dependency
graph of the tasks. It issues a task whenever its input dependencies
have completed execution.

The MPICH transport also saves the schedules in a schedule database for
later reuse. Transport API has two functions to support reuse of
schedules - `TSP_save_schedule` and `TSP_get_schedule`. Each schedule
has a unique key associated with it which is used to store the schedule
in a hash table. This key is made of the arguments to the collective
operation and the unique parameters of the collective algorithm. Instead
of generating a new schedule every time, the algorithm writer can first
check if a schedule for the collective algorithm alreadys exists and
reuse it if it exists. This saves the schedule generation time for
persistent collectives. Once the persistent collectives API is accepted
(proposed for MPI 4.4), these schedules can be stored in the
`MPIR_Request` itself and hence will not require searching the database.

## Algorithm Naming

All the functions related to collective operations have the MPIC prefix.
MPICH transport based tree algorithm for broadcast is named as
`MPIC_MPICH_TREE_Bcast(bcast_argumnets, tree_parameters)`. A tree
based broadcast on top of OFI transport could be named as
`MPIC_OFI_TREE_Bcast()`. These names are autogenerated based on the
transport they are compiled with. They are implemented with `COLL_*`
macro which is expanded for each transport and algorithm type. The
default algorithms (the original MPIR algorithms) are called as
`MPIC_DEFAULT_Bcast()`, and so on.

Each algorithm type can have its own communicatory type (for example,
`COLL_comm_t`) where it can store communicator specific data. For
example, a recursive exchange based implementation of allreduce using
binary blocks algorithms will require storing subcommunicators of power
of 2 number of processors (refer to Thakur et al, Optimization of
Collective Communication Ops in MPICH, IJHPCA 2005). In case of tree
based algorithms, the communicator type can be used to encapsulate tree
structures. Transport specific datatypes are also allowed (named,
`TSP_dt_t`) so that any transport specific information (for example
iovecs in case of OFI) can be stored in them.

## Implementation Details

The algorithms to be compiled are included in the
`src/mpi/coll/include/coll_impl.h` file and the corresponding types for
the algorithms are included in the `src/mpi/coll/include/types_decl.h`
file. The general structure that the directories inside the algorithms
directory follow is that they have a `pre.h` and `post.h`, where `pre.h` has
all the types defined for that algorithm and post.h has all collective
operation calls defined. There is a "common" directory inside the
algorithms directory which contains various utility and common schedule
functions that can be used across various algorithms.

## More on COLL_NAMESPACE

Each collective operation implementation has two components - the
transport and the algorithm. `COLL_NAMESPACE` is made of up these two
components. An example of `COLL_NAMESPACE` is `MPIC_MPICH_TREE_` where
MPICH is the name of transport and `TREE` is the algorithm name.
`COLL_NAMESPACE` is prefixed to functions/data structures used to
implement collective operations. Consider a generic routine such as
`COLL_schedule_tree_bcast` that generates a schedule for tree based
broadcast. This routine can be used by multiple algorithm types. For
example, `TREE` algorithms, `HYBRID` algorithms (an example of hybrid
algorithm is allreduce in which we do recursive doubling for inter-node
and tree based reduce/bcast for intranode). Both these algorithms can
use the same `COLL_schedule_tree_bcast` function. When compiled with
`TREE` algorithms this function becomes `MPIC_MPICH_TREE_schedule_tree_bcast`,
when compiled with `HYBRID` algorithms, this function becomes
`MPIC_MPICH_HYBRID_schedule_tree_bcast`.

Other component of the `COLL_NAMESPACE` is the transport. Same
implementation of the algorithm can be compiled with different
transports - for example, mpich transport, ofi transport, etc.
