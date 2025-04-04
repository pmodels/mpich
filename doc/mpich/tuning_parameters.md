# MPICH Tuning Parameters

This document discusses some of the new features added to the MPICH-based MPI
implementation for the Aurora computer and the environment variables (CVARs)
that the user can set to modify its behavior. MPICH is configured to use
sensible default values for all the environment variables to provide overall
good performance. However, users can change those defaults values to better tune
their applications. This report provides documentation on how those CVARS work
and the possible values that they can take. This documentation also provides
information about per-communicator info hints that MPICH utilizes to make
runtime decisions at a finer granularity.

Notice that performance of the application depends on how ranks are mapped to
the cores of the processor. For information on how to specify process-core
binding with mpirun, information is provided here:
https://github.com/pmodels/mpich/blob/main/doc/wiki/how_to/Using_the_Hydra_Process_Manager.md#process-core-binding.

This document provides information about generic environment variables (Section 1),
collective selection (Section 2), threading optimizations (Section 3),
shared memory (Section 4), RMA (Section 5), support for multiple NICs (Section 6)
and GPU aware communication (Section 7).

## 1. General

The environment variables in this section impact some of the general behavior
in MPICH that isn’t covered by more specific sections below.

* `MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS` / `MPIR_CVAR_CH4_OFI_RANK_BITS`
/ `MPIR_CVAR_CH4_OFI_TAG_BITS`: Specify the number of bits to use for each
part of the tag matching scheme. There are 60 bits available that can be
distributed to allow for more of each of these parts of the matching scheme.
More context bits allows more context IDs (used for Communicators, RMA windows,
and Files). More rank bits allows for more MPI ranks (though the rank bits are
usually not used for hardware tag matching and therefore are not subject to
the size limit). More tag bits allows for a larger user tag value with
matching communication. The default values (in most cases) for each are:
  * `MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS`: 20
  * `MPIR_CVAR_CH4_OFI_RANK_BITS`: 0 (offloaded to software and gets a full
    32 bits)
  * `MPIR_CVAR_CH4_OFI_TAG_BITS`: 31

* `MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY`: If set to positive, this CVAR specifies
the maximum number of retries of OFI operations before returning `MPIX_ERR_EAGAIN`.
This value is effective only when the communicator has the `MPI_OFI_set_eagain`
info hint set to true. This could potentially help if seeing a high amount of
flow control from the fabric that is causing the MPI application to fail
communication.

## 2. Collective selection

When an application calls a collective, the MPI runtime needs to determine
how to execute the collective in the most efficient way. For that, the MPI
runtime may take into account the message size, number of nodes, and number
of ranks per node that participate in the collective. MPICH comes with a
default setting (that uses JSON files) to automatically select the best
strategy to execute a collective operation. The structure of the JSON files
is described in Section 2.1, although in general users should not need to
change the contents of these JSON files, as they should have been
appropriately tuned for the target system. Notice that the current
implementation only supports collective selection for blocking collectives.
The selection of the algorithms for non-blocking collectives uses the
environment variables described in Section 2.4.

For advanced users that do not want to rely on the collective algorithms
selected by the JSON files and want to select the particular algorithm a
collective should use, MPICH provides environment variables (CVARS). These
environment variables, when used, supersede the setting specified in the
JSON files. In most cases, more than one environment variable needs to be
specified to obtain the desired behavior. To help users choose the
appropriate configuration variables, this documentation provides a high
level overview of how the MPICH runtime determines the collective algorithm
to use. First, when an application calls a collective operation which
involves multiple nodes and multiple ranks per node, the runtime needs to
determine if the collective should execute in a hierarchical manner, where
the ranks inside the node perform an intra-node collective first followed
by an inter-node collective. For each collective, there are different ways
to compose this intra-node and inter-node portion of the collective, as
determined by the composition environment variables described in Section 2.2.
Then, for those ranks that are within the same node, it is possible to
execute specialized algorithms. This can be selected with the environment
variables described in Section 2.6. Environment variables can be used to
determine the exact algorithm used to implement the collective. MPICH
provides support to select the best algorithm for blocking and for
non-blocking collective calls, as described in 3 and 4, respectively.
As mentioned above, non-blocking collectives execute the algorithm
determined by the appropriate environment variables, as these are not
supported with the JSON files.

MPICH also supports the persistent API for non-blocking collectives
that will appear in MPI 4.0. This is described in Section 2.5.

MPICH is now GPU-aware, that is, it supports MPI calls that pass pointers
to buffers allocated in the GPU device. Currently, this support is not
enabled for non-blocking collectives, but it is supported for blocking
collectives. Since GPU-support is still in progress, some of the CVARS
described in this document may not apply to those collectives that use
GPU buffers. This documentation will be updated as implementation
progresses.

```
                             +-----------------+
                  /----------| No composition  |
                 /           +-----------------+
+-------+       /                                                +------------+
|  MPI  |------<                                    /------------| Inter node |
+-------+       \                                  /             +------------+
                 \           +-------------+      /
                  \----------| Composition |-----<
                             +-------------+      \
                                                   \             +------------+
                                                    \------------| Intra node |
                                                                 +------------+
```


### 2.1. JSON file

MPICH comes with a default JSON tuning file. The file settings determine
which algorithms a collective will use for a particular message and
communicator size. There are 3 types/levels of JSON tuning files:
1. Composition: It determines the algorithm to partition a communicator
for hierarchical algorithms that take into account the ranks on the same
node and on different nodes. This JSON file is used by setting
`MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE=/path/to/ch4_selection_tuning.json`.
2. Coll: This file determines which algorithm used for inter or intra
algorithms, depending on the composition. This JSON file can be used by
setting `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE=/path/to/coll_selection_tuning.json`.
This file contains algorithm covered in the blocking collectives section
(Section 2.3).
3. Posix: This file determines algorithms to be used for intra node cases.
This JSON file can be used by setting
`MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE=/path/to/posix_selection_tuning.json`.
The files contains algorithms covered in the intra node section (Section 2.6).

These environment variables should be loaded when the user loads the
MPICH module. It is possible for the user to submit their own tuning
JSON files, but a detailed explanation of the content of the JSON files
is beyond the scope of this documentation. As mentioned above, selecting
specific algorithms with CVARs will override the algorithm selection
specified in the JSON files.

### 2.2 Compositions

MPICH uses the environment variable `MPIR_CVAR_<collname>_COMPOSITION` to
indicate the composition to utilize, that is, how to compose the part of
the collective executed by the ranks on the same node with the part of
the collective executed by the ranks on different nodes. For instance,
`MPI_CVAR_BCAST_COMPOSITION` indicates the possible types of composition
for BCAST. The options for these CVARS depend on the specific algorithm.
The possible options are:

* Auto: Auto selection by using the JSON file.

