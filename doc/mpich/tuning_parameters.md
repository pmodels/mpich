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

This following section of this document provides information about threading
optimizations (Section 1).

## 1. Multi-threading

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

### 1.1 CVARs (Environment Variables)

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

### 1.2 Communicator Info Hints

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

## 2. Multi-NIC

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

### 2.1 CVARs (Environment Variables)

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


### 2.2 Communicator Info Hints

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

### 2.3 MPI_T Performance Variables (PVARs) Support

Performance variables (PVARs) related to Multi-NIC usage have been added in MPICH and the appropriate 
instrumentation has been added to keep track of the number of bytes sent and received through each 
Network Interface Card (NIC). Notice that PVARS are currently only available through our debug build. 
The new PVARs to track the amount of bytes sent and received are shown Table V.

**Table V: PVARS to track bytes sent and received**

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

**Table VI: PVARS to track amount of data sent through striping**

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
