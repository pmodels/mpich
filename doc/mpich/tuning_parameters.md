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
https://wiki.mpich.org/mpich/index.php/Using_the_Hydra_Process_Manager#Process-core_Binding.

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