* NM + SHM with send-recv is used for BCAST, IBCAST, REDUCE, and IREDUCE
algorithms and uses an algorithm that takes advantage of the shared memory
among the ranks on the same node. With this composition, if the collective
root rank and the node lead rank are different, there is first a  point to
point message between the collective root rank and the lead rank of the node
where the collective root rank runs (MPICH usually selects as lead rank the
rank with lowest id on the node).

* NM + SHM without send-recv is used for BCAST, IBCAST, REDUCE, and IREDUCE
algorithms and uses an algorithm that takes advantage of the shared memory
among the ranks on the same node. With this composition, if the collective
root rank and the node lead rank are different there is no point to point
message, but instead a BCAST or a REDUCE operation is performed on the node
where the collective root rank runs. This Bcast or Reduce is executed in the
beginning of the BCAST or at the end of the REDUCE, respectively.

* NM only: Does not apply a composition. The collective executes without
taking into account whether the ranks are within the same or different nodes.
* Multi-Leaders: When using a composition, a rank in each node is selected
to communicate to drive the inter-node part of the collective. With this
option, we allow multiple leaders to participate on that part of the
collective. This is specially important if the message is large. It can
also take advantage of multiple network cards.

* NM + SHM with reduce + Bcast. The intra-node part of the collective uses
an algorithm that takes advantage of the shared memory of the node and that
executes the collective as an intra-node reduce, and inter-node allreduce,
and an intra-node reduce operation.

Multi-leader algorithms are supported for ALLREDUCE, ALLTOALL, and ALLGATHER
when the  CVAR `MPIR_CVAR_<collname>_COMPOSITION` is set to the appropriate
value. In this case:

* `MPIR_CVAR_NUM_MULTI_LEADS` indicates the number of leader ranks per node.

* `MPIR_CVAR_ALLREDUCE_SHM_PER_LEADER` indicates the amount of shared memory
region (in bytes) per node-leader and communicator for ALLREDUCE.


#### Table I: CVARS for composition configurations

```
+ ============== + ================================================= + ============================================== +
| Composition    | Name of CVAR                                      | Options                                        |
+ ============== + ================================================= + ============================================== +
| BCAST/IBCASST  | MPIR_CVAR_BCAST_COMPOSITION                       | 0 (Auto Selection), 1 (NM + SHM with send-recv)|
|                |                                                   | 2 (NM + SHM without send-recv), 3 (NM only)    |
+ -------------- + ------------------------------------------------- + ---------------------------------------------- +
| REDUCE/IREDUCE | MPIR_CVAR_REDUCE_COMPOSITION                      | 0 (Auto Selection), 1 (NM + SHM with send-recv)|
|                |                                                   | 2 (NM + SHM without send-recv), 3 (NM only)    |
+ -------------- + ------------------------------------------------- + ---------------------------------------------- +
| ALLREDUCE/     | MPIR_CVAR_ALLREDUCE_COMPOSITION /                 | 0 (Auto Selection) 1 (NM + SHM with reduce +   |
|                |                                                   | bcast) 2 (NM only) 3(SHM only) 4(Multi-Leaders)|
+ -------------- + ------------------------------------------------- + ---------------------------------------------- +
| ALLTOALL       | MPIR_CVAR_ALLTOALL_COMPOSITION                    | 0 (Auto Selection), 1 (Multi-Leaders),         |
|                |                                                   | 3 (NM only)                                    |
+ -------------- + ------------------------------------------------- + ---------------------------------------------- +
| ALLGATHER      | MPIR_CVAR_ALLGATHER_COMPOSITION                   | 0 (Auto Selection), 1 (Multi-Leaders),         |
|                |                                                   | 3 (NM only)                                    |
+ -------------- + ------------------------------------------------- + ---------------------------------------------- +
| BARRIER /      | MPIR_CVAR_BARRIER_COMPOSITION /                   | 0 (Auto Selection), 1 (NM + SHM), 2 (NM only)  |
| IBARRIER       | MPI_CVAR_IBARRIER_COMPOSITION                     |                                                |
+ -------------- + ------------------------------------------------- + ---------------------------------------------- +
```

### 2.3. Blocking collective algorithms

Collective calls like MPI_Allreduce or MPI_Bcast are blocking calls that
only return once the collective is completed. MPICH supports the algorithms
described in the paper by Thakur et al.
(https://www.mcs.anl.gov/~thakur/papers/ijhpca-coll.pdf).
In addition, many algorithms have been extended to execute with radices
higher than 2. A Higher radix decreases the total number of steps it
takes to execute an algorithm, and can result in lower running time,
specially for small messages. A detailed explanation of high radix
algorithms is, however, beyond the scope of this document.

To select a particular algorithm for a blocking collective, use
`MPIR_CVAR_<collname>_INTRA_ALGORITHM=<algo_name>` where \<collname\> is
one of the collectives and \<algo_name\> is one of the supported algorithms
for that collective, as shown in the first and second columns of Table II,
respectively. For example, MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM=pairwise will
use the pairwise algorithm to implement an ALLTOALL collective. Some
algorithms, such as kbrucks, have additional parameters that can be
specified when that algorithm is selected
(e.g., MPIR_CVAR_ALLTOALL_BRUCKS_KVAL=4, where 4 is the desired radix for
the algorithm). If the corresponding CVAR to set the value of a given
parameter is not used, a default value will be used, as documented in
Table II.

To use the same algorithm for both, blocking and non-blocking versions
of the collective, set `MPIR_CVAR_<collname>_DEVICE_COLLECTIVE=0`. In
this case, both blocking and non-blocking collectives, e.g. BCAST and
IBCAST, will use the same blocking algorithm. Notice that if
`MPIR_CVAR_<collname>_DEVICE_COLLECTIVE` is set to 0, compositions
(Section 2.2) are disabled.

#### Table II: Algorithms available for each blocking collective operation

```
+ ========== + ============================ + ================================== + =================== +
| Collective | Algo name                    | Algo params                        | Notes               |
+ ========== + ============================ + ================================== + =================== +
| ALLTOALL   | pairwise                     | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLTOALL   | pairwise_sendrecv_replace    | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLTOALL   | scattered                    | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLTOALL   | brucks                       | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLTOALL   | k_brucks                     | MPIR_CVAR_ALLTOALL_BRUCKS_KVAL=k   | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHER  | ring                         | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHER  | brucks                       | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHER  | k_brucks                     | MPIR_CVAR_ALLGATHER_BRUCKS_KVAL=k  | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHER  | recursive_doubling           | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHER  | recexch_doubling             | MPIR_CVAR_ALLGATHER_RECEXCH_KVAL=k | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHER  | recexch_halving              | MPIR_CVAR_ALLGATHER_RECEXCH_KVAL=k | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHERV | ring                         | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHERV | brucks                       | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHERV | brucks                       | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLGATHERV | recursive_doubling           | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLREDUCE  | ring                         | Implementation based off:                                |
|            |                              | https://andrew.gibiansky.com/blog/machine-learning/      |
|            |                              | baidu-allreduce/                                         |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLREDUCE  | recursive_doubling           | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLREDUCE  | recexch                      | MPIR_CVAR_ALLREDUCE_RECEXCH_KVAL=k | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLREDUCE  | reduce_scatter_allgather     | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLREDUCE  | k_reduce_scatter_allgather   | MPIR_CVAR_ALLREDUCE_RECEXCH_KVAL=k | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| ALLREDUCE  | tree                         | MPIR_CVAR_ALLREDUCE_TREE_TYPE=kary/| k >= 2, default k=2 |
|            |                              | knomial_1/knomial_2/topology_aware/| default tree type is|
|            |                              | topology_aware_k/topology_wave     | knomial_1           |
|            |                              | MPIR_CVAR_ALLREDUCE_TREE_KVAL=k,   |                     |
|            |                              | MPIR_CVAR_ALLREDUCE_TREE_PIPELINE  |                     |
|            |                              | _CHUNK_SIZE =4096,                 |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| BARRIER    | recexch                      | MPIR_CVAR_BARRIER_RECEXCH_KVAL=k   | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| BARRIER    | k_dissemination              | MPIR_CVAR_BARRIER_DISSEM_KVAL=k    | k >= 2, default k=2 |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| BCAST      | binomial                     | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| BCAST      | scatter_recursive_doubling_  | NONE                               |                     |
|            | allgather                    |                                    |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| BCAST      | scatter_ring_allgather       | NONE                               |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| BCAST      | tree                         | MPIR_CVAR_BCAST_TREE_TYPE=kary/    | k >= 2, default k=2 |
|            |                              | knomial_1/knomial_2/topology_aware/| default tree type is|
|            |                              | topology_aware_k/topology_wave,    | knomial_1           |
|            |                              | MPIR_CVAR_BCAST_TREE_KVAL=k        |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
| BCAST      | pipelined_tree               | MPIR_CVAR_BCAST_TREE_TYPE=kary/    | k >= 2, default k=2 |
|            |                              | knomial_1/knomial_2/topology_aware/| default tree type is|
|            |                              | topology_aware_k/topology_wave,    | knomial_1           |
|            |                              | MPIR_CVAR_BCAST_TREE_KVAL=k        |                     |
+ ---------- + ---------------------------- + ---------------------------------- + ------------------- +
```

`MPIR_CVAR_CH4_GPU_COLL_SWAP_BUFFER_SZ` determines the size of the swap
buffer used for GPU collective operations. Currently, it only supports
MPI_Bcast and MPI_Allreduce. If the message size is smaller or equal to
this swap buffer size, MPICH will transfer the data to the host and
perform the collective operation there. If the message size exceeds this
buffer size, MPICH will perform the collective operation directly on the
GPU.

The following CVARs are related to the topology-aware collective algorithms:
* `MPIR_CVAR_<collname>_TOPO_REORDER_ENABLE`, where \<collname\> is BCAST
or ALLREDUCE, controls if the leaders are reordered based on the number of
ranks in each group.

* `MPIR_CVAR_<collname>_TOPO_OVERHEAD`, where \<collname\> is BCAST or
ALLREDUCE, controls the overhead used in the topology-wave algorithm.

* `MPIR_CVAR_<collname>_TOPO_DIFF_GROUPS`, where \<collname\> is BCAST
or ALLREDUCE, controls the latency between different groups used in the
topology-wave algorithm.

* `MPIR_CVAR_<collname>_TOPO_DIFF_SWITCHES`, where \<collname\> is BCAST
or ALLREDUCE, controls the latency between different switches in the same
group used in the topology-wave algorithm.

* `MPIR_CVAR_<collname>_TOPO_SAME_SWITCHES`, where \<collname\> is BCAST
or ALLREDUCE, controls the latency between the same switch used in the
topology-wave algorithm.

### 2.4. Non Blocking collective algorithms

Non blocking algorithms are implemented using two types of schedulers,
a Directed Acyclic Graph scheduler and a linear or sequential scheduler.
The DAG-based scheduler is experimental and the performance is still under
evaluation. Performance gains with the algorithms based on the DAG
scheduler can be observed when used through persistent API
(See Section 2.5).

To select a particular non-blocking algorithm for a collective, use
`MPIR_CVAR_<collname>_INTRA_ALGORITHM=<algo_name>` where \<collname\>
is one of the collectives and \<algo_name\> is one of the supported
algorithms for that collective, as shown in the first and second
columns of Table III, respectively. For example,
MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM=sched_ring will use a ring-based
algorithm to implement an IALLTOALL collective. Some algorithms, such
as gentrans_brucks, have additional parameters
(e.g., MPIR_CVAR_IALLTOALL_BRUCKS_KVAL=4, where 4 is the desired radix
for the algorithm). When the value for that parameter is not specified,
MPICH uses a default value, as specified in Table III>
Notice that the algorithms based on the linear scheduler have names that
start with sched, while the ones based on the DAG scheduler have names
that start with gentran.

A blocking collective, such as BCAST can use non-blocking algorithms
if the CVAR `MPIR_CVAR_BCAST_INTRA_ALGORITHM=nb`. This CVAR transforms
a BCAST call into IBCAST followed by wait. In this case, the user can
select any of the non-blocking algorithms for BCAST.

To disable composition while using these algorithms, set
`MPIR_CVAR_*collname*_DEVICE_COLLECTIVE=0>`.

Please refer to Section 2.3 for the CVARs related to topology-aware
Ireduce.

#### Table III: Algorithms available for each blocking collective operation

```
+ ===================== + ============================ + ================================== + =======================+
| Collective            | Algo name                    | Algo params                        | Notes                  |
+ ===================== + ============================ + ================================== + =======================+
| IALLGATHER            | sched_ring                   | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHER            | sched_brucks                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHER            | sched_recursive_doubling     | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHER            | gentran_ring                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHER            | gentran_brucks               | MPIR_CVAR_IALLGATHER_BRUCKS_KVAL   | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHER            | gentran_recexch_doubling     | MPIR_CVAR_IALLGATHER_RECEXCH_KVAL  | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHER            | gentran_recexch_halving      | MPIR_CVAR_IALLGATHER_RECEXCH_KVAL  | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHERV           | sched_ring                   | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHERV           | sched_brucks                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHERV           | sched_recursive_doubling     | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHERV           | gentran_ring                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHERV           | gentran_brucks               | MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL  | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHERV           | gentran_recexch_doubling     | MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLGATHERV           | gentran_recexch_halving      | MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | sched_naive                  | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | sched_smp                    | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | sched_recursive_doubling     | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | sched_reduce_scatter         | NONE                               |                        |
|                       | allgather                    |                                    |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | gentran_recexch_single_buffer| MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL  | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | gentran_recexch_multiple_    | MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL  | k >= 2, default k=2    |
|                       | buffer                       |                                    |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | gentran_ring                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLREDUCE            | gentran_tree                 | MPIR_CVAR_IALLREDUCE_TREE_KVAL,    | k >=2, default k=2     |
|                       |                              | MPIR_CVAR_IALLREDUCE_TREE_TYPE=    | Pipeline chunksize:    |
|                       |                              | kary/knomial_1/knomial_2,          | Max chunk size (in     |
|                       |                              | MPIR_CVAR_IALLREDUCE_TREE_BUFFER_  | bytes)  Default: 0B    |
|                       |                              | PER_CHILD=0/1, MPIR_CVAR_IALLREDUCE| Tree buffer per child: |
|                       |                              | _TREE_PIPELINE_CHUNK_SIZE,         | If set to 1, algorithm |
|                       |                              |                                    | will allocate dedicated|
|                       |                              |                                    | buffer for every child |
|                       |                              |                                    | it receives data from. |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALL             | sched_brucks                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALL             | sched_inplace                | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALL             | sched_permuted_sendrecv      | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALL             | gentran_ring                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALL             | gentran_brucks               | MPIR_CVAR_IALLTOALL_BRUCKS_KVAL    | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALL             | gentran_scattered            | MPIR_CVAR_IALLTOALL_SCATTERED_     | Batch size:max # of    |
|                       |                              | BATCH_SIZE, MPIR_CVAR_IALLTOALL_   | outstanding sends/     |
|                       |                              | SCATTERED_OUTSTANDING_TASKS        | recvs posted at a time |
|                       |                              |                                    | Outstanding tasks: #   |
|                       |                              |                                    | of send/recv tasks     |
|                       |                              |                                    | that algo waits for    |
|                       |                              |                                    | completion before      |
|                       |                              |                                    | posting next batch     |
|                       |                              |                                    | send/recvs of that size|
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLV            | sched_blocked                | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLV            | sched_inplace                | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLV            | sched_pairwise_exchange      | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLV            | gentran_blocked              | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLV            | gentran_inplace              | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLV            | gentran_scattered            | MPIR_CVAR_IALLTOALLV_SCATTERED_    | Batch size: max #      |
|                       |                              | OUTSTANDING_TASKS, MPIR_CVAR_      | of outstanding sends/  |
|                       |                              | IALLTOALLV_SCATTERED_BATCH_SIZE    | recvs posted at a time.|
|                       |                              |                                    | Outstanding tasks: #   |
|                       |                              |                                    | of send/recv tasks     |
|                       |                              |                                    | that algo waits for    |
|                       |                              |                                    | completion before      |
|                       |                              |                                    | posting next batch     |
|                       |                              |                                    | send/recvs of that size|
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLW            | sched_blocked                | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLW            | sched_inplace                | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLW            | gentran_blocked              | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IALLTOALLW            | gentran_inplace              | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBARRIER              | sched_auto                   | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBARRIER              | sched_recursive_doubling     | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBARRIER              | gentran_recexch              | MPIR_CVAR_IBARRIER_RECEXCH_KVAL    | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBARRIER              | gentran_dissem               | MPIR_CVAR_IBARRIER_DISSEM_KVAL     | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | sched_binomial               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | sched_smp                    | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | sched_scatter_recursive_     | NONE                               |                        |
|                       | doubling_allgather           |                                    |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | sched_scatter_ring_allgather | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | gentran_tree                 | MPIR_CVAR_IBCAST_TREE_KVAL, MPIR_  | k >= 2, default k=2    |
|                       |                              | CVAR_IBCAST_TREE_TYPE = kary/      | Pipeline chunksize:    |
|                       |                              | knomial_1/knomial_2, MPIR_CVAR_    | Max chunk size (in     |
|                       |                              | IBCAST_TREE_PIPELINE_CHUNK_SIZE    | bytes)  Default: 0B    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | gentran_ring                 | MPIR_CVAR_IBCAST_RING_CHUNK_SIZE   | Max chunk size (in     |
|                       |                              |                                    | bytes)  Default: 0B    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | gentran_scatterv_recexch_    | MPIR_CVAR_IBCAST_SCATTERV_KVAL,    | k >= 2, default k=2    |
|                       | allgatherv                   | MPIR_CVAR_IBCAST_ALLGATHERV_       |                        |
|                       |                              | RECEXCH_KVAL                       |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IBCAST                | gentran_scatterv_ring_       | MPIR_CVAR_IBCAST_SCATTERV_KVAL     | k >= 2, default k=2    |
|                       | allgatherv                   |                                    |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IGATHER               | sched_binomial               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IGATHER               | gentran_tree                 | MPIR_CVAR_IGATHER_TREE_KVAL        | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IGATHERV              | sched_linear                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IGATHERV              | gentran_linear               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLGATHER   | sched_linear                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLGATHER   | gentran_linear               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLGATHERV  | sched_linear                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLGATHERV  | gentran_linear               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLTOALL    | sched_linear                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLTOALL    | gentran_linear               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLTOALLV   | sched_linear                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLTOALLV   | gentran_linear               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLTOALLW   | sched_linear                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| INEIGHBOR_ALLTOALLW   | gentran_linear               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE               | sched_smp                    | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE               | sched_binomial               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE               | sched_reduce_scatter_gather  | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE               | gentran_ring                 | MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE  | Max chunk size (in     |
|                       |                              |                                    | bytes) Default: 0B     |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE               | gentran_tree                 | MPIR_CVAR_IREDUCE_TREE_KVAL, MPIR_ | k >= 2, default k=2    |
|                       |                              | CVAR_IREDUCE_TREE_TYPE = kary/     | Pipeline chunksize:    |
|                       |                              | knomial_1/knomial_2/topology_aware/| Max chunk size (in     |
|                       |                              | topology_aware_k/topology_wave,    | bytes)  Default: 0B    |
|                       |                              | MPIR_CVAR_IREDUCE_TREE_PIPELINE_   | Tree buffer per child: |
|                       |                              | CHUNK_SIZE,                        | If set to 1, algorithm |
|                       |                              | MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_ | will allocate dedicated|
|                       |                              | CHILD                              | buffer for every child |
|                       |                              |                                    | it receives data from. |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER       | sched_noncommutative         | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER       | sched_recursive_doubling     | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER       | sched_pairwise               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER       | sched_recursive_halving      | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER       | gentran_recexch              | MPIR_CVAR_IREDUCE_SCATTER_         | k >=2, default k=2.    |
|                       |                              | RECEXCH_KVAL                       |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER_BLOCK | sched_noncommutative         | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER_BLOCK | sched_recursive_doubling     | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER_BLOCK | sched_pairwise               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER_BLOCK | sched_recursive_haLving      | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| IREDUCE_SCATTER_BLOCK | gentran_recexch              | MPIR_CVAR_IREDUCE_SCATTER_BLOCK_   | k>=2, default k=2.     |
|                       |                              | RECEXCH_KVAL                       |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| ISCAN                 | sched_smp                    | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| ISCAN                 | sched_recursive_doubling     | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| ISCAN                 | gentran_recursive_doubling   | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| ISCATTER              | sched_binomial               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| ISCATTER              | gentran_tree                 | MPIR_CVAR_ISCATTER_TREE_KVAL       | k >= 2, default k=2    |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| ISCATTERV             | sched_linear                 | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
| ISCATTERV             | gentran_linear               | NONE                               |                        |
+ --------------------- + ---------------------------- + ---------------------------------- + -----------------------+
```

## 2.5. Persistent collectives

Persistent collectives is a new API that has been approved to appear
in the next MPI 4.0 standard. This new API is helpful in scenarios
where a non-blocking collective with same send/receive buffers is
called multiple times in an application. The advantage of the
persistent API is that it provides information to the MPI runtime
that this collective will be called multiple times with the same
arguments. As a result, the MPI runtime can spend some additional
time to optimize the collective (such as algorithm related optimizations
or schedule generation) since that extra time will be amortized over
the multiple invocations of the persistent collective.

The persistent collectives API use the schedule created with DAG
based scheduler (gentran) for non-blocking collectives. Thus, the
algorithms labeled gentran_* in Table III in Section 2.4 can be used
to select the desired algorithm for a particular collective.
Currently, MPICH only supports the persistent collective API for
BCAST, REDUCE, ALLREDUCE, ALLTOALL, ALLGATHER and ALLGATHER
collectives. Support for the rest of the collectives will be added
on a later release.


### 2.6. Intra-node collective algorithms

MPICH uses the environment variable
`MPIR_CVAR_<collname>_POSIX_INTRA_ALGORITHM` to indicate the algorithms
that can be used for intra-node collectives. The possible options for
this CVAR are:
* `auto`: Auto selection by using the JSON file. This is the default value.

* `mpir`  : Implements the algorithm specified for the blocking/non-blocking
collectives in Section 2.3 and 2.4. With this option, the same algorithm
and parameter values are used for both the intra-node and inter-node portion
of the collective. Notice that with non-blocking collectives, only the
algorithms using the linear scheduler are supported for intra-node
collectives.

* `release_gather`: Implements optimized intra-node collectives taking
advantage of the shared memory in the node, as described in the paper
by Jain et al. (https://dl.acm.org/doi/10.5555/3291656.3291695).
When the CVAR is set to use release-gather algorithms, additional
CVARS can be used to tune some of the parameter values utilized by
these collectives, as shown in Table IV.

* `stream_read`: Implements optimized intra-node collectives utilizing
the XeLinks between the Intel GPUs. Currently, this algorithm only
supports MPI_Bcast and MPI_Alltoall. When the CVAR is set to use
stream-read algorithms, additional CVARS is available to tune performance,
as shown in Table V.

#### Table IV: Algorithms available for release-gather collectives

```
+ ========== + =============================================== + ==================================== +
| Collective | Name of CVAR                                    |  Notes                               |
+ ========== + =============================================== + ==================================== +
| BCAST/     | MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE/    | Buffer Size in Bytes. Default:       |
| REDUCE     | MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE    | 32768 B                              |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
| BCAST/     | MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS/            | Number of cells used for pipelining. |
| REDUCE     | MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS            | Default: 4                           |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
| BCAST      | MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE/            | kary (default), knomial_1, or        |
| REDUCE     | MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE            | knomial_2                            |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
| BCAST      | MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL             | Radix of the tree. Default: 64       |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
| REDUCE     | MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL            | Radix of the tree. Default: 4        |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
| BCAST/     | MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES | Enables Topology-aware trees (k-ary  |
| REDUCE     |                                                 | and knomial_1). Enabled by default   |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
| BCAST/     | MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE               | Max Memory Size (in KB). Default:    |
| REDUCE     |                                                 | 65536 KB                             |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
```

#### Table V: Parameters available for stream-read GPU collectives

```
+ ========== + =============================================== + ==================================== +
| Collective | Name of CVAR                                    |  Notes                               |
+ ========== + =============================================== + ==================================== +
| BCAST      | MPIR_CVAR_BCAST_PC_READ_MSG_SIZE_THRESHOLD      | Invoke stream read bcast only when   |
|            |                                                 | the messages are larger than this    |
|            |                                                 | threshold. Default: 1024 B           |
+ ---------- + ----------------------------------------------- + ------------------------------------ +
```

## 3. Multi-threading

MPI provides four different threading modes: `MPI_THREAD_SINGLE`,
`MPI_THREAD_FUNNELED`, `MPI_THREAD_SERIALIZED`, and `MPI_THREAD_MULTIPLE`.
MPI_THREAD_MULTIPLE is the most advanced threading mode and the focus of this
Section. With this mode, an application runs with several threads, and any
thread can make calls to MPI at any time. Thus, the MPI library needs to protect
access to its internal data structures appropriately. It also tries to exploit
concurrency by taking advantage of the availability of multiple hardware
contexts in the network card, which we call `VCIs` (Virtual Communication
Interfaces).

MPICH provides two threading synchronization strategies. One strategy is to use
a `global` coarse-grained lock to protect every MPI call that accesses shared
state inside the MPI runtime. With this model, if two threads call concurrently
the MPI runtime, the runtime will serialize the calls. Another strategy uses a
fine-grained lock to protect the access to each network context or VCI. With
this approach, threads trying to access the same VCI get serialized, but threads
accessing different VCIs can execute in parallel. With the fine-grained locking
model, MPICH supports a `direct` mode where the MPI runtime itself grabs a per
VCI lock and a `lockless` mode where MPI does not grab a lock and relies on the
network provider to provide fine grain synchronization (MPICH uses the
FI_THREAD_SAFE mode from OFI to support the `lockless` mode).

A threading synchronization strategy can be selected at the configuration time
as:
  --enable-thread-cs=global
  to select coarse-grained locking, and
  --enable-thread-cs=per-vci
  to select fine-grained locking.

Under fine-grained locking, a specific locking mode at configuration time as:
  --enable-ch4-mt=<mode>
  mode can be one of direct, lockless and runtime.

If configuration is done with --enable-ch4-mt=runtime, user can select a
threading mode at runtime using the environment MPIR_CVAR_CH4_MT_MODEL
(described below in Section 1.1).

### 3.1 CVARs (Environment Variables)

MPICH users can use the CVAR MPIR_CVAR_CH4_MT_MODEL to select at runtime among
the threading synchronization models, `global`, `direct`, or `lockless`. If the
CVAR is not set, the default model is `direct`.

e.g., export MPIR_CVAR_CH4_MT_MODEL=direct

Users can specify the number of VCIs to use by setting the environment variable
`MPIR_CVAR_CH4_NUM_VCIS=<number_vcis>`, where the value `<num_vcis>` should be
less or equal than the number of network contexts provided by the OFI provider.
This number can be retrieved from the OFI provider. Running the  `fi_info -v`
command should return the number of transmit contexts (max_ep_tx_ctx) and
receive contexts (max_ep_rx_ctx) it supports. MPICH uses the minimum of
max_ep_tx_ctx and max_ep_rx_ctx as the maximum number of VCIs it can support. If
the system has multiple NICS, this value is the `minimum across all the NICS`.

e.g., export MPIR_CVAR_CH4_NUM_VCIS=16

When multiple VCIs are used, MPICH tries to make per-vci progress to avoid lock
contention inside the progress engine. However, it also progresses all VCIs,
i.e., make global progress, once in a while to avoid potential deadlocks. Such
global progress is turned on by default but can be turned off by setting
`MPIR_CVAR_CH4_GLOBAL_PROGRESS=0`, which can improve performance under the use
of multiple VCIs. However, caution should be taken as it can result in a deadlock.

### 3.2 Communicator Info Hints

When selecting the fine-grain modes (`direct` or `lockless`), messages sent
using different communicators can be sent/received using different VCIs.
However, messages using the same communicator may have to be sent/received
through the same VCI, to guarantee the ordering semantics imposed by the MPI
standard. However, messages sent through the same communicator can be
sent/received through different VCIs if the MPI runtime knows that wildcards
will not be used on that communicator. Thus, MPICH implements the MPI 4.0
per-communicator info hints and recommend users to use them to  indicate the
MPI runtime when wildcards are not used, specially when using `direct` or
`lockless` threading modes with multiple VCIs if the threads use the same
communicator.
Here are the standard hints from the MPI 4.0 standard that are supported
(we recommend using both info hints if the application is not using wildcards):

* `mpi_assert_no_any_source`: Set this info hint if the application does
   not use the wildcard MPI_ANY_SOURCE on the given communicator.
* `mpi_assert_no_any_tag`: Set this hint if the application does not use
   MPI_ANY_TAG on the given communciator.

MPICH also supports a non-standard per-communicator info hint that provides the
user with more control of a communicator-VCI mapping:
* `vci`: Specific sender and receiver VCI can be selected for each communicator
   using this hint. Applications can utilize this feature to control how
   messages are spread across the VCIs.
For instance, programmers can write the applications so that pairs of threads
communicate by using different communicator, where each communicator can be
mapped to a different VCI through the `vci` info hint. Valid values for this
info hints are 0 to (total vcis - 1). This hint is also utilized by MPICH to
select a progress thread to process reduction collective operation.

## 4. Shared Memory

The shared memory code has a few environment variables to tune its behavior.
The most important of these focuses on MPICH behavior when sending large
messages. XPMEM is the module used to send large messages. It requires a
kernel module and user-level library, but even if these are available,
it can still be disabled at runtime.

* `MPIR_CVAR_CH4_XPMEM_ENABLE`: If set to 0, the XPMEM module will not be
used. The default is 1.
* `MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD`: If a send message size is greater
than or equal to this threshold (in bytes), then XPMEM will be used to
send messages (assuming it is enabled by `MPIR_CVAR_CH4_XPMEM_ENABLE`.
The default value is 64KB.

There is an additional CVAR (`MPIR_CVAR_GENQ_SHMEM_POOL_FREE_QUEUE_SENDER_SIDE`)
to turn on recevier-side queuing when using iqueue. The genq shmem code
allocates pools of cells on each process and, when needed, a cell is removed
from the pool and passed to another process. This can happen by either
removing a cell from the pool of the sending process or from the pool of
the receiving process. This CVAR determines which pool to use. If true, the
cell will come from the sender-side. If false, the cell will come from the
receiver-side.

There are specific advantages of using receiver-side cells when combined with
the "avx" fast configure option, which allows MPICH to use AVX streaming copy
intrintrinsics, when available, to avoid polluting the cache of the sender
with the data being copied to the receiver. Using receiver-side cells does
have the trade-off of requiring an MPMC lock for the free queue rather than
an MPSC lock, which is used for sender-side cells. Initial performance
analysis shows that using the MPMC lock in this case had no significant
performance loss.

By default, the queue will continue to use sender-side queues until the
performance impact is verified.

**5. RMA**

MPICH exposes two configuration variables that might be important for users.

* `MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX`: If set to positive, this CVAR
specifies the maximum number of transmit contexts RMA can utilize with
a scalable endpoint. This value is effective only when the CVAR
`MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS` to use scalable endpoints
is enabled; otherwise, the RMA CVAR is ignored.

* `MPIR_CVAR_CH4_RMA_MEM_EFFICIENT`:  This CVAR allows the user to trade
memory savings with some additional run time overhead. When the CVAR is
set to 1, the memory-saving mode is on, and the allocated target objects
that the MPICH runtime caches are freed at the the end of the epoch. If
the CVAR is set to 0, performance-efficient mode is on, and all allocated
target objects are cached and freed only at win_finalize.


## 6. Multi-NIC

MPICH supports multiple NICs per node, i.e., if there are more than one NIC on a 
node, MPICH utilizes all the available NICs for communication to improve the 
overall bandwidth and message rate. For this, MPICH maps different MPI ranks on a 
node to different NICs. For example, if four ranks are launched on a node with four 
NICs, each of them will use a different NIC. By default, each rank is assigned a 
different preferred NIC (based on its affinity to the NICs) that is used for all its 
communication. MPICH also performs a balanced distribution of the ranks across NICs. 
For better performance, the users should provide the pinning information of the MPI ranks.

In order to utilize all the available NICs on a node, MPICH supports additional 
optimizations, like striping and hashing. These optimizations allow a single rank to 
utilize multiple NICs on a node and they are useful when there are unutilized NICs 
(not assigned as a preferred NIC of any rank) on the node. The goal of the striping 
optimization is to split a message into chunks and send each chunk through a different NIC. 
On the other hand, hashing allows a rank to send and receive full messages through different 
NICs. With hashing, the sender and receiver select the NIC to send/receive to/receive from 
among all available NICs. By default, all the available NICs on a node are used for striping 
and hashing (when enabled). But the users can also use a CVAR or communicator info hint to 
select the number of NICs to be used for these optimizations. There are also info hints to 
enable/disable these optimizations per communicator. The users can also use an info hint to 
assign a NIC per communicator. The related CVARs and info hints are described below:

### 6.1 CVARs (Environment Variables)

`MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_STRIPING`: This CVAR enables striping for all the communicators in 
the application. The default value of this CVAR is 0 (i.e., striping is disabled by default). 
Striping can be enabled on particular communicators by using the `enable_striping` info hint. 
If this CVAR is disabled, the application cannot use striping on any communicator.

`MPIR_CVAR_CH4_OFI_MULTI_NIC_STRIPING_THRESHOLD`: This CVAR determines the message size at (or above) 
which we start striping the data across multiple NICs. Currently, the default value of this 
CVAR is 1MB (i.e., messages >= 1MB will use the striping optimization) when striping is ON.

`MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_HASHING`: This CVAR enables hashing for all the communicators in the 
application. The default value of this CVAR is 0 (i.e., hashing is disabled by default). Hashing 
can be enabled on particular communicators by using the `enable_hasing` info hint. If this CVAR is 
disabled, the application cannot use hashing on any communicator.

`MPIR_CVAR_CH4_OFI_MAX_NICS`: This CVAR determines the number of physical NICs to use. The default 
is -1 which means utilizing all available NICs. A value strictly less than -1 or equal to 0 will be 
mapped to using one NIC which is the first provider in the list of providers returned by 
`fi_getinfo()`. There is an upper bound for this value: the compile-time constant, 
`MPIDI_OFI_MAX_NICS`.

`MPIR_CVAR_DEBUG_SUMMARY`: Prints out lots of debug information at initialization time to help find 
problems with OFI provider selection.


### 6.2 Communicator Info Hints

`enable_striping`: This info hint overwrites the environment variable, `MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_STRIPING`. 
When used, it enables or disables the striping feature for the specified communicator.

`enable_hashing`: This info hint overwrites the environment variable, `MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_HASHING`. 
When used, it enables or disables the hashing feature for the specified communicator.

`num_nics` (output only and only set of `MPI_INFO_ENV`): It indicates the number of physical NICs on the system.

`num_close_nics` (output only and only set of `MPI_INFO_ENV`): This info hint offers the user a way to get the number 
of close NICs (NICs on the same socket) to the calling process.

`multi_nic_pref_nic`: This info hint indicates that all messages over the specified communicator will use this NIC. The 
value given for this should be a number between 0 and `num_nics - 1`. If a value < 0 is given, then the value is ignored. 
If a value >= `num_nics` is given, the value is wrapped around via modulus operator (`multi_nic_pref_nic % num_nics`). 
The NIC represented by index "0" is different for each process. MPICH will automatically assign the NICs in a round robin 
fashion for each MPI process. As an example, here is a table with an array of NICs in order of preference for each MPI process 
where we have 4 MPI ranks with 4 NICs across 2 CPU sockets:

|MPI Process|0|1|2|3|
|----|---|---|---|---|
|P-0 | 0 | 1 | 2 | 3|
|P-1 | 1 | 0 | 2 | 3|
|P-2 | 2 | 3 | 0 | 1|
|P-3 | 3 | 2 | 0 | 1|

Note that nics that are local to a different socket than the application's process will not be reordered.

In general, if a rank wants messages sent with different communicators to go through different NICs, the rank should use different 
communicators and use an info hint with a different `multi_nic_pref_nic ` for each communicator. Similarly, for multi-threaded 
applications to take advantage of this feature, the user must create one communicator per thread with a different `multi_nic_pref_nic`, 
so that each thread is assigned a different close NIC to transfer its data.

### 6.3 MPI_T Performance Variables (PVARs) Support

Performance variables (PVARs) related to Multi-NIC usage have been added in MPICH and the appropriate 
instrumentation has been added to keep track of the number of bytes sent and received through each 
Network Interface Card (NIC). Notice that PVARS are currently only available through our debug build. 
The new PVARs to track the amount of bytes sent and received are shown Table V.

#### Table V: PVARS to track bytes sent and received

```
+ ================================ + ================ + ================= + ==== + ======= +
| Variable Name                    | Class            | Handle            | Type | Storage |
+ ================================ + ================ + ================= + ==== + ======= +
| nic_sent_bytes_count             | Counter (Array)  | Non-continuous    | SUM  | Static  |
| nic_recvd_bytes_count            | Counter (Array)  | Non-continuous    | SUM  | Static  |
+ -------------------------------- + ---------------- + ------------------------ + ------- +
```

The new PVARs to track the amount of data sent using the striping optimization (when messages are 
sufficiently large) are shown in Table VI. Notice that with striping, an initial handshake and small data 
transfer is sent via fi_send whereas most of the data transfer takes place through fi_read call.
Hence, the sum of the two PVARS in Table VI represent the total amount of data transferred through striping.

#### Table VI: PVARS to track amount of data sent through striping**

```
+ ================================ + ================ + ================= + ==== + ======= +
| Variable Name                    | Class            | Handle            | Type | Storage |
+ ================================ + ================ + ================= + ==== + ======= +
| striped_nic_sent_bytes_count     | Counter (Array)  | Non-continuous    | SUM  | Static  |
| striped_nic_recvd_bytes_count    | Counter (Array)  | Non-continuous    | SUM  | Static  |
+ -------------------------------- + ---------------- + ------------------------ + ------- +
```

The new PVARs to track number of bytes sent and received through RMA calls are shown in Table VII.

**Table VII: PVARS to track amount of data sent through RMA calls**

```
+ ================================ + ================ + ================= + ==== + ======= +
| Variable Name                    | Class            | Handle            | Type | Storage |
+ ================================ + ================ + ================= + ==== + ======= +
| rma_pref_phy_nic_put_bytes_count | Counter (Array)  | Non-continuous    | SUM  | Static  |
| rma_pref_phy_nic_get_bytes_count | Counter (Array)  | Non-continuous    | SUM  | Static  |
+ -------------------------------- + ---------------- + ------------------------ + ------- +
```

Following are the steps to use PVARs in an application:

* Initialize MPI_T interface using `MPI_T_init_thread` routine.
* Get the number of performance variables (PVARs) available in MPICH using `MPI_T_cvar_get_num` routine.
* Get the information about the performance variables (eg., name, class, datatype, etc.) and select the 
required ones using `MPI_T_pvar_get_info` routine. In this case, the newly added Multi-NIC PVARs.
* Create a new session for accessing these new performance variables using the `MPI_T_pvar_session_create` routine.
* Allocate the handle for each performance variable using the `MPI_T_pvar_handle_alloc` routine.
* Start the session for each performance variable using the `MPI_T_pvar_start` routine.
* Stop the session for each performance variable once the required MPI calls have been made to measure 
the Multi-NIC usage using `MPI_T_pvar_stop` routine.
* Deallocate the handles created using `MPI_T_pvar_handle_free` routine.
* Deallocate the entire session using `MPI_T_pvar_session_free` routine.
* Tear down the MPI_T interface using the `MPI_T_finalize` routine.

## 7. GPU Support

MPICH has now support for Intel GPUs and MPI calls can use a pointer to
a buffer allocated in the GPU. Similarly, MPI works with buffers that
are allocated using unified shared memory, where the buffer can migrate
between the Host and the GPU. Notice that the MPI call will execute from
the host, so the GPU kernels cannot contain MPI calls. Support for unified
shared memory is limited to buffers that migrate between the host and a
single device (documentation will be updated as the development progresses).

MPICH works with multi-GPU nodes. However, the application is responsible
for distributing the ranks among the multiple GPUs on the system.

MPICH supports native reduction operations that execute on the GPU for
MPI collectives and RMA compute operations. This feature is enabled by
default. There are two environment variables to disable native reductions
for collectives and RMA:

* `MPIR_CVAR_ENABLE_YAKSA_REDUCTION`: This CVAR determines whether collective
reduction will be performed in GPU or not. The default value of the CVAR is 1
which means if the user buffer is in GPU, the reduction will also execute in
the GPU. The users can disable reduction in GPU by using
`MPIR_CVAR_ENABLE_YAKSA_REDUCTION = 0`; this enables the fallback path
(host-based) for reduction.

As of now, Intel GPUs do not support operations for all datatypes. For
example, DG1 does not support operations with ` double` type (64 bits);
ATS supports operations with `double` but not `long double` (128 bits).
We have added environment variables for the users to enable support for
different datatypes (64 bits, 128 bits) based on the GPU platform they
are using. These CVARs are:
* `MPIR_CVAR_GPU_DOUBLE_SUPPORT`: This CVAR determines whether `double`
(64 bit) type is supported in GPU or not. The default value of the CVAR
is 0 which means `double` is not supported in GPU. If the user's platform
has ATS that supports operations with 64 bit values, then the user should
use `MPIR_CVAR_GPU_DOUBLE_SUPPORT = 1`; reductions with `double` types
will then happen in GPU.
* `MPIR_CVAR_GPU_LONG DOUBLE_SUPPORT`: This CVAR determines whether
`long double` (128 bit) type is supported in GPU or not. The default value
of the CVAR is 0 which means `long double` is not supported in GPU. If the
user's platform has a GPU that supports operations with 128 bit values,
then the user should use `MPIR_CVAR_GPU_LONG_DOUBLE_SUPPORT = 1`;
reductions with `long double` types will then happen in GPU.
 * `MPIR_CVAR_YAKSA_COMPLEX_SUPPORT`: This CVAR determines whether
 operations with `complex` types are supported in GPU or not. The default
 value of the CVAR is 0 which means `complex` is not supported in GPU. If
 the user's platform has ATS that supports operations with 64 bit values,
 then the user should use `MPIR_CVAR_YAKSA_COMPLEX_SUPPORT = 1`;
 reductions with `float _Complex` or similar 64 bit `complex` values will
 then execute in GPU.

MPICH now supports GPU-to-GPU Interprocess Communication (IPC) with
builtin datatypes. This is enabled by default for message sizes above a
given threshold. We have added an environment variable to allow the user
to modify this threshold:

* `MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD`: Specifies the data size
threshold (in bytes) for which GPU IPC is used. Default is 1. Use -1 to
disable IPC.

* `MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE`: Selects an implementation for
ZE shareable IPC handle, it can be `drmfd` (the default) which uses device
fd-based shareable IPC handle, or `pidfd`, which uses an implementation
based on pidfd_getfd syscall (only available after Linux kernel 5.6.0).

* `MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE`: select engine type to do the IPC
copying, can be 0|1|2|auto. Default is using main copy engine. "auto"
mode will use main copy engine when two buffers are on the same root
device, otherwise use link copy engine.

* `MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL`:  choose read/write/auto
protocol for IPC. Default is read. Auto will prefer write protocol when
remote device is visible, otherwise fallback to read.

MPI_Get and MPI_Put also supports the IPC. For contiguous datatypes, an
option is added to bypass Yaksa:

* `MPIR_CVAR_CH4_IPC_GPU_RMA_ENGINE_TYPE`: select engine type to do the IPC
copying, can be 0|1|2|auto|yaksa. Default is "auto" which will use main
copy engine when two buffers are on the same root device, otherwise use
link copy engine. "yaksa" is to fallback to using Yaksa

MPICH supports fast memory copying using mmap mechanism (bypassing kernel
launching), which is optimized for very small messages:

* `MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD`: make sure this CVAR is set to 1
to enable IPC. Fast memory copy depends on IPC enabled.

* `MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE`: set the max message size
(in bytes) to use fast memory copying.

GPU pipeline uses host buffer and pipelining technique to send internode
messages instead of GPU RDMA. To enable this mode, use the following two
CVARs:

* `MPIR_CVAR_CH4_OFI_ENABLE_GPU_PIPELINE`: This CVAR enables GPU pipeline
for inter-node pt2pt messages
* `MPIR_CVAR_CH4_OFI_GPU_PIPELINE_THRESHOLD`: The threshold to start using
GPU pipelining. Default is 1MB.

* `MPIR_CVAR_CH4_OFI_GPU_PIPELINE_BUFFER_SZ`: Specifies the chunk size
(in bytes) for GPU pipeline data transfer.

* `MPIR_CVAR_CH4_OFI_GPU_PIPELINE_NUM_BUFFERS_PER_CHUNK`: Specifies the
number of buffers for GPU pipeline data transfer in each block/chunk of
the pool.

* `MPIR_CVAR_CH4_OFI_GPU_PIPELINE_MAX_NUM_BUFFERS`: Specifies the maximum
total number of buffers MPICH buffer pool can allocate.

* `MPIR_CVAR_CH4_OFI_GPU_PIPELINE_D2H_ENGINE_TYPE`: Specify engine type
for copying from device to host (sender side), default 0

* `MPIR_CVAR_CH4_OFI_GPU_PIPELINE_H2D_ENGINE_TYPE`: Specify engine type
for copying from host to device (receiver side), default 0

To enable GPU Direct RDMA support for pt2pt communication, use the
following CVARs:
* `MPIR_CVAR_CH4_OFI_ENABLE_HMEM`: This CVAR with a value of `1` enables
inter-node GPU Direct RDMA pt2pt communication.
* `MPIR_CVAR_CH4_OFI_GPU_RDMA_THRESHOLD`: This is the message size
threshold for using GPU Direct RDMA. The default value of this CVAR
is 0B. Messages  >= 0B will go through GPU Direct RDMA. The users can
tune this threshold for different providers.

The fallback path is used when GPU messages are copied to host buffer
for communication:

* `MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE`: can be -1|0|1|2. Default
is 1, which is the main copy engine. -1 will use yaksa for copying

* `MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE`:  can be -1|0|1|2.
Default is 1, which is the main copy engine. -1 will use yaksa for
copying

In some circumstances, one may want to disable the GPU support:

* `MPIR_CVAR_ENABLE_GPU`: The default value of this CVAR is 1. Set to
0 to disable GPU support including GPU initialization.

Misc:

* `MPIR_CVAR_GPU_USE_IMMEDIATE_COMMAND_LIST`:  to use immediate command
lists, instead of normal command lists plus command queues

* `MPIR_CVAR_DEBUG_SUMMARY` + `MPIR_CVAR_ENABLE_GPU`: when enabled together,
print GPU hardware information

* CVARs related to the intra-node GPU collectives are introduced in
Section 2.6.
