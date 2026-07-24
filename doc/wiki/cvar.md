**MPIR_CVAR_ABORT_ON_LEAKED_HANDLES** - MEMORY

Controls whether MPI aborts during finalization when leaked MPI object handles are detected. It is used by the handle-allocation debug finalization check, which is compiled only when MPICH is configured with `--enable-g=handlealloc` or a stronger debug level. When active and leaked handles are found, this CVAR enables aborting instead of only reporting the leaked handle counts.

* false - default, report leaked handles without aborting.
* true - abort when leaked handles are detected during finalization.

Used in functions: MPIR_check_handles_on_finalize

**MPIR_CVAR_ALLGATHERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Allgatherv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Allgatherv.

* true - default, allows the device to override MPIR-level `MPI_Allgatherv` collective algorithms.
* false - disables the device override for `MPI_Allgatherv`.

Used in functions: MPI_Allgatherv

**MPIR_CVAR_ALLGATHERV_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Allgatherv_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Allgatherv_init.

* true - default, allows the device to override MPIR-level `MPI_Allgatherv_init` collective algorithms.
* false - disables the device override for `MPI_Allgatherv_init`.

Used in functions: MPI_Allgatherv_init

**MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Allgatherv algorithm. Automatic selection may be overridden by `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which inter-communicator Allgatherv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden by a collective selection tuning JSON file.
* nb - forces the nonblocking algorithm.
* remote_gather_local_bcast - forces the remote-gather-local-bcast algorithm.

Used in functions: MPI_Allgatherv

**MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Allgatherv algorithm. Automatic selection may be overridden by `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Allgatherv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden by a collective selection tuning JSON file.
* brucks - forces the Brucks algorithm.
* nb - forces the nonblocking algorithm.
* recursive_doubling - forces the recursive-doubling algorithm.
* ring - forces the ring algorithm.

Used in functions: MPI_Allgatherv

**MPIR_CVAR_ALLGATHERV_IPC_READ_MSG_SIZE_THRESHOLD** - COLLECTIVE

Sets the message-size threshold, in bytes, for using POSIX GPU IPC read allgatherv. It is used only when the POSIX GPU IPC read allgatherv path is selected and GPU shared-memory support is enabled. The path also requires matching send and receive datatypes, a contiguous datatype, and GPU IPC-capable send buffers from all ranks; otherwise it falls back to the MPIR Allgatherv implementation. When active, this CVAR sets the minimum allgatherv send payload size that can use GPU IPC read instead of fallback allgatherv.

* integer - default is 256, represents the allgatherv GPU IPC read message-size threshold in bytes.

Used in functions: MPIDI_POSIX_mpi_allgatherv_gpu_ipc_read

**MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE** - COLLECTIVE

Sets the target pipeline message size, in bytes, for chunking data in Allgatherv ring algorithms. It is used in the blocking intra-communicator ring algorithm when the largest receive contribution is larger than this size and the CVAR is positive; otherwise that path sends each contribution without pipeline chunking. It is also used by the sched-based nonblocking intra-communicator ring algorithm to set the chunk size for scheduled ring sends and receives. The CVAR is effective only when these ring implementations are selected, such as by forcing the ring algorithm or by internal collective selection.

* 0 or negative integer - Disables pipeline chunking in the blocking ring algorithm and disables CVAR-driven enlargement of the sched-based nonblocking ring chunk size.
* positive integer - default is 32768, represents the target pipeline chunk size in bytes for Allgatherv ring transfers.

Used in functions: MPIR_Allgatherv_intra_ring, MPIR_Iallgatherv_intra_sched_ring

**MPIR_CVAR_ALLGATHERV_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Selects the algorithm used for POSIX shared-memory intranode allgatherv. It is used in the POSIX allgatherv implementation and does not use automatic collective selection or depend on another CVAR in `src/mpid/ch4/shm/posix/posix_coll.h`. When active, this CVAR sets whether POSIX intranode allgatherv uses MPIR fallback or the POSIX GPU IPC read allgatherv path.

* mpir - default, use the MPIR allgatherv implementation.
* ipc_read - use the POSIX GPU IPC read allgatherv path.

Used in functions: MPIDI_POSIX_mpi_allgatherv

**MPIR_CVAR_ALLGATHER_BRUCKS_KVAL** - COLLECTIVE

Sets the radix value for the generic transport Brucks-based Allgather algorithm. It is used when the Allgather radix-k Brucks algorithm is selected, such as by forcing `MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM` to `k_brucks` or by internal collective selection. When active, it controls the radix used by the Brucks-based Allgather implementation.

* integer - default is 2, represents the radix value for generic transport Brucks-based Allgather.

Used in functions: MPI_Allgather

**MPIR_CVAR_ALLGATHER_COMPOSITION** - COLLECTIVE

Selects the CH4 allgather composition in `MPID_Allgather`. When set to automatic selection, the CH4 collective-selection table is consulted and may fall back to the MPIR allgather implementation if no selection is found. Forced CH4 compositions apply only when their communicator and message-size constraints are met; the multi-leader composition requires an intracommunicator whose ranks are node-canonical and whose per-rank message data fits within `MPIR_CVAR_ALLGATHER_SHM_PER_RANK`. If a forced composition cannot be applied, the fallback path uses the MPIR allgather implementation for intercommunicators and the network-module-only CH4 composition for intracommunicators. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets whether CH4 allgather uses automatic collective selection, multi-leader inter-node plus intra-node composition, or network-module-only composition.

* 0 - default, use CH4 collective-selection table lookup and MPIR fallback when no selection is found.
* 1 - use the multi-leader inter-node plus intra-node intracommunicator composition when the communicator and message-size constraints are met.
* 2 - use the network-module-only intracommunicator composition when the communicator constraints are met.

Used in functions: MPID_Allgather

**MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Allgather` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Allgather.

* true - default, allows the device to override MPIR-level `MPI_Allgather` collective algorithms.
* false - disables the device override for `MPI_Allgather`.

Used in functions: MPI_Allgather

**MPIR_CVAR_ALLGATHER_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Allgather_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Allgather_init.

* true - default, allows the device to override MPIR-level `MPI_Allgather_init` collective algorithms.
* false - disables the device override for `MPI_Allgather_init`.

Used in functions: MPI_Allgather_init

**MPIR_CVAR_ALLGATHER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Allgather algorithm. It is used for `MPI_Allgather` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* local_gather_remote_bcast - Forces the local-gather-remote-bcast algorithm.
* nb - Forces the nonblocking algorithm.

Used in functions: MPI_Allgather

**MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Allgather algorithm. Automatic selection may be overridden by `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; selected recursive-exchange and radix-k Brucks paths use the Allgather k-value CVARs, and recursive-exchange paths may also use the single-phase receive CVAR. When active, this CVAR sets which Allgather implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden by a collective selection tuning JSON file.
* brucks - forces the Brucks algorithm.
* k_brucks - forces the radix-k Brucks algorithm.
* nb - forces the nonblocking algorithm.
* recursive_doubling - forces the recursive-doubling algorithm.
* ring - forces the ring algorithm.
* recexch_doubling - forces the recursive-exchange distance-doubling algorithm.
* recexch_halving - forces the recursive-exchange distance-halving algorithm.
* circ_graph - forces the queued circulant graph algorithm.

Used in functions: MPI_Allgather

**MPIR_CVAR_ALLGATHER_IPC_READ_MSG_SIZE_THRESHOLD** - COLLECTIVE

Sets the message-size threshold, in bytes, for using POSIX GPU IPC read allgather. It is used only when the POSIX GPU IPC read allgather path is selected and GPU shared-memory support is enabled. The path also requires matching send and receive datatypes and counts, a contiguous datatype, and GPU IPC-capable send buffers from all ranks; otherwise it falls back to the MPIR Allgather implementation. When active, this CVAR sets the minimum allgather payload size that can use GPU IPC read instead of fallback allgather.

* integer - default is 256, represents the allgather GPU IPC read message-size threshold in bytes.

Used in functions: MPIDI_POSIX_mpi_allgather_gpu_ipc_read

**MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE** - COLLECTIVE

Sets the Allgather and Allgatherv long-message threshold, in bytes, used by sched-auto intra-communicator algorithm selection. When the total received data is below this threshold and the communicator size is a power of two, recursive doubling is selected. Otherwise, selection falls through to the `MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE` threshold, where shorter messages use Brucks and remaining messages use ring.

* integer - default is 524288, represents the Allgather and Allgatherv long-message threshold in bytes.

Used in functions: MPIR_Iallgather_intra_sched_auto, MPIR_Iallgatherv_intra_sched_auto

**MPIR_CVAR_ALLGATHER_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Selects the algorithm used for POSIX shared-memory intranode allgather. It is used in the POSIX allgather implementation and does not use automatic collective selection or depend on another CVAR in `src/mpid/ch4/shm/posix/posix_coll.h`. When active, this CVAR sets whether POSIX intranode allgather uses MPIR fallback or the POSIX GPU IPC read allgather path.

* mpir - default, use the MPIR allgather implementation.
* ipc_read - use the POSIX GPU IPC read allgather path.

Used in functions: MPIDI_POSIX_mpi_allgather

**MPIR_CVAR_ALLGATHER_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for recursive-exchange Allgather algorithms. It is used when a recursive-exchange Allgather path is selected, such as by forcing `MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM` to `recexch_doubling` or `recexch_halving`, or by internal collective selection. The selected recursive-exchange path may also use `MPIR_CVAR_ALLGATHER_RECEXCH_SINGLE_PHASE_RECV` to control receive-posting behavior.

* integer - default is 2, represents the k value used by recursive-exchange Allgather algorithms.

Used in functions: MPI_Allgather

**MPIR_CVAR_ALLGATHER_RECEXCH_SINGLE_PHASE_RECV** - COLLECTIVE

Controls receive posting for recursive-exchange Allgather algorithms. It is used when a recursive-exchange Allgather path is selected, such as by forcing `MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM` to `recexch_doubling` or `recexch_halving`, or by internal collective selection. When active, it sets whether receives are posted for one phase or two phases in recursive-exchange Allgather algorithms.

* false - default, posts receives for two phases.
* true - posts receives for one phase.

Used in functions: MPI_Allgather

**MPIR_CVAR_ALLGATHER_SHM_PER_RANK** - COLLECTIVE

Sets the per-rank shared-memory buffer size used by the CH4 allgather multi-leader composition. This CVAR is used when `MPIR_CVAR_ALLGATHER_COMPOSITION` forces the multi-leader composition or when automatic CH4 collective selection chooses that composition. The multi-leader composition also requires an intracommunicator whose ranks are node-canonical, and the per-rank allgather message data must fit within this limit. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll_impl.h` or `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets both the per-rank shared-memory allocation size for the node-local staging buffer and the per-rank message-size limit for using the CH4 allgather multi-leader composition.

* 4096 - default, use 4096 bytes of shared memory per rank.
* positive integer - use the specified number of bytes of shared memory per rank.

Used in functions: MPIDI_Allgather_allcomm_composition_json, MPID_Allgather, MPIDI_Allgather_intra_composition_alpha

**MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE** - COLLECTIVE

Sets the Allgather and Allgatherv short-message threshold, in bytes, used by sched-auto intra-communicator algorithm selection. It is used only after the recursive-doubling condition based on `MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE` and a power-of-two communicator size is not selected. When active, total received data below this threshold selects the Brucks algorithm; otherwise the ring algorithm is selected.

* integer - default is 81920, represents the Allgather and Allgatherv short-message threshold in bytes.

Used in functions: MPIR_Iallgather_intra_sched_auto, MPIR_Iallgatherv_intra_sched_auto

**MPIR_CVAR_ALLREDUCE_CACHE_PER_LEADER** - COLLECTIVE

Sets the cache-tile size used by the CH4 allreduce multi-leader delta composition during the local reduction among node leaders. In `src/mpid/ch4/src/ch4_coll_impl.h`, this CVAR is used when `MPIDI_Allreduce_intra_composition_delta` runs and intra-node leader communicators exist; each leader reduces its assigned portion of every allreduce shared-memory chunk in cache-sized tiles before the inter-node allreduce step. The tile calculation also depends on the datatype extent and the per-leader element count for the current shared-memory chunk, which is determined from the number of leaders and `MPIR_CVAR_ALLREDUCE_SHM_PER_LEADER`. The same composition also uses `MPIR_CVAR_ALLREDUCE_LOCAL_COPY_OFFSETS` later for the final local copy pattern. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll_impl.h`. When active, this CVAR sets how much data each local-reduction tile should cover to reduce cache misses in the multi-leader allreduce path.

* 512 - default, use 512 bytes per local-reduction cache tile.
* positive integer - use the specified number of bytes per local-reduction cache tile.

Used in functions: MPIDI_Allreduce_intra_composition_delta

**MPIR_CVAR_ALLREDUCE_CCL** - COLLECTIVE

Selects the collective communication library backend used by the CCL allreduce wrapper. It is used only when the CCL allreduce algorithm is selected, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `ccl` or by collective tuning selection. When the selected backend is unavailable or cannot handle the reduction arguments, the wrapper falls back to the internally selected Allreduce algorithm.

* auto - default, selects an available CCL backend for the CCL allreduce wrapper.
* nccl - selects NCCL for the CCL allreduce wrapper.
* rccl - selects RCCL for the CCL allreduce wrapper.

Used in functions: get_ccl_from_string, MPIR_Allreduce_intra_ccl

**MPIR_CVAR_ALLREDUCE_COMPOSITION** - COLLECTIVE

Selects the CH4 allreduce composition in `MPID_Allreduce`. When set to automatic selection, the CH4 collective-selection table is consulted and may fall back to the MPIR allreduce implementation if no selection is found. Forced CH4 compositions apply only when their communicator and operation constraints are met; some paths require an intracommunicator, a parent communicator, an all-local intranode communicator, a node-balanced communicator, or a commutative operation. The multi-leader composition uses `MPIR_CVAR_NUM_MULTI_LEADS` to choose the number of leaders, rounded so it divides the node-local communicator size. If a forced composition cannot be applied, the fallback path uses the MPIR allreduce implementation for intercommunicators and the network-module-only CH4 composition for intracommunicators. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets whether CH4 allreduce uses automatic collective selection, combined network-module and shared-memory composition, network-module-only composition, shared-memory-only composition, or multi-leader inter-node plus intra-node composition.

* 0 - default, use CH4 collective-selection table lookup and MPIR fallback when no selection is found.
* 1 - use the combined network-module and shared-memory intracommunicator composition with reduce followed by broadcast when the communicator and operation constraints are met.
* 2 - use the network-module-only intracommunicator composition when the communicator constraints are met.
* 3 - use the shared-memory-only intracommunicator composition when all ranks are in the node-local communicator.
* 4 - use the multi-leader inter-node plus intra-node intracommunicator composition when the communicator, message-size, and operation constraints are met.

Used in functions: MPID_Allreduce

**MPIR_CVAR_ALLREDUCE_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Allreduce` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Allreduce.

* true - default, allows the device to override MPIR-level `MPI_Allreduce` collective algorithms.
* false - disables the device override for `MPI_Allreduce`.

Used in functions: MPI_Allreduce

**MPIR_CVAR_ALLREDUCE_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Allreduce_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Allreduce_init.

* true - default, allows the device to override MPIR-level `MPI_Allreduce_init` collective algorithms.
* false - disables the device override for `MPI_Allreduce_init`.

Used in functions: MPI_Allreduce_init

**MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Allreduce algorithm. It is used for `MPI_Allreduce` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* reduce_exchange_bcast - Forces the reduce-exchange-bcast algorithm.

Used in functions: MPI_Allreduce

**MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Allreduce algorithm. Automatic selection may be overridden by `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; selected implementations may also use the Allreduce recursive-multiplying, tree, recursive-exchange, and CCL CVARs. When active, this CVAR sets which Allreduce implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* smp - Forces the SMP algorithm.
* recursive_doubling - Forces the recursive-doubling algorithm.
* recursive_multiplying - Forces the recursive-multiplying algorithm.
* reduce_scatter_allgather - Forces the reduce-scatter-allgather algorithm.
* tree - Forces the pipelined tree algorithm.
* recexch - Forces the generic transport recursive-exchange algorithm.
* ring - Forces the ring algorithm.
* k_reduce_scatter_allgather - Forces the k reduce-scatter-allgather algorithm.
* ccl - Forces the CCL algorithm.

Used in functions: MPI_Allreduce

**MPIR_CVAR_ALLREDUCE_LOCAL_COPY_OFFSETS** - COLLECTIVE

Sets the number of offsets used by the CH4 allreduce multi-leader delta composition when copying reduced data from the shared-memory buffer to the receive buffer. In `src/mpid/ch4/src/ch4_coll_impl.h`, this CVAR is used when `MPIDI_Allreduce_intra_composition_delta` runs, after the local reduction, inter-node allreduce, and node-local synchronization steps complete for each shared-memory chunk. The copy pattern depends on the current chunk count, datatype extent, and the rank in the node-local communicator; if the current chunk cannot be evenly split by the configured offset count, the function uses a single offset for the local copy. The same composition also uses the number of leaders passed into the function, `MPIR_CVAR_ALLREDUCE_SHM_PER_LEADER` for shared-memory chunking, and `MPIR_CVAR_ALLREDUCE_CACHE_PER_LEADER` for cache-sized local reductions. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll_impl.h`. When active, this CVAR sets how many offset segments the final local copy should use in the multi-leader allreduce path.

* 2 - default, split the final local copy into two offset segments when the current chunk can be evenly divided that way.
* 1 - use a single contiguous final local copy segment.
* positive integer greater than 1 - split the final local copy into the specified number of offset segments when the current chunk can be evenly divided that way; otherwise use a single offset segment.

Used in functions: MPIDI_Allreduce_intra_composition_delta

**MPIR_CVAR_ALLREDUCE_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Selects the algorithm used for POSIX shared-memory intranode allreduce. It is used in the POSIX allreduce implementation; automatic selection consults the POSIX collective-selection table and can fall back to the MPIR allreduce implementation when no POSIX selection is found. The shared-memory release-gather path is used only when MPICH is not threaded and the reduction operation is commutative. When active, this CVAR sets whether POSIX intranode allreduce uses MPIR fallback, shared-memory release-gather allreduce, or internal POSIX collective selection.

* auto - default, use internal POSIX collective selection.
* mpir - use the MPIR allreduce implementation.
* release_gather - use the POSIX shared-memory release-gather allreduce path when threading and operation-commutativity conditions allow it.

Used in functions: MPIDI_POSIX_mpi_allreduce

**MPIR_CVAR_ALLREDUCE_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for the generic transport recursive-exchange Allreduce algorithm. It is used when intra-communicator Allreduce selects the recursive-exchange algorithm, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `recexch` or by internal collective selection. The recursive-exchange path may also use `MPIR_CVAR_ALLREDUCE_RECEXCH_SINGLE_PHASE_RECV`; when collective tuning selection provides a recursive-exchange Allreduce container, that container's k value may be used instead of this CVAR.

* integer - default is 2, represents the k value used by recursive-exchange Allreduce.

Used in functions: MPI_Allreduce

**MPIR_CVAR_ALLREDUCE_RECEXCH_SINGLE_PHASE_RECV** - COLLECTIVE

Controls receive posting for recursive-exchange Allreduce algorithms. It is used when intra-communicator Allreduce selects the recursive-exchange algorithm, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `recexch` or by internal collective selection. The recursive-exchange path also uses `MPIR_CVAR_ALLREDUCE_RECEXCH_KVAL`; when active, this CVAR sets whether receives are posted for one phase or two phases in recursive-exchange Allreduce algorithms.

* false - default, posts receives for two phases.
* true - posts receives for one phase.

Used in functions: MPI_Allreduce

**MPIR_CVAR_ALLREDUCE_RECURSIVE_MULTIPLYING_KVAL** - COLLECTIVE

Sets the radix value for the generic recursive-multiplying Allreduce algorithm. It is used when intra-communicator Allreduce selects the recursive-multiplying algorithm, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `recursive_multiplying` or by internal collective selection. When active, it controls the radix used by the recursive-multiplying Allreduce implementation.

* integer - default is 2, represents the radix value for generic recursive-multiplying Allreduce.

Used in functions: MPI_Allreduce

**MPIR_CVAR_ALLREDUCE_SHM_PER_LEADER** - COLLECTIVE

Sets the per-leader shared-memory buffer size used by the CH4 allreduce multi-leader delta composition. In `src/mpid/ch4/src/ch4_coll_impl.h`, this CVAR is used when `MPIDI_Allreduce_intra_composition_delta` runs; that composition creates multi-leader subcommunicators as needed, allocates one shared-memory buffer per node leader, and chunks messages that exceed the per-leader buffer. The composition also uses the number of leaders passed into the function, `MPIR_CVAR_ALLREDUCE_CACHE_PER_LEADER` for cache-sized local reductions, and `MPIR_CVAR_ALLREDUCE_LOCAL_COPY_OFFSETS` for the final local copy pattern. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll_impl.h`. When active, this CVAR sets the per-node-leader shared-memory allocation size used for staging reduced allreduce data in the multi-leader delta composition.

* -1 - default, choose the per-leader shared-memory size from the packed message size of the first call that allocates the communicator's allreduce shared-memory buffer, capped at 4 MB.
* 1-4194304 - use the specified number of bytes of shared memory per node leader.
* greater than 4194304 - use the maximum per-leader shared-memory size of 4 MB.

Used in functions: MPIDI_Allreduce_intra_composition_delta

**MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE** - COLLECTIVE

Sets the allreduce short-message threshold, in bytes, used by sched-auto intra-communicator Allreduce and by group Allreduce algorithm selection. When active, messages at or below this threshold use recursive doubling. Messages above this threshold use reduce-scatter followed by allgather only when the operation is built-in and the count is at least the nearest lower power of two of the participating communicator or group size; otherwise recursive doubling is still used. In sched-auto intra-communicator Allreduce, this CVAR is bypassed when the parent-communicator SMP path is selected for a commutative operation.

* integer - default is 2048, represents the allreduce short-message threshold in bytes.

Used in functions: MPII_Allreduce_group_intra, MPIR_Iallreduce_intra_sched_auto

**MPIR_CVAR_ALLREDUCE_TOPO_DIFF_GROUPS** - COLLECTIVE

Sets the latency cost used for communication between different topology groups when constructing topology-wave Allreduce trees. It is used only by the intra-communicator tree Allreduce implementation when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_ALLREDUCE_TREE_TYPE` together with the tree Allreduce algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_ALLREDUCE_TOPO_REORDER_ENABLE`, `MPIR_CVAR_ALLREDUCE_TOPO_OVERHEAD`, and the Allreduce topology latency CVARs for different switches and same switches. When collective tuning selection provides an intra-tree Allreduce container, that container's different-groups topology latency value overrides this CVAR for the tree construction.

* integer - default is 2800, represents the topology-wave latency cost between different groups.

Used in functions: MPIR_Allreduce_intra_tree

**MPIR_CVAR_ALLREDUCE_TOPO_DIFF_SWITCHES** - COLLECTIVE

Sets the latency cost used for communication between different switches in the same topology group when constructing topology-wave Allreduce trees. It is used only by the intra-communicator tree Allreduce implementation when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_ALLREDUCE_TREE_TYPE` together with the tree Allreduce algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_ALLREDUCE_TOPO_REORDER_ENABLE`, `MPIR_CVAR_ALLREDUCE_TOPO_OVERHEAD`, and the Allreduce topology latency CVARs for different groups and same switches. When collective tuning selection provides an intra-tree Allreduce container, that container's different-switches topology latency value overrides this CVAR for the tree construction.

* integer - default is 1900, represents the topology-wave latency cost between different switches in the same group.

Used in functions: MPIR_Allreduce_intra_tree

**MPIR_CVAR_ALLREDUCE_TOPO_OVERHEAD** - COLLECTIVE

Sets the fixed overhead cost used when constructing topology-wave Allreduce trees. It is used only by the intra-communicator tree Allreduce implementation when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_ALLREDUCE_TREE_TYPE` together with the tree Allreduce algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_ALLREDUCE_TOPO_REORDER_ENABLE` and the Allreduce topology latency CVARs for different groups, different switches, and same switches. When collective tuning selection provides an intra-tree Allreduce container, that container's topology overhead value overrides this CVAR for the tree construction.

* integer - default is 200, represents the topology-wave overhead cost.

Used in functions: MPIR_Allreduce_intra_tree

**MPIR_CVAR_ALLREDUCE_TOPO_REORDER_ENABLE** - COLLECTIVE

Enables reordering leaders based on the number of ranks in each group when creating topology-aware Allreduce trees. It is used only by the intra-communicator tree Allreduce implementation when `MPIR_CVAR_ALLREDUCE_TREE_TYPE` selects a topology-aware, topology-aware-k, or topology-wave tree. For topology-wave trees, the tree construction also uses the Allreduce topology cost CVARs for overhead and inter-group, inter-switch, and same-switch latencies.

* false - Disables leader reordering for topology-aware Allreduce tree construction.
* true - default, enables leader reordering for topology-aware Allreduce tree construction.

Used in functions: MPIR_Allreduce_intra_tree

**MPIR_CVAR_ALLREDUCE_TOPO_SAME_SWITCHES** - COLLECTIVE

Sets the latency cost used for communication within the same switch when constructing topology-wave Allreduce trees. It is used only by the intra-communicator tree Allreduce implementation when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_ALLREDUCE_TREE_TYPE` together with the tree Allreduce algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_ALLREDUCE_TOPO_REORDER_ENABLE`, `MPIR_CVAR_ALLREDUCE_TOPO_OVERHEAD`, and the Allreduce topology latency CVARs for different groups and different switches. When collective tuning selection provides an intra-tree Allreduce container, that container's same-switches topology latency value overrides this CVAR for the tree construction.

* integer - default is 1600, represents the topology-wave latency cost within the same switch.

Used in functions: MPIR_Allreduce_intra_tree

**MPIR_CVAR_ALLREDUCE_TREE_BUFFER_PER_CHILD** - COLLECTIVE

Sets whether tree-based Allreduce allocates a dedicated receive buffer for each child. It is used when the tree Allreduce algorithm is selected, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `tree` or by internal collective selection, with k-ary or knomial tree algorithms. The tree path also relies on the selected Allreduce tree type, tree k value, and tree pipeline chunk size CVARs. When active, this CVAR controls whether child receives can be preposted at the cost of additional memory, or whether one shared child receive buffer serializes receives.

* false - default, uses one receive buffer for all children, so receives from children are serialized.
* true - allocates a dedicated receive buffer for each child, enabling receives to be preposted at the cost of additional memory.

Used in functions: MPIR_Allreduce_intra_tree

**MPIR_CVAR_ALLREDUCE_TREE_KVAL** - COLLECTIVE

Sets the branching factor for k-ary or knomial Allreduce trees. It is initialized during collective setup and used when the tree Allreduce algorithm is selected, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `tree` or by internal collective selection. The tree path also relies on the selected Allreduce tree type, tree pipeline chunk size, and tree buffer-per-child CVARs; topology-aware tree choices may also use the Allreduce topology CVARs.

* integer - default is 2, represents the branching factor for k-ary or knomial Allreduce trees.

Used in functions: MPII_Coll_init

**MPIR_CVAR_ALLREDUCE_TREE_PIPELINE_CHUNK_SIZE** - COLLECTIVE

Sets the maximum chunk size, in bytes, for pipelining in tree-based Allreduce. It is used when the tree Allreduce algorithm is selected, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `tree` or by internal collective selection. The tree path also relies on the selected Allreduce tree type, tree k value, and tree buffer-per-child CVARs; topology-aware tree choices may also use the Allreduce topology CVARs.

* 0 - default, disables pipelining.
* positive integer - represents the maximum pipeline chunk size in bytes.

Used in functions: MPIR_Allreduce_intra_tree

**MPIR_CVAR_ALLREDUCE_TREE_TYPE** - COLLECTIVE

Sets the tree type for tree-based Allreduce. It is initialized during collective setup and used when the tree Allreduce algorithm is selected, such as by forcing `MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM` to `tree` or by internal collective selection. The tree path also relies on the Allreduce tree k value, tree pipeline chunk size, and tree buffer-per-child CVARs; topology-aware tree choices also rely on the Allreduce topology CVARs for leader reordering and topology cost settings.

* kary - represents a k-ary tree.
* knomial_1 - default, represents a knomial_1 tree and supports both commutative and non-commutative reduce operations.
* knomial_2 - represents a knomial_2 tree.
* topology_aware - represents a topology-aware tree.
* topology_aware_k - represents a topology-aware tree with branching factor k.
* topology_wave - represents a topology-wave tree.

Used in functions: MPII_Coll_init

**MPIR_CVAR_ALLTOALLV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Alltoallv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Alltoallv.

* true - default, allows the device to override MPIR-level `MPI_Alltoallv` collective algorithms.
* false - disables the device override for `MPI_Alltoallv`.

Used in functions: MPI_Alltoallv

**MPIR_CVAR_ALLTOALLV_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Alltoallv_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Alltoallv_init.

* true - default, allows the device to override MPIR-level `MPI_Alltoallv_init` collective algorithms.
* false - disables the device override for `MPI_Alltoallv_init`.

Used in functions: MPI_Alltoallv_init

**MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Alltoallv algorithm. It is used for `MPI_Alltoallv` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which inter-communicator Alltoallv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* pairwise_exchange - Forces the pairwise-exchange algorithm.
* nb - Forces the nonblocking algorithm.

Used in functions: MPI_Alltoallv

**MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Alltoallv algorithm. It is used for `MPI_Alltoallv` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; selected scattered and pairwise sendrecv-replace paths may also use Alltoall-family CVARs that control throttling or pair ordering. When active, this CVAR sets which Alltoallv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden by a collective selection tuning JSON file.
* nb - Forces the nonblocking algorithm.
* pairwise_sendrecv_replace - Forces the pairwise sendrecv-replace algorithm.
* scattered - Forces the scattered algorithm.

Used in functions: MPI_Alltoallv

**MPIR_CVAR_ALLTOALLV_PAIRWISE_NEW** - COLLECTIVE

Selects the bit-wise pair ordering for the in-place pairwise sendrecv-replace Alltoall-family implementation. It is used only when that shared pairwise sendrecv-replace implementation is selected for intra-communicator `MPI_IN_PLACE` Alltoall, Alltoallv, or Alltoallw. When enabled, all ranks exchange with peers in rank-XOR mask order; when disabled, the implementation uses the existing intranode-aware ordering when communicator intranode information is available, otherwise rank order.

* false - default, uses the existing intranode-aware or rank-order pairwise sendrecv-replace ordering.
* true - enables bit-wise pair ordering for pairwise sendrecv-replace exchanges.

Alternate names: MPIR_CVAR_ALLTOALL_PAIRWISE_NEW, MPIR_CVAR_ALLTOALLW_PAIRWISE_NEW

Used in functions: MPIR_Alltoall_intra_pairwise_sendrecv_replace, MPIR_Alltoallv_intra_pairwise_sendrecv_replace, MPIR_Alltoallw_intra_pairwise_sendrecv_replace

**MPIR_CVAR_ALLTOALLW_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Alltoallw` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Alltoallw.

* true - default, allows the device to override MPIR-level `MPI_Alltoallw` collective algorithms.
* false - disables the device override for `MPI_Alltoallw`.

Used in functions: MPI_Alltoallw

**MPIR_CVAR_ALLTOALLW_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Alltoallw_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Alltoallw_init.

* true - default, allows the device to override MPIR-level `MPI_Alltoallw_init` collective algorithms.
* false - disables the device override for `MPI_Alltoallw_init`.

Used in functions: MPI_Alltoallw_init

**MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Alltoallw algorithm. It is used for `MPI_Alltoallw` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which inter-communicator Alltoallw implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* pairwise_exchange - Forces the pairwise-exchange algorithm.

Used in functions: MPI_Alltoallw

**MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Alltoallw algorithm. It is used for `MPI_Alltoallw` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; selected scattered and pairwise sendrecv-replace paths may also use Alltoall-family CVARs that control throttling or pair ordering. When active, this CVAR sets which Alltoallw implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* pairwise_sendrecv_replace - Forces the pairwise sendrecv-replace algorithm.
* scattered - Forces the scattered algorithm.

Used in functions: MPI_Alltoallw

**MPIR_CVAR_ALLTOALL_BRUCKS_KVAL** - COLLECTIVE

Sets the radix for the generic transport Brucks-based intra-communicator Alltoall algorithm. It is used only when the radix-k Brucks Alltoall path is selected, such as by forcing `MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM` to `k_brucks` or by internal collective/tuning selection. When active, it controls the base used to split Alltoall data movement into phases and determines how many nonzero digit exchanges are attempted in each phase.

* integer 2 or greater - default is 2, represents the radix used by the k-Brucks Alltoall algorithm.

Used in functions: MPIR_Alltoall_intra_k_brucks

**MPIR_CVAR_ALLTOALL_COMPOSITION** - COLLECTIVE

Selects the CH4 alltoall composition in `MPID_Alltoall`. When set to automatic selection, the CH4 collective-selection table is consulted and may fall back to the MPIR alltoall implementation if no selection is found. Forced CH4 compositions apply only when their communicator and message-size constraints are met; the multi-leader composition requires an intracommunicator whose ranks are node-canonical and whose per-rank message data fits within `MPIR_CVAR_ALLTOALL_SHM_PER_RANK`. If a forced composition cannot be applied, the fallback path uses the MPIR alltoall implementation for intercommunicators and the network-module-only CH4 composition for intracommunicators. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets whether CH4 alltoall uses automatic collective selection, multi-leader inter-node plus intra-node composition, or network-module-only composition.

* 0 - default, use CH4 collective-selection table lookup and MPIR fallback when no selection is found.
* 1 - use the multi-leader inter-node plus intra-node intracommunicator composition when the communicator and message-size constraints are met.
* 2 - use the network-module-only intracommunicator composition when the communicator constraints are met.

Used in functions: MPID_Alltoall

**MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Alltoall` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Alltoall.

* true - default, allows the device to override MPIR-level `MPI_Alltoall` collective algorithms.
* false - disables the device override for `MPI_Alltoall`.

Used in functions: MPI_Alltoall

**MPIR_CVAR_ALLTOALL_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Alltoall_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Alltoall_init.

* true - default, allows the device to override MPIR-level `MPI_Alltoall_init` collective algorithms.
* false - disables the device override for `MPI_Alltoall_init`.

Used in functions: MPI_Alltoall_init

**MPIR_CVAR_ALLTOALL_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Alltoall algorithm. It is used for `MPI_Alltoall` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* pairwise_exchange - Forces the pairwise-exchange algorithm.

Used in functions: MPI_Alltoall

**MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Alltoall algorithm. It is used for `MPI_Alltoall` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; the selected implementation may also use the Alltoall short-message and medium-message thresholds, the Alltoall throttle value, or the Alltoall Brucks radix CVAR.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* brucks - Forces the Brucks algorithm.
* k_brucks - Forces the radix-k Brucks algorithm.
* nb - Forces the nonblocking algorithm.
* pairwise - Forces the pairwise algorithm.
* pairwise_sendrecv_replace - Forces the pairwise sendrecv-replace algorithm.
* scattered - Forces the scattered algorithm.

Used in functions: MPI_Alltoall

**MPIR_CVAR_ALLTOALL_IPC_READ_MSG_SIZE_THRESHOLD** - COLLECTIVE

Sets the message-size threshold, in bytes, for using POSIX GPU IPC read alltoall. It is used only when the POSIX GPU IPC read alltoall path is selected and GPU shared-memory support is enabled. The path also requires matching send and receive datatypes and counts, a contiguous datatype, and GPU IPC-capable send buffers from all ranks; otherwise it falls back to the MPIR Alltoall implementation. When active, this CVAR sets the minimum alltoall payload size that can use GPU IPC read instead of fallback alltoall.

* integer - default is 256, represents the alltoall GPU IPC read message-size threshold in bytes.

Used in functions: MPIDI_POSIX_mpi_alltoall_gpu_ipc_read

**MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE** - COLLECTIVE

Sets the per-destination medium-message threshold, in bytes, used by sched-auto intra-communicator Ialltoall algorithm selection. It is used only for non-in-place operations after the short-message path controlled by `MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE` and communicator size has not been selected. When active, messages at or below this threshold select the permuted-sendrecv schedule; larger messages select the pairwise schedule.

* integer - default is 32768, represents the per-destination medium-message threshold in bytes.

Used in functions: MPIR_Ialltoall_intra_sched_auto

**MPIR_CVAR_ALLTOALL_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Selects the algorithm used for POSIX shared-memory intranode alltoall. It is used in the POSIX alltoall implementation and does not use automatic collective selection or depend on another CVAR in `src/mpid/ch4/shm/posix/posix_coll.h`. When active, this CVAR sets whether POSIX intranode alltoall uses MPIR fallback or the POSIX GPU IPC read alltoall path.

* mpir - default, use the MPIR alltoall implementation.
* ipc_read - use the POSIX GPU IPC read alltoall path.

Used in functions: MPIDI_POSIX_mpi_alltoall

**MPIR_CVAR_ALLTOALL_SHM_PER_RANK** - COLLECTIVE

Sets the per-rank shared-memory buffer size used by the CH4 alltoall multi-leader composition. This CVAR is used when `MPIR_CVAR_ALLTOALL_COMPOSITION` forces the multi-leader composition or when automatic CH4 collective selection chooses that composition. The multi-leader composition also requires an intracommunicator whose ranks are node-canonical, and the per-rank alltoall message data must fit within this limit. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll_impl.h` or `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets both the per-rank shared-memory allocation size for the node-local staging buffer and the per-rank message-size limit for using the CH4 alltoall multi-leader composition.

* 4096 - default, use 4096 bytes of shared memory per rank.
* positive integer - use the specified number of bytes of shared memory per rank.

Used in functions: MPIDI_Alltoall_allcomm_composition_json, MPID_Alltoall, MPIDI_Alltoall_intra_composition_alpha

**MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE** - COLLECTIVE

Sets the per-destination short-message threshold, in bytes, used by sched-auto intra-communicator Ialltoall algorithm selection. It is used only for non-in-place operations when the communicator has at least 8 ranks; messages at or below this threshold select the Brucks algorithm, otherwise selection falls through to `MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE` and then to the large-message path. When active, it controls the cutoff for using the short-message Alltoall schedule.

* integer - default is 256, represents the per-destination short-message threshold in bytes.

Used in functions: MPIR_Ialltoall_intra_sched_auto

**MPIR_CVAR_ALLTOALL_THROTTLE** - COLLECTIVE

Sets the maximum number of source and destination ranks handled in one communication block by selected intra-communicator Alltoall-family scattered, blocked, and permuted-sendrecv algorithms. It is used only when one of those implementations is selected, such as by forcing the corresponding Alltoall-family algorithm CVAR or by internal collective/tuning selection. When active, the implementation posts or schedules receives and sends for one block of ranks, waits for that block to complete, and then continues with the next block.

* 0 - Uses the communicator size as the block size, causing all receives and sends to be posted or scheduled in one block.
* positive integer - default is 32, represents the maximum number of source and destination ranks handled in one communication block.

Used in functions: MPIR_Alltoall_intra_scattered, MPIR_Alltoallv_intra_scattered, MPIR_Alltoallw_intra_scattered, MPIR_Ialltoall_intra_sched_permuted_sendrecv, MPIR_Ialltoallv_intra_sched_blocked, MPIR_Ialltoallw_intra_sched_blocked

**MPIR_CVAR_ASYNC_PROGRESS** - THREADS

Controls whether MPICH requests and starts asynchronous progress support. It is used during local process attribute initialization to request `MPI_THREAD_MULTIPLE`, and during asynchronous initialization after MPI world initialization to start the device asynchronous progress thread only when the provided thread level is `MPI_THREAD_MULTIPLE`; otherwise MPICH prints a warning. The asynchronous initialization path is compiled only for threaded builds with `MPI_THREAD_MULTIPLE` support. When active, this CVAR enables asynchronous progress for point-to-point, collective, one-sided, and I/O operations and sets the requested thread-safety level to `MPI_THREAD_MULTIPLE`.

* false - default, do not request `MPI_THREAD_MULTIPLE` for asynchronous progress and do not start the device asynchronous progress thread.
* true - request `MPI_THREAD_MULTIPLE` and start the device asynchronous progress thread when MPI world initialization has completed and `MPI_THREAD_MULTIPLE` is provided.

Used in functions: MPII_init_local_proc_attrs, MPII_init_async

**MPIR_CVAR_BARRIER_COMPOSITION** - COLLECTIVE

Selects the CH4 barrier composition in `MPID_Barrier`. When set to automatic selection, the CH4 collective-selection table is consulted and may fall back to the MPIR barrier implementation if no selection is found. Forced CH4 compositions apply only to intracommunicators, with the inter-node plus intra-node composition also requiring a parent communicator; if a forced composition cannot be applied, the fallback path uses the MPIR barrier implementation for intercommunicators and the network-module-only CH4 composition for intracommunicators. No alternate CVAR name or dependency on another CVAR is visible in `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets whether CH4 barrier uses automatic collective selection, a combined network-module and shared-memory composition, or a network-module-only composition.

* 0 - default, use CH4 collective-selection table lookup and MPIR fallback when no selection is found.
* 1 - use the combined network-module and shared-memory intracommunicator composition when the communicator constraints are met.
* 2 - use the network-module-only intracommunicator composition when the communicator constraints are met.

Used in functions: MPID_Barrier

**MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Barrier` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Barrier.

* true - default, allows the device to override MPIR-level `MPI_Barrier` collective algorithms.
* false - disables the device override for `MPI_Barrier`.

Used in functions: MPI_Barrier

**MPIR_CVAR_BARRIER_DISSEM_KVAL** - COLLECTIVE

Sets the k value for the high-radix dissemination intra-communicator Barrier algorithm. It is used only when that Barrier implementation is selected, such as by forcing `MPIR_CVAR_BARRIER_INTRA_ALGORITHM` to `k_dissemination` or by internal collective/tuning selection.

* integer - default is 2, represents the k value used by the high-radix dissemination Barrier algorithm.

Used in functions: MPI_Barrier

**MPIR_CVAR_BARRIER_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Barrier_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Barrier_init.

* true - default, allows the device to override MPIR-level `MPI_Barrier_init` collective algorithms.
* false - disables the device override for `MPI_Barrier_init`.

Used in functions: MPI_Barrier_init

**MPIR_CVAR_BARRIER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Barrier algorithm. It is used for `MPI_Barrier` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* bcast - Forces the Bcast Barrier algorithm.
* nb - Forces the nonblocking Barrier algorithm.

Used in functions: MPI_Barrier

**MPIR_CVAR_BARRIER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Barrier algorithm. It is used for `MPI_Barrier` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. The selected barrier implementation may also use `MPIR_CVAR_BARRIER_DISSEM_KVAL`, `MPIR_CVAR_BARRIER_RECEXCH_KVAL`, or `MPIR_CVAR_BARRIER_RECEXCH_SINGLE_PHASE_RECV` for radix and receive-posting behavior.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking Barrier algorithm.
* smp - Forces the SMP Barrier algorithm.
* k_dissemination - Forces the high-radix dissemination Barrier algorithm.
* recexch - Forces the recursive-exchange Barrier algorithm.

Used in functions: MPI_Barrier

**MPIR_CVAR_BARRIER_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Selects the algorithm used for POSIX shared-memory intranode barrier. It is used in the POSIX barrier implementation; automatic selection consults the POSIX collective-selection table and can fall back to the MPIR barrier implementation when no POSIX selection is found. The shared-memory release-gather path is used only when MPICH is not threaded. When active, this CVAR sets whether POSIX intranode barrier uses MPIR fallback, shared-memory release-gather barrier, or internal POSIX collective selection.

* auto - default, use internal POSIX collective selection.
* mpir - use the MPIR barrier implementation.
* release_gather - use the POSIX shared-memory release-gather barrier path when threading conditions allow it.

Alternate name: MPIR_CVAR_GROUP_COLL_ALGO

Used in functions: MPIDI_POSIX_mpi_barrier

**MPIR_CVAR_BARRIER_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for the recursive-exchange intra-communicator Barrier algorithm. It is used only when that Barrier implementation is selected, such as by forcing `MPIR_CVAR_BARRIER_INTRA_ALGORITHM` to `recexch` or by internal collective/tuning selection. The selected recursive-exchange Barrier path may also use `MPIR_CVAR_BARRIER_RECEXCH_SINGLE_PHASE_RECV` to control receive-posting behavior.

* integer - default is 2, represents the k value used by the recursive-exchange Barrier algorithm.

Used in functions: MPI_Barrier

**MPIR_CVAR_BARRIER_RECEXCH_SINGLE_PHASE_RECV** - COLLECTIVE

Controls receive posting for recursive-exchange Barrier algorithms. It is used only when a recursive-exchange Barrier implementation is selected, such as by forcing `MPIR_CVAR_BARRIER_INTRA_ALGORITHM` to `recexch` or by internal collective/tuning selection. When active, it sets whether recursive-exchange Barrier receives are posted for one phase or for two phases.

* false - default, posts receives for two phases.
* true - posts receives for one phase.

Used in functions: MPI_Barrier

**MPIR_CVAR_BCAST** - COLLECTIVE

In `src/mpid/ch4/shm/posix/release_gather/release_gather.c`, `MPIR_CVAR_BCAST` is not a standalone CVAR or an alternate name for another CVAR. It appears only in the CVAR block as part of the shorthand `MPIR_CVAR_BCAST{REDUCE}_INTRANODE_TREE_KVAL` and `MPIR_CVAR_BCAST{REDUCE}_INTRANODE_TREE_TYPE`, referring to the broadcast side of the POSIX shared-memory release-gather intranode tree CVARs. The broadcast tree settings are captured when release-gather communicator state is initialized, and topology-aware tree creation is attempted only when topology-aware intranode trees are enabled, user-provided process binding is present, and hardware topology support is initialized; otherwise a non-topology-aware broadcast tree is created. When active through the actual broadcast intranode tree CVARs, this shorthand represents the broadcast tree radix and tree type used by POSIX release-gather broadcast collectives.

* no standalone values - `MPIR_CVAR_BCAST` does not define valid values by itself; values are defined by the actual broadcast intranode tree CVARs it refers to.

Used in functions: none as a standalone CVAR in `src/mpid/ch4/shm/posix/release_gather/release_gather.c`

**MPIR_CVAR_BCAST_COMPOSITION** - COLLECTIVE

Selects the CH4 broadcast composition in `MPID_Bcast`. When set to automatic selection, the CH4 collective-selection table is consulted and may fall back to the MPIR broadcast implementation if no selection is found. If `MPIR_CVAR_COLL_HYBRID_MEMORY` is disabled, automatic selection first checks the broadcast buffer memory type and uses the GPU-specific collective-selection table for strict device buffers. Forced CH4 compositions apply only to intracommunicators, with the combined network-module and shared-memory compositions also requiring a parent communicator; if a forced composition cannot be applied, the fallback path uses the MPIR broadcast implementation for intercommunicators and the network-module-only CH4 composition for intracommunicators. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets whether CH4 broadcast uses automatic collective selection, a combined network-module and shared-memory composition, or a network-module-only composition.

* 0 - default, use CH4 collective-selection table lookup and MPIR fallback when no selection is found.
* 1 - use the combined network-module and shared-memory intracommunicator composition with explicit send-receive between rank 0 and root when the communicator constraints are met.
* 2 - use the combined network-module and shared-memory intracommunicator composition without explicit send-receive between rank 0 and root when the communicator constraints are met.
* 3 - use the network-module-only intracommunicator composition when the communicator constraints are met.
* 4 - use the combined network-module and shared-memory intracommunicator composition selected by the delta path when the communicator constraints are met.

Used in functions: MPID_Bcast

**MPIR_CVAR_BCAST_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Bcast` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Bcast.

* true - default, allows the device to override MPIR-level `MPI_Bcast` collective algorithms.
* false - disables the device override for `MPI_Bcast`.

Used in functions: MPI_Bcast

**MPIR_CVAR_BCAST_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Bcast_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Bcast_init.

* true - default, allows the device to override MPIR-level `MPI_Bcast_init` collective algorithms.
* false - disables the device override for `MPI_Bcast_init`.

Used in functions: MPI_Bcast_init

**MPIR_CVAR_BCAST_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Bcast algorithm. It is used for `MPI_Bcast` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking Bcast algorithm.
* remote_send_local_bcast - Forces the remote-send-local-bcast algorithm.

Used in functions: MPI_Bcast

**MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE** - COLLECTIVE

Sets the total shared-memory broadcast buffer size for POSIX shared-memory release-gather broadcast operations. It is captured when blocking release-gather state is initialized for a communicator and is read when nonblocking release-gather broadcast state is initialized. It is used only when broadcast release-gather buffer initialization is requested; together with `MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS`, it determines the broadcast cell capacity used for pipelining. The allocated broadcast shared memory also counts against `MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE`; exceeding that limit makes initialization fail and the collective fall back. When active, this CVAR sets the total broadcast shared-memory buffer capacity used by POSIX release-gather broadcast collectives.

* integer - default is 32768, represents the total broadcast buffer size in bytes.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init

**MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS** - COLLECTIVE

Sets the number of cells in the POSIX shared-memory release-gather broadcast buffer. It is captured when blocking release-gather state is initialized for a communicator and is read when nonblocking release-gather broadcast state is initialized and scheduled. For nonblocking broadcast, it sizes the per-cell flag shared memory, sizes and initializes the last-completed sequence-number tracking array, initializes per-cell gather and release flags, and maps each broadcast chunk sequence number to a shared-memory cell. Together with `MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE`, it determines the broadcast cell capacity used for pipelining. The associated shared-memory allocations count against `MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE`; exceeding that limit makes initialization fail and the collective fall back. When active, this CVAR sets the number of shared-memory broadcast cells used by POSIX release-gather broadcast collectives.

* integer - default is 4, represents the number of broadcast buffer cells.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init, MPIDI_POSIX_NB_RG_root_datacopy_completion, MPIDI_POSIX_NB_RG_finish_send_recv_completion, MPIDI_POSIX_NB_RG_update_release_flag_completion, MPIDI_POSIX_NB_RG_non_root_datacopy_cb, MPIDI_POSIX_NB_RG_gather_completion, MPIDI_POSIX_NB_RG_update_gather_flag_cb

**MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL** - COLLECTIVE

Sets the k value used for POSIX shared-memory release-gather broadcast trees. It is captured when blocking release-gather state is initialized for a communicator and is read when nonblocking release-gather tree state is initialized. Together with `MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE`, it is used to create the broadcast tree; topology-aware tree creation is attempted only when `MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES` is enabled, user-provided process binding is present, and hardware topology support is initialized, otherwise the non-topology-aware tree is created. When active, this CVAR sets the broadcast tree radix used by POSIX release-gather broadcast collectives.

* integer - default is 64, represents the broadcast tree k value.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init

**MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE** - COLLECTIVE

Sets the tree type used for POSIX shared-memory release-gather broadcast trees. It is captured when blocking or nonblocking release-gather state is initialized for a communicator. Together with `MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL`, it is used to create the broadcast tree; topology-aware tree creation is attempted only when `MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES` is enabled, user-provided process binding is present, and hardware topology support is initialized, otherwise the non-topology-aware tree is created. When active, this CVAR selects the broadcast tree shape used by POSIX release-gather broadcast collectives.

* kary - default, use a k-ary tree type.
* knomial_1 - use a knomial tree type where ranks are added in order from the left side.
* knomial_2 - use a knomial tree type where ranks are added in order from the right side; supported only with non-topology-aware trees.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init

**MPIR_CVAR_BCAST_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Bcast algorithm. It is used for `MPI_Bcast` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. The selected Bcast implementation may also use Bcast threshold, tree, topology, pipeline, or circ_graph CVARs to control algorithm parameters.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* binomial - Forces the Binomial Tree algorithm.
* nb - Forces the nonblocking Bcast algorithm.
* circ_graph - Forces the queued circulant graph algorithm.
* smp - Forces the SMP algorithm.
* scatter_recursive_doubling_allgather - Forces the Scatter Recursive-Doubling Allgather algorithm.
* scatter_ring_allgather - Forces the Scatter Ring algorithm.
* pipelined_tree - Forces the tree-based pipelined algorithm.
* tree - Forces the tree-based algorithm.

Used in functions: MPI_Bcast

**MPIR_CVAR_BCAST_IPC_READ_MSG_SIZE_THRESHOLD** - COLLECTIVE

Sets the message-size threshold, in bytes, for using POSIX GPU IPC read broadcast. It is used only when the POSIX GPU IPC read Bcast path is selected and GPU shared-memory support is enabled. The path also requires a contiguous datatype and GPU IPC-capable buffers from all ranks; otherwise it falls back to the MPIR Bcast implementation. When active, this CVAR sets the minimum broadcast payload size that can use GPU IPC read instead of fallback broadcast.

* integer - default is 256, represents the broadcast GPU IPC read message-size threshold in bytes.

Used in functions: MPIDI_POSIX_mpi_bcast_gpu_ipc_read

**MPIR_CVAR_BCAST_IS_NON_BLOCKING** - COLLECTIVE

Controls whether `MPI_Bcast` uses non-blocking send operations. It is used by `MPI_Bcast` and does not rely on other CVAR values or modes being active.

* false - Disables non-blocking send operations in `MPI_Bcast`.
* true - default, enables non-blocking send operations in `MPI_Bcast`.

Used in functions: MPI_Bcast

**MPIR_CVAR_BCAST_LONG_MSG_SIZE** - COLLECTIVE

Sets the long-message threshold, in bytes, used by broadcast sched-auto and SMP auto-selection paths. It is considered only after the message is not short according to `MPIR_CVAR_BCAST_SHORT_MSG_SIZE` and the communicator size is not below `MPIR_CVAR_BCAST_MIN_PROCS`. When active, it divides the remaining broadcasts between the medium-message power-of-two scatter/recursive-doubling-allgather path and the large-message or non-power-of-two scatter/ring-allgather path.

* integer - default is 524288, represents the broadcast long-message threshold in bytes.

Used in functions: MPIR_Ibcast_intra_sched_auto, MPIR_Bcast_intra_smp

**MPIR_CVAR_BCAST_MIN_PROCS** - COLLECTIVE

Sets the communicator-size threshold used by the SMP intra-communicator Bcast path. It is used only when `MPIR_Bcast_intra_smp` is selected, such as by forcing `MPIR_CVAR_BCAST_INTRA_ALGORITHM` to `smp` or by internal collective selection. Broadcasts whose communicator size is below this threshold, or whose message size is below `MPIR_CVAR_BCAST_SHORT_MSG_SIZE`, use the short-message SMP path. Otherwise, the SMP path selects between scatter/allgather variants based on `MPIR_CVAR_BCAST_LONG_MSG_SIZE` and whether the communicator size is a power of two.

* integer - default is 8, represents the minimum communicator size for using the SMP scatter/allgather selection path.

Used in functions: MPIR_Bcast_intra_smp

**MPIR_CVAR_BCAST_OFI_INTRA_ALGORITHM** - COLLECTIVE

Selects the CH4 OFI netmod intra-communicator Bcast path. It is used by the OFI netmod Bcast entry point, where it can route the operation to MPIR collective fallback, OFI triggered-operation tree implementations, or internal algorithm selection that can be overridden with `MPIR_CVAR_CH4_OFI_COLL_SELECTION_TUNING_JSON_FILE`. The triggered-operation paths require OFI triggered operations, OFI data auto-progress, and a datatype that can be converted to an OFI datatype; otherwise they fall back to the MPIR Bcast implementation. When a triggered-operation tree path is active, it also uses the Bcast tree type and k value selected by `MPIR_CVAR_BCAST_TREE_TYPE` and `MPIR_CVAR_BCAST_TREE_KVAL`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_CH4_OFI_COLL_SELECTION_TUNING_JSON_FILE`.
* mpir - Fallback to MPIR collectives.
* trigger_tree_tagged - Forces the triggered-operations based Tagged Tree algorithm.
* trigger_tree_rma - Forces the triggered-operations based RMA Tree algorithm.

Used in functions: MPIDI_NM_mpi_bcast

**MPIR_CVAR_BCAST_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Selects the algorithm used for POSIX shared-memory intranode broadcast. It is used in the POSIX broadcast implementation; automatic selection consults the POSIX collective-selection tables, and when hybrid memory support is disabled it first queries the broadcast buffer memory type so GPU device buffers can use the GPU-specific selection table. Automatic selection can fall back to the MPIR broadcast implementation when no POSIX selection is found. The shared-memory release-gather path is used only when MPICH is not threaded. When active, this CVAR sets whether POSIX intranode broadcast uses MPIR fallback, shared-memory release-gather broadcast, GPU IPC read broadcast, or internal POSIX collective selection.

* auto - default, use internal POSIX collective selection, with GPU-device-buffer selection when hybrid memory support is disabled.
* mpir - use the MPIR broadcast implementation.
* release_gather - use the POSIX shared-memory release-gather broadcast path when threading conditions allow it.
* ipc_read - use the POSIX GPU IPC read broadcast path.

Used in functions: MPIDI_POSIX_mpi_bcast

**MPIR_CVAR_BCAST_RECV_PRE_POST** - COLLECTIVE

Controls receive pre-posting for the pipelined-tree Bcast algorithm. It is used only when the pipelined-tree Bcast path runs with nonblocking sends enabled, such as by forcing `MPIR_CVAR_BCAST_INTRA_ALGORITHM` to `pipelined_tree` or by internal collective/tuning selection. When active, it sets whether receives for all pipeline chunks are posted up front or whether only the initial receives are pre-posted for larger chunk counts.

* false - default, pre-posts only the initial receives when there are more than three pipeline chunks.
* true - pre-posts receives for all pipeline chunks.

Used in functions: MPIR_Bcast_intra_pipelined_tree

**MPIR_CVAR_BCAST_SHORT_MSG_SIZE** - COLLECTIVE

Sets the short-message threshold, in bytes, used by broadcast auto-selection paths. It is used by sched-auto intra-communicator Ibcast, by the SMP intra-communicator Bcast path, and by the fallback flat auto-selection path for TSP Ibcast. Messages below this threshold, or broadcasts whose communicator size is below `MPIR_CVAR_BCAST_MIN_PROCS`, use a tree-style short-message path. Messages at or above this threshold use scatter/allgather paths, with the exact path also depending on `MPIR_CVAR_BCAST_LONG_MSG_SIZE` and whether the communicator size is a power of two in the sched-auto and SMP paths.

* integer - default is 12288, represents the broadcast short-message threshold in bytes.

Used in functions: MPIR_Ibcast_intra_sched_auto, MPIR_Bcast_intra_smp, MPIR_Ibcast_sched_intra_tsp_flat_auto

**MPIR_CVAR_BCAST_TOPO_DIFF_GROUPS** - COLLECTIVE

Sets the latency cost used for communication between different topology groups when constructing topology-wave Bcast trees. It is used only by the intra-communicator tree and pipelined-tree Bcast implementations when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_BCAST_TREE_TYPE` together with the tree or pipelined-tree Bcast algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_BCAST_TOPO_REORDER_ENABLE`, `MPIR_CVAR_BCAST_TOPO_OVERHEAD`, and the Bcast topology latency CVARs for different switches and same switches. When collective tuning selection provides an intra-tree Bcast container, that container's different-groups topology latency value overrides this CVAR for the non-pipelined tree construction.

* integer - default is 2800, represents the topology-wave latency cost between different groups.

Used in functions: MPIR_Bcast_intra_tree, MPIR_Bcast_intra_pipelined_tree

**MPIR_CVAR_BCAST_TOPO_DIFF_SWITCHES** - COLLECTIVE

Sets the latency cost used for communication between different switches in the same topology group when constructing topology-wave Bcast trees. It is used only by the intra-communicator tree and pipelined-tree Bcast implementations when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_BCAST_TREE_TYPE` together with the tree or pipelined-tree Bcast algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_BCAST_TOPO_REORDER_ENABLE`, `MPIR_CVAR_BCAST_TOPO_OVERHEAD`, and the Bcast topology latency CVARs for different groups and same switches. When collective tuning selection provides an intra-tree Bcast container, that container's different-switches topology latency value overrides this CVAR for the non-pipelined tree construction.

* integer - default is 1900, represents the topology-wave latency cost between different switches in the same group.

Used in functions: MPIR_Bcast_intra_tree, MPIR_Bcast_intra_pipelined_tree

**MPIR_CVAR_BCAST_TOPO_OVERHEAD** - COLLECTIVE

Sets the fixed overhead cost used when constructing topology-wave Bcast trees. It is used only by the intra-communicator tree and pipelined-tree Bcast implementations when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_BCAST_TREE_TYPE` together with the tree or pipelined-tree Bcast algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_BCAST_TOPO_REORDER_ENABLE` and the Bcast topology latency CVARs for different groups, different switches, and same switches. When collective tuning selection provides an intra-tree Bcast container, that container's topology overhead value overrides this CVAR for the non-pipelined tree construction.

* integer - default is 200, represents the topology-wave overhead cost.

Used in functions: MPIR_Bcast_intra_tree, MPIR_Bcast_intra_pipelined_tree

**MPIR_CVAR_BCAST_TOPO_SAME_SWITCHES** - COLLECTIVE

Sets the latency cost used for communication within the same switch when constructing topology-wave Bcast trees. It is used only by the intra-communicator tree and pipelined-tree Bcast implementations when the selected tree type is `topology_wave`, such as through `MPIR_CVAR_BCAST_TREE_TYPE` together with the tree or pipelined-tree Bcast algorithm or collective tuning selection. Topology-wave tree construction also uses `MPIR_CVAR_BCAST_TOPO_REORDER_ENABLE`, `MPIR_CVAR_BCAST_TOPO_OVERHEAD`, and the Bcast topology latency CVARs for different groups and different switches. When collective tuning selection provides an intra-tree Bcast container, that container's same-switches topology latency value overrides this CVAR for the non-pipelined tree construction.

* integer - default is 1600, represents the topology-wave latency cost within the same switch.

Used in functions: MPIR_Bcast_intra_tree, MPIR_Bcast_intra_pipelined_tree

**MPIR_CVAR_BCAST_TREE_KVAL** - COLLECTIVE

Sets the k value for tree-based Bcast algorithms. It is used by the OFI netmod Bcast path when `MPIR_CVAR_BCAST_OFI_INTRA_ALGORITHM` selects `trigger_tree_tagged` or `trigger_tree_rma`; those paths also use the Bcast tree type selected by `MPIR_CVAR_BCAST_TREE_TYPE` and require triggered operations, data auto-progress, and an OFI-supported datatype, otherwise they fall back to the MPIR Bcast implementation.

* integer - default is 2, represents the k value used by tree-based Bcast algorithms.

Used in functions: MPIDI_NM_mpi_bcast, MPIDI_OFI_Bcast_intra_triggered_tagged, MPIDI_OFI_Bcast_intra_triggered_rma

**MPIR_CVAR_BCAST_TREE_PIPELINE_CHUNK_SIZE** - COLLECTIVE

Sets the chunk size, in bytes, used by the pipelined Bcast tree algorithm. It is used only when the pipelined-tree Bcast path is selected, such as by forcing `MPIR_CVAR_BCAST_INTRA_ALGORITHM` to `pipelined_tree` or by internal collective/tuning selection. When active, the Bcast payload is split into chunks of this size for pipeline movement along the selected tree, which also depends on the Bcast tree type and k value.

* integer - default is 8192, represents the chunk size in bytes used by the pipelined Bcast tree algorithm.

Used in functions: MPIR_Bcast_intra_pipelined_tree

**MPIR_CVAR_BCAST_TREE_TYPE** - COLLECTIVE

Sets the tree type used by tree-based Bcast algorithms. It is read during collective initialization and stored as the global Bcast tree type. It is used when a tree-based Bcast path is selected, such as by forcing `MPIR_CVAR_BCAST_INTRA_ALGORITHM` to `tree` or `pipelined_tree`, or by internal collective/tuning selection. The tree construction also uses the Bcast tree k value, and topology-based tree types rely on the Bcast topology CVARs for reordering and topology cost parameters.

* kary - default, represents a k-ary tree type.
* knomial_1 - represents a knomial tree type that grows starting from the left of the root.
* knomial_2 - represents a knomial tree type that grows starting from the right of the root.
* topology_aware - represents a topology-aware tree type.
* topology_aware_k - represents a topology-aware tree type with branching factor k.
* topology_wave - represents a topology-wave tree type.

Used in functions: MPII_Coll_init

**MPIR_CVAR_CH3_COMM_CONNECT_TIMEOUT** - CH3

Sets the default timeout period for CH3 dynamic-process connection attempts. It is used by the root process in `MPI_Comm_connect` when connecting to a server communicator whose named port exists but has no pending accept, and the per-call MPI info key `timeout` overrides this CVAR when provided. It does not rely on another CVAR being enabled, but it is active only for CH3 builds with dynamic-process support. When active, this CVAR sets how long the connecting side waits for the accept-side handshake before revoking the connection request and returning a port error.

* 180 - default, wait up to 180 seconds for the accept-side handshake.
* positive integer - wait up to the specified number of seconds for the accept-side handshake.
* 0 or negative integer - do not continue waiting after the initial progress check.

Used in functions: MPIDI_Comm_connect, MPIDI_Create_inter_root_communicator_connect

**MPIR_CVAR_CH3_EAGER_MAX_MSG_SIZE** - CH3

Sets the eager-send message-size threshold for CH3 virtual connections. It is used when a CH3 virtual connection is initialized, after the ready-send eager threshold is set to no limit and before channel-specific virtual-connection initialization runs. It does not rely on another CVAR or mode being active. When active, this CVAR sets the maximum CH3 message size sent eagerly before switching to rendezvous mode.

* 131072 - default, send CH3 messages eagerly up to 131072 bytes before switching to rendezvous mode.
* integer - use the specified byte threshold for switching CH3 messages from eager to rendezvous mode.

Used in functions: MPIDI_VC_Init

**MPIR_CVAR_CH3_ENABLE_HCOLL** - CH3

Controls whether CH3 enables HCOLL collectives. It is used during CH3 communicator initialization when MPICH is built with HCOLL support; in that path MPICH sets HCOLL environment defaults if the user has not already provided them, then registers HCOLL communicator create and destroy hooks. It does not rely on another CVAR being enabled, but it is effective only in builds where HCOLL support is compiled in. When active, this CVAR enables HCOLL collective support for CH3 communicators.

* false - default, do not enable HCOLL collectives.
* true - enable HCOLL collectives.

Alternate name: MPIR_CVAR_ENABLE_HCOLL

Used in functions: MPIDI_CH3I_Comm_init

**MPIR_CVAR_CH3_INTERFACE_HOSTNAME** - CH3

Specifies the hostname or IP address advertised for this process when the CH3 nemesis TCP netmod builds its business card, so other processes use that address when connecting to it. It is used only when `MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE` is not set; setting both is an error. If this CVAR is not set, MPICH falls back to the PMI hostname, then a per-rank interface hostname environment variable, and then local processor/interface lookup. When active, this CVAR sets the advertised host description and socket address used for TCP connections to this process.

* NULL - default, do not set an advertised hostname or IP address through this CVAR.
* string - hostname or IP address to advertise for connections to this process.

Alternate name: MPIR_CVAR_INTERFACE_HOSTNAME

Used in functions: GetSockInterfaceAddr, MPID_nem_tcp_get_business_card

**MPIR_CVAR_CH3_PG_VERBOSE** - CH3

Enables verbose CH3 process-group output. It is used when a process group is created and when process groups are finalized, and it does not rely on another CVAR or mode being active. When active, this CVAR prints process-group creation messages and process-group state information to stdout.

* false - default, do not print verbose CH3 process-group output.
* true - print verbose CH3 process-group output.

Used in functions: MPIDI_PG_Create, MPIDI_PG_Finalize

**MPIR_CVAR_CH3_PORT_RANGE** - CH3

Sets the TCP port range used by the CH3 nemesis TCP listener and recognized by the Hydra process manager. It is used when Hydra checks environment settings for a requested port range and when the CH3 nemesis TCP netmod binds its listener socket while initializing TCP connectivity. The TCP listener validates that the lower bound is nonnegative and not greater than the upper bound before binding. When active, this CVAR sets the port-selection range for process-manager and MPICH TCP sockets.

* 0:0 - default, bind the TCP listener to any available port.
* low:high - bind the TCP listener to an available port in the requested inclusive range.

Alternate name: MPIR_CVAR_PORTRANGE, MPIR_CVAR_PORT_RANGE

Used in functions: check_environment, MPID_nem_tcp_listen

**MPIR_CVAR_CH3_RMA_ACTIVE_REQ_THRESHOLD** - CH3

Sets the active-request threshold at which CH3 RMA operation routines block for progress. It is used after nonlocal CH3 RMA operations are queued and progress is attempted; local, shared-window, and same-node shared-memory operations bypass this path. It does not rely on another CVAR being enabled, but it is active only for CH3 RMA operations that use the queued network path and when the active CH3 RMA request count reaches the configured threshold. When active, this CVAR sets the number of active CH3 RMA requests that triggers blocking progress in the operation routine.

* 65536 - default, wait for progress when the active CH3 RMA request count reaches 65536.
* negative integer - never block in the operation routine based on the active CH3 RMA request count.
* 0 - always trigger blocking progress in the operation routine until no active CH3 RMA requests remain.
* positive integer - wait for progress when the active CH3 RMA request count reaches the specified threshold.

Used in functions: MPIDI_CH3I_Accumulate, MPIDI_CH3I_Get, MPIDI_CH3I_Get_accumulate, MPIDI_CH3I_Put, MPID_Compare_and_swap, MPID_Fetch_and_op

**MPIR_CVAR_CH3_RMA_OP_GLOBAL_POOL_SIZE** - CH3

Sets the number of RMA operation entries allocated in the CH3 global operation pool during RMA initialization. It is used when CH3 RMA initializes its global operation pool and does not rely on another CVAR or mode being active. When active, this CVAR sets how many preallocated global operation records are available for RMA operations that cannot be issued immediately.

* 16384 - default, allocate 16384 operation entries in the CH3 global RMA operation pool.
* positive integer - allocate the requested number of operation entries in the CH3 global RMA operation pool.

Used in functions: MPIDI_RMA_init

**MPIR_CVAR_CH3_RMA_OP_PIGGYBACK_LOCK_DATA_SIZE** - CH3

Sets the operation data-size threshold for marking CH3 RMA operations as candidates to piggyback a lock. It is used when nonlocal CH3 RMA put, accumulate, and get-accumulate operations are queued; local and shared-memory operations bypass this path. The operation must use predefined datatypes, except that get-accumulate with `MPI_NO_OP` has an empty origin buffer and can still be considered when the result and target datatypes are predefined. Immediate RMA packet selection is checked separately against `MPIDI_RMA_IMMED_BYTES`. When active, this CVAR sets the maximum operation data size that can be considered for piggybacking a lock with the RMA operation.

* 65536 - default, consider eligible RMA operations up to 65536 bytes for piggybacking a lock.
* positive integer - consider eligible RMA operations up to the requested number of bytes for piggybacking a lock.

Used in functions: MPIDI_CH3I_Accumulate, MPIDI_CH3I_Get_accumulate, MPIDI_CH3I_Put

**MPIR_CVAR_CH3_RMA_OP_WIN_POOL_SIZE** - CH3

Sets the number of RMA operation entries allocated in each CH3 window-private operation pool during window creation. It is used after the window communicator has been duplicated and does not rely on another CVAR or mode being active, but it is active only for CH3 RMA windows initialized through the common window initialization path. When active, this CVAR sets how many preallocated operation records are available from the window-private pool for RMA operations that cannot be issued immediately.

* 256 - default, allocate 256 operation entries in each CH3 window-private RMA operation pool.
* positive integer - allocate the requested number of operation entries in each CH3 window-private RMA operation pool.

Used in functions: win_init

**MPIR_CVAR_CH3_RMA_POKE_PROGRESS_REQ_THRESHOLD** - CH3

Sets the active-request threshold at which CH3 RMA operation issuing pokes the progress engine. It is used while issuing queued RMA operations to a target after an operation is issued but its request has not completed. It does not rely on another CVAR being enabled, but it is active only for CH3 RMA progress paths that issue queued operations for a ready window and when the active CH3 RMA request count is above the configured threshold. When active, this CVAR sets the number of active CH3 RMA requests that triggers a progress-engine poke during RMA operation issuing.

* 128 - default, poke the progress engine when the active CH3 RMA request count is above 128.
* integer - poke the progress engine when the active CH3 RMA request count is above the specified threshold.

Used in functions: issue_ops_target

**MPIR_CVAR_CH3_RMA_SCALABLE_FENCE_PROCESS_NUM** - CH3

Sets the process-count threshold for switching CH3 RMA fence synchronization from the basic algorithm to the scalable algorithm. It is used by `MPI_Win_fence` after fence synchronization state is validated, and it does not rely on another CVAR being enabled. The selected fence path can still be bypassed by `MPI_MODE_NOPRECEDE`, either by completing immediately when `MPI_MODE_NOSUCCEED` is also asserted or by starting the next epoch through an immediate or nonblocking barrier. When active, this CVAR sets the communicator size at which CH3 RMA fence uses the scalable synchronization algorithm instead of the basic algorithm.

* 1024 - default, use the scalable CH3 RMA fence algorithm when the window communicator has at least 1024 processes.
* positive integer - use the scalable CH3 RMA fence algorithm when the window communicator has at least the requested number of processes; otherwise use the basic algorithm.
* 0 or negative integer - always use the scalable CH3 RMA fence algorithm for normal fence synchronization.

Used in functions: MPID_Win_fence

**MPIR_CVAR_CH3_RMA_SLOTS_SIZE** - CH3

Sets the number of target-list slots allocated for each CH3 RMA window during window creation. It is used after the window communicator has been duplicated, and the actual slot count is capped by the window communicator size. It does not rely on another CVAR or mode being active, but it is active only for CH3 RMA windows initialized through the common window initialization path. When active, this CVAR sets the number of round-robin hash slots used to organize RMA target elements for a window.

* 262144 - default, allocate up to 262144 target-list slots per CH3 RMA window, capped by the window communicator size.
* positive integer - allocate up to the requested number of target-list slots per CH3 RMA window, capped by the window communicator size.

Used in functions: win_init

**MPIR_CVAR_CH3_RMA_TARGET_GLOBAL_POOL_SIZE** - CH3

Sets the number of RMA target entries allocated in the CH3 global target pool during RMA initialization. It is used when CH3 RMA initializes its global target pool and does not rely on another CVAR or mode being active. When active, this CVAR sets how many preallocated global target records are available for RMA targets that cannot be issued immediately.

* 16384 - default, allocate 16384 target entries in the CH3 global RMA target pool.
* positive integer - allocate the requested number of target entries in the CH3 global RMA target pool.

Used in functions: MPIDI_RMA_init

**MPIR_CVAR_CH3_RMA_TARGET_LOCK_DATA_BYTES** - CH3

Sets the per-window byte limit for buffering target-side lock operation data in CH3 RMA. It is used when a passive-target lock request carrying operation data cannot acquire the target window lock and the request is queued on the target; lock-only, immediate-operation, and get-only packets do not consume this data buffer. The check compares the current buffered lock data for the window against this CVAR, and if buffering would exceed the limit, MPICH queues only the lock request and drops the incoming operation data so the origin retransmits it later. When active, this CVAR sets how much queued lock-operation data a target window may buffer.

* 655360 - default, buffer up to 655360 bytes of queued target lock-operation data per window.
* positive integer - buffer up to the requested number of bytes of queued target lock-operation data per window.

Used in functions: enqueue_lock_origin

**MPIR_CVAR_CH3_RMA_TARGET_LOCK_ENTRY_WIN_POOL_SIZE** - CH3

Sets the number of target-side RMA lock-entry records allocated in each CH3 window-private lock-entry pool during window creation. It is used after the window communicator has been duplicated and does not rely on another CVAR or mode being active, but it is active only for CH3 RMA windows initialized through the common window initialization path. When active, this CVAR sets how many preallocated target lock-entry records are available for RMA lock requests that cannot be satisfied immediately.

* 256 - default, allocate 256 target lock-entry records in each CH3 window-private RMA lock-entry pool.
* positive integer - allocate the requested number of target lock-entry records in each CH3 window-private RMA lock-entry pool.

Used in functions: win_init

**MPIR_CVAR_CH3_RMA_TARGET_WIN_POOL_SIZE** - CH3

Sets the number of RMA target entries allocated in each CH3 window-private target pool during window creation. It is used after the window communicator has been duplicated, and the actual pool size is capped by the window communicator size. It does not rely on another CVAR or mode being active, but it is active only for CH3 RMA windows initialized through the common window initialization path. When active, this CVAR sets how many preallocated target records are available from the window-private pool for RMA targets that cannot be issued immediately.

* 256 - default, allocate up to 256 target entries in each CH3 window-private RMA target pool, capped by the window communicator size.
* positive integer - allocate up to the requested number of target entries in each CH3 window-private RMA target pool, capped by the window communicator size.

Used in functions: win_init

**MPIR_CVAR_CH4_CMA_ENABLE** - CH4

Controls whether the CH4 CMA shared-memory IPC path may be selected for intranode point-to-point communication. It is used only when MPICH is built with the CMA shared-memory submodule; during CMA local initialization, MPICH may force this setting off when Linux Yama ptrace permissions do not allow CMA. The CMA path is selected only for messages at least as large as `MPIR_CVAR_CH4_IPC_CMA_P2P_THRESHOLD` and datatypes whose contiguous-block count fits in the CMA iovec limit. When active, this CVAR enables use of CMA-based single-copy IPC transfers.

* true - default, allow eligible intranode point-to-point messages to use CMA-based single-copy IPC transfers.
* false - disable selection of the CMA-based IPC transfer path.

Used in functions: MPIDI_CMA_init_local, MPIDI_CMA_get_ipc_attr

**MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE** - COLLECTIVE

Sets the tuning JSON source used to initialize the CH4 collective-selection table during `MPID_Init`. If this CVAR is empty, CH4 initializes collective selection from the built-in generic JSON buffer; otherwise, it loads the collective-selection table from the specified file and records that file as the selection source. In `src/mpid/ch4/src/ch4_init.c`, this CVAR controls the regular CH4 collective-selection table independently from the GPU collective-selection table controlled by `MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE_GPU`. When active, this CVAR sets the source of CH4 collective-selection tuning data used by non-GPU collective selection.

* "" - default, use the built-in generic collective-selection JSON buffer.
* file path - load the CH4 collective-selection tuning JSON data from the specified file.

Used in functions: MPID_Init

**MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE_GPU** - COLLECTIVE

Sets the tuning JSON source used to initialize the CH4 GPU collective-selection table during `MPID_Init`. If this CVAR is empty, CH4 initializes GPU collective selection from the built-in generic JSON buffer; otherwise, it loads the GPU collective-selection table from the specified file and records that file as the GPU selection source. In `src/mpid/ch4/src/ch4_init.c`, this CVAR controls the GPU collective-selection table independently from the regular CH4 collective-selection table controlled by `MPIR_CVAR_CH4_COLL_SELECTION_TUNING_JSON_FILE`. The GPU collective-selection table is initialized unconditionally during `MPID_Init`; only debug-summary printing of the GPU selection source depends on `MPIR_CVAR_ENABLE_GPU`. When active, this CVAR sets the source of CH4 collective-selection tuning data used by GPU collective selection.

* "" - default, use the built-in generic collective-selection JSON buffer for GPU collective selection.
* file path - load the CH4 GPU collective-selection tuning JSON data from the specified file.

Used in functions: MPID_Init

**MPIR_CVAR_CH4_COMM_CONNECT_TIMEOUT** - CH4

Sets the default timeout period for CH4 dynamic-process connection attempts. It is used by the root process in `MPI_Comm_connect` when connecting to a server communicator whose named port exists but has no pending accept, and the per-call MPI info key `timeout` overrides this CVAR when provided. It does not rely on another CVAR being enabled. When active, this CVAR sets how long the connecting side waits for the accept-side handshake before returning a port error.

* 180 - default, wait up to 180 seconds for the accept-side handshake.
* positive integer - wait up to the specified number of seconds for the accept-side handshake.
* 0 or negative integer - do not continue waiting after the initial progress check.

Used in functions: MPID_Comm_connect, dynamic_intercomm_create, establish_peer_conn

**MPIR_CVAR_CH4_ENABLE_STREAM_WORKQ** - CH4

Controls whether CH4 stream enqueue operations use the stream work queue path. It is used by send, receive, nonblocking send, nonblocking receive, wait, and waitall enqueue operations; when disabled, those operations call the regular enqueue implementation instead. When enabled, the communicator or enqueue request must be associated with an MPIX GPU stream and that stream must provide the work queue, with progress supplied by the corresponding stream progress thread. When active, this CVAR enables deferring stream enqueue operations through the stream work queue using GPU trigger and completion events.

* false - default, use the regular stream enqueue implementation instead of the stream work queue path.
* true - use the stream work queue path for CH4 stream enqueue operations.

Used in functions: MPID_Send_enqueue, MPID_Recv_enqueue, MPID_Isend_enqueue, MPID_Irecv_enqueue, MPID_Wait_enqueue, MPID_Waitall_enqueue

**MPIR_CVAR_CH4_GLOBAL_PROGRESS** - CH4

Sets how often CH4 progress polls all VCIs for global progress. It is used only when CH4 is built with support for more than one VCI and the current progress state is not limited to an explicit VCI; if only one VCI is active, global polling is skipped regardless of this CVAR. When global polling is selected, progress is attempted on every active VCI, otherwise progress is attempted only on the VCIs listed in the current progress state. When active, this CVAR sets the frequency for polling every VCI during CH4 progress.

* low - poll all VCIs very infrequently.
* normal - default, poll all VCIs at the normal frequency for typical applications.
* high - poll all VCIs on every eligible progress check.

Used in functions: MPIDI_do_global_progress, MPIDI_progress_test, MPIDI_progress_test_vci

**MPIR_CVAR_CH4_GPU_COLL_MAX_NUM_BUFFERS** - CH4_OFI

Sets the maximum number of registered host buffers in the CH4 GPU collective private buffer pool. In `src/mpid/ch4/src/ch4_init.c`, this CVAR is used during `MPID_Init` when `MPIDU_genq_private_pool_create` creates `MPIDI_global.gpu_coll_pool`; that pool also uses `MPIR_CVAR_CH4_GPU_COLL_SWAP_BUFFER_SZ` as the size of each buffer and `MPIR_CVAR_CH4_GPU_COLL_NUM_BUFFERS_PER_CHUNK` as the number of buffers allocated per chunk. The pool is created unconditionally during `MPID_Init` in the reviewed file, with no dependency on `MPIR_CVAR_ENABLE_GPU` or another mode being active. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_init.c`. When active, this CVAR sets the total buffer limit for the registered host buffer pool used by GPU collective data transfer.

* 256 - default, allow up to 256 GPU collective transfer buffers in the pool.
* integer - allow up to the specified number of GPU collective transfer buffers in the pool.

Used in functions: MPID_Init

**MPIR_CVAR_CH4_GPU_COLL_NUM_BUFFERS_PER_CHUNK** - CH4_OFI

Sets the number of registered host buffers allocated in each chunk of the CH4 GPU collective private buffer pool. In `src/mpid/ch4/src/ch4_init.c`, this CVAR is used during `MPID_Init` when `MPIDU_genq_private_pool_create` creates `MPIDI_global.gpu_coll_pool`; that pool also uses `MPIR_CVAR_CH4_GPU_COLL_SWAP_BUFFER_SZ` as the size of each buffer and `MPIR_CVAR_CH4_GPU_COLL_MAX_NUM_BUFFERS` as the total buffer limit. The pool is created unconditionally during `MPID_Init` in the reviewed file, with no dependency on `MPIR_CVAR_ENABLE_GPU` or another mode being active. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_init.c`. When active, this CVAR sets the chunk size, in buffers, for the registered host buffer pool used by GPU collective data transfer.

* 1 - default, create one GPU collective transfer buffer per pool chunk.
* integer - create the specified number of GPU collective transfer buffers per pool chunk.

Used in functions: MPID_Init

**MPIR_CVAR_CH4_GPU_COLL_SWAP_BUFFER_SZ** - CH4_OFI

Sets the registered host swap-buffer size used by CH4 GPU collectives. In `src/mpid/ch4/src/ch4_init.c`, this CVAR sets the cell size for the GPU collective private buffer pool; the pool also uses `MPIR_CVAR_CH4_GPU_COLL_NUM_BUFFERS_PER_CHUNK` and `MPIR_CVAR_CH4_GPU_COLL_MAX_NUM_BUFFERS` to determine its chunking and maximum number of buffers. In `src/mpid/ch4/src/ch4_coll_impl.h`, selected CH4 broadcast and allreduce compositions use this CVAR as the maximum message size for swapping strict-device GPU buffers through host memory; the swap path is used only when the computed datatype span is no larger than this buffer size, and the delta broadcast path also requires a node-roots communicator for host-buffer allocation. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_init.c` or `src/mpid/ch4/src/ch4_coll_impl.h`. When active, this CVAR sets both the size of each registered host buffer available for GPU collective data transfer and the size threshold for using those buffers in the reviewed GPU collective swap paths.

* 1048576 - default, use one MiB registered host swap buffers.
* non-negative integer - use the specified number of bytes for each registered host swap buffer and as the maximum datatype span eligible for the reviewed GPU collective swap paths.

Used in functions: MPID_Init, MPIDI_Bcast_intra_composition_alpha, MPIDI_Bcast_intra_composition_beta, MPIDI_Bcast_intra_composition_gamma, MPIDI_Bcast_intra_composition_delta, MPIDI_Allreduce_intra_composition_alpha, MPIDI_Allreduce_intra_composition_beta, MPIDI_Allreduce_intra_composition_gamma

**MPIR_CVAR_CH4_IOV_DENSITY_MIN** - CH4

Sets the minimum datatype or receive IOV density for choosing direct IOV-based transfer paths instead of packing, unpacking, or active-message fallback paths. It is used for noncontiguous OFI RMA put and get operations after native OFI RMA is available, the target memory region can be prepared, and GPU RMA is permitted; for UCX noncontiguous receives when UCX datatype receives are disabled and UCX IOV receives are compiled in; and for OFI long active-message RDMA-read receives when deciding whether sparse multi-IOV receives should unpack through a temporary buffer. When active, this CVAR sets the density threshold used to select high-density datatype IOV paths for CH4 OFI and UCX communication.

* 16384 - default, require density of at least 16384 bytes per contiguous block or receive IOV before selecting the high-density IOV path.
* positive integer - use the specified density threshold; larger values favor packing, unpacking, or fallback paths for more sparse datatypes.
* 0 or negative integer - treat all nonnegative-density datatypes as high density for threshold comparisons, and do not select OFI long active-message unpacking solely because average receive IOV size is below this threshold.

Used in functions: MPIDI_OFI_do_put, MPIDI_OFI_do_get, MPIDI_UCX_recv, do_long_am_recv

**MPIR_CVAR_CH4_IPC_CMA_P2P_THRESHOLD** - CH4

Sets the minimum send message size, in bytes, for selecting the CMA-based single-copy protocol for intranode communication. It is used only when MPICH is built with the CMA shared-memory submodule and `MPIR_CVAR_CH4_CMA_ENABLE` is enabled; datatypes whose contiguous-block count exceeds the CMA iovec limit are not selected for CMA. When active, this CVAR sets the point-to-point size threshold for eligible CMA-based IPC transfers.

* 8192 - default, select CMA-based single-copy IPC transfers for eligible messages of at least 8192 bytes.
* integer - select CMA-based single-copy IPC transfers for eligible messages whose size is greater than or equal to the specified byte threshold.

Used in functions: MPIDI_CMA_get_ipc_attr

**MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE** - CH4

Controls the size behavior of the cache containing GPU IPC mapped buffers. It is used during GPU initialization only when `MPIR_CVAR_ENABLE_GPU` is enabled; the selected value is stored in `MPL_gpu_info.max_cache_entries` before MPL GPU initialization. This setting currently affects only the specialized GPU IPC cache mechanism selected by `MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE`. When active, this CVAR sets whether that cache is unrestricted, limited by another CVAR, or disabled.

* unlimited - do not restrict the specialized GPU IPC mapped-buffer cache size.
* limited - default, limit the specialized GPU IPC mapped-buffer cache size based on `MPIR_CVAR_CH4_IPC_GPU_MAX_CACHE_ENTRIES`.
* disabled - disable the specialized GPU IPC mapped-buffer cache by setting the maximum cache entries to 0.

Used in functions: MPII_init_gpu

**MPIR_CVAR_CH4_IPC_GPU_ENGINE_TYPE** - CH4

Sets the GPU engine type used for CH4 shared-memory GPU IPC point-to-point data copies. It is used only when MPICH is built with the GPU shared-memory IPC submodule and the GPU IPC path is selected for intranode communication; that selection requires eligible GPU device buffers and buffers large enough for `MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD` unless the address is detected as repeated. The selected copy direction can also depend on `MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL`. When active, this CVAR selects the MPL GPU transfer engine used by asynchronous GPU IPC read and write copies.

* auto - default, choose the copy engine automatically from the source and destination device relationship.
* compute - use a compute engine.
* copy_high_bandwidth - use a high-bandwidth copy engine.
* copy_low_latency - use a low-latency copy engine.

Used in functions: MPIDI_IPCI_choose_engine

**MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE** - CH4

Controls how CH4 GPU IPC handles and mapped buffers are cached. It is used during GPU initialization when `MPIR_CVAR_ENABLE_GPU` is enabled to request specialized backend caching, with fallback to generic caching if the backend does not support specialized caching. It is used by the GPU IPC shmmod when creating, mapping, unmapping, destroying, and finalizing IPC handles; IPC handle creation is reached only for GPU buffers selected for GPU IPC, including buffers that meet the `MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD` requirement or repeat-address condition. When active, this CVAR enables the selected GPU IPC handle cache mechanism or disables IPC handle caching.

* specialized - default, use the GPU-specific MPL specialized cache mechanism when available; fall back to the generic cache mechanism if unavailable.
* generic - use the generic CH4 GPU IPC handle and mapped-buffer cache mechanism.
* disabled - disable GPU IPC handle caching, unmap handles after use, and destroy created handles when sends complete.

Used in functions: MPII_init_gpu, MPIDI_GPU_fill_ipc_handle, MPIDI_GPU_ipc_handle_map, MPIDI_GPU_ipc_handle_unmap, MPIDI_GPU_send_complete, ipc_handle_free_hook

**MPIR_CVAR_CH4_IPC_GPU_MAX_CACHE_ENTRIES** - CH4

Sets the maximum number of entries per device for the GPU IPC mapped-buffer cache. It is used during GPU initialization only when `MPIR_CVAR_ENABLE_GPU` is enabled and `MPIR_CVAR_CH4_IPC_GPU_CACHE_SIZE` is `limited`; in that case, the value is copied to `MPL_gpu_info.max_cache_entries` before MPL GPU initialization. This maximum currently affects only the specialized GPU IPC cache mechanism selected by `MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE`. When active, this CVAR sets the per-device entry limit for the specialized GPU IPC mapped-buffer cache.

* 16 - default, use 16 entries per device as the specialized GPU IPC mapped-buffer cache limit.
* positive integer - maximum number of specialized GPU IPC mapped-buffer cache entries per device.

Used in functions: MPII_init_gpu

**MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD** - CH4

Sets the GPU IPC peer-to-peer threshold for intranode communication. It is used during GPU initialization only when `MPIR_CVAR_ENABLE_GPU` is enabled; if the MPL GPU backend does not support IPC, MPICH sets this CVAR to disable size-threshold selection. It is then used only when the GPU IPC shmmod is enabled and examines otherwise eligible strict GPU device buffers; GPU IPC is selected when the buffer bounds length meets the threshold or when the buffer base address has been seen repeatedly. When active, this CVAR sets the minimum GPU buffer bounds length used to select the GPU-based single-copy IPC protocol.

* 1048576 - default, select GPU IPC for otherwise eligible strict GPU device buffers whose buffer bounds length is at least 1048576 bytes, and for repeat buffer base addresses.
* 0 - select GPU IPC for all otherwise eligible strict GPU device buffers.
* positive integer - minimum GPU buffer bounds length in bytes for selecting GPU IPC; smaller buffers use GPU IPC only for repeat buffer base addresses.
* -1 - disable size-threshold selection for non-repeat buffer base addresses; this value is set during GPU initialization when the MPL GPU backend does not support IPC.

Used in functions: MPII_init_gpu, MPIDI_GPU_get_ipc_attr

**MPIR_CVAR_CH4_IPC_GPU_READ_WRITE_PROTOCOL** - CH4

Selects the read/write protocol behavior for CH4 shared-memory GPU IPC data copies by choosing which device maps the remote IPC buffer. It is used only when MPICH is built with the GPU shared-memory IPC submodule and the GPU IPC path is selected for intranode communication; that selection requires eligible strict GPU device buffers and buffers large enough for `MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD` unless the address is detected as repeated. Protocol selection also depends on whether the remote device is visible locally and, for automatic selection, whether the datatype is contiguous. When active, this CVAR sets the GPU IPC buffer-mapping side used by point-to-point GPU IPC copies.

* read - default, map the remote IPC buffer on the local device.
* write - map the remote IPC buffer on the remote device when that device is visible locally; otherwise map it on the local device.
* auto - map the remote IPC buffer on the remote device when that device is visible locally and the datatype is contiguous; otherwise map it on the local device.

Used in functions: MPIDI_GPU_ipc_get_map_dev

**MPIR_CVAR_CH4_IPC_GPU_RMA_ENGINE_TYPE** - CH4

Sets the GPU engine type used for CH4 POSIX shared-memory RMA put and get GPU-copy paths. It is used only when MPICH is built with GPU support and the POSIX shared-memory RMA path handles the operation locally or through shared window memory. If this CVAR selects the non-GPU-engine path, POSIX RMA put and get fall back to the generic local-copy path instead. The memory-registration-preferred window mode changes whether the GPU copy is issued through a nonblocking local-copy request, but does not change which engine this CVAR selects. No dependency on another CVAR is visible in the reviewed files. When active, this CVAR selects the MPL GPU transfer engine used for eligible POSIX shared-memory RMA put and get copies.

* auto - default, choose a high-bandwidth copy engine when either buffer is host memory or both GPU buffers are on the same device; otherwise choose a low-latency copy engine.
* yaksa - use the generic local-copy path instead of selecting an MPL GPU engine.
* compute - use a compute engine.
* copy_high_bandwidth - use a high-bandwidth copy engine.
* copy_low_latency - use a low-latency copy engine.

Used in functions: MPIDI_RMA_choose_engine, MPIDI_POSIX_do_put, MPIDI_POSIX_do_get

**MPIR_CVAR_CH4_IPC_MAP_REPEAT_ADDR** - CH4

Enables tracking of how often the same buffer address is sent repeatedly so the CH4 shared-memory IPC path can consider repeated-address use when deciding whether to use an IPC algorithm. It is used by the repeat-address helper only when this CVAR is enabled; the IPC selection effect is driver-dependent and can be combined with message-size threshold checks. For high-latency buffers such as GPU device buffers, IPC may be selected when either the message size threshold is met or the buffer address is repeated. For drivers with relatively high address-mapping overhead, such as XPMEM, IPC may require both the message size condition and repeated-address detection. No dependency on another CVAR is visible in the reviewed file. When active, this CVAR enables repeated-buffer-address tracking for CH4 IPC selection.

* true - default, track repeated buffer addresses for CH4 IPC selection decisions.
* false - disable repeated buffer address tracking.

Used in functions: MPIDI_IPCI_is_repeat_addr

**MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD** - CH4

Sets the minimum send message size, in bytes, for selecting the XPMEM-based single-copy protocol for intranode communication. It is used only when MPICH is built with the XPMEM shared-memory submodule and `MPIR_CVAR_CH4_XPMEM_ENABLE` is enabled; selection also requires the message size not to exceed `MPIR_CVAR_CH4_IPC_XPMEM_P2P_UPPER_THRESHOLD` when that upper threshold is active, and requires repeated-address detection for the buffer. When active, this CVAR sets the point-to-point size threshold for eligible XPMEM-based IPC transfers.

* 65536 - default, select XPMEM-based single-copy IPC transfers for eligible messages of at least 65536 bytes.
* integer - select XPMEM-based single-copy IPC transfers for eligible messages whose size is greater than or equal to the specified byte threshold.

Used in functions: MPIDI_XPMEM_get_ipc_attr

**MPIR_CVAR_CH4_IPC_XPMEM_P2P_UPPER_THRESHOLD** - CH4

Sets the optional maximum send message size for selecting the XPMEM-based single-copy protocol for intranode communication. It is used only when MPICH is built with the XPMEM shared-memory submodule and `MPIR_CVAR_CH4_XPMEM_ENABLE` is enabled; selection also requires the message size to meet `MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD` and requires repeated-address detection for the buffer. When active, this CVAR sets the upper point-to-point size cutoff for eligible XPMEM-based IPC transfers.

* -1 - default, do not apply an upper size limit to XPMEM-based single-copy IPC selection.
* 0 or negative integer - do not apply an upper size limit to XPMEM-based single-copy IPC selection.
* positive integer - skip XPMEM-based single-copy IPC transfers for otherwise eligible messages larger than the specified byte limit.

Used in functions: MPIDI_XPMEM_get_ipc_attr

**MPIR_CVAR_CH4_IPC_ZE_SHAREABLE_HANDLE** - CH4

Selects the implementation used for Level Zero shareable IPC handles. It is used during IPC FD communicator bootstrap only when the GPU IPC handle type is shareable FD; the reviewed FD setup path is for Level Zero and also requires multiple local ranks and a communicator that has not already initialized FD sharing. When active, this CVAR selects how shareable IPC handles are obtained for the CH4 GPU IPC path.

* drmfd - default, use device FD-based shareable IPC handles and initialize local FD sharing for eligible shareable-FD GPU IPC use.
* pidfd - use the `pidfd_getfd` syscall to implement shareable IPC handles.

Used in functions: MPIDI_FD_comm_bootstrap

**MPIR_CVAR_CH4_MAX_NUM_PACK_BUFFERS** - CH4

Sets the maximum number of buffers in the CH4 per-VCI pack-buffer pool used for packing and unpacking active-message data. It is used when the per-VCI pool is created, together with `MPIR_CVAR_CH4_PACK_BUFFER_SIZE` for the size of each buffer and `MPIR_CVAR_CH4_NUM_PACK_BUFFERS_PER_CHUNK` for chunk allocation. When active, this CVAR sets the pool-wide cap for CH4 active-message pack buffers.

* 0 - default, allow an unlimited number of pack buffers in the pool.
* positive integer - limit the pool to the specified number of pack buffers.

Used in functions: MPIDI_init_per_vci

**MPIR_CVAR_CH4_MT_MODEL** - CH4

Selects the CH4 multi-threading model during device initialization when CH4 is built with runtime multi-threading model selection. It is parsed by `set_runtime_configurations`, which is called from `MPID_Init`; an invalid non-empty value causes initialization to fail. If runtime multi-threading model selection is not enabled at configure time, a non-empty setting is ignored and CH4 prints a warning. When active, this CVAR sets the CH4 runtime multi-threading model used by CH4 initialization and related thread-safety setup.

* "" - default, use the direct CH4 multi-threading model.
* direct - use the direct CH4 multi-threading model.
* lockless - use the lockless CH4 multi-threading model and create the request-pool mutexes used by that model during `MPID_Init`.

Used in functions: parse_mt_model, set_runtime_configurations, MPID_Init

**MPIR_CVAR_CH4_NETMOD** - CH4

Selects the CH4 network module during device initialization. It is used by `choose_netmod`, which is called from `MPID_Init` before the network module's local initialization hook runs. If this CVAR is not set, CH4 uses the first registered network module as the default; otherwise the configured string is matched case-insensitively against the registered CH4 network module names, and initialization reports an invalid netmod error when no registered name matches. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_init.c`. When active, this CVAR sets which CH4 network-module function tables are used for network communication.

* "" - default, use the first registered CH4 network module.
* registered CH4 network module name - use the matching CH4 network module.

Used in functions: choose_netmod, MPID_Init

**MPIR_CVAR_CH4_NUM_PACK_BUFFERS_PER_CHUNK** - CH4

Sets how many buffers are allocated in each chunk of the CH4 per-VCI pack-buffer pool used for packing and unpacking active-message data. It is used when the per-VCI pool is created, together with `MPIR_CVAR_CH4_PACK_BUFFER_SIZE` for the size of each buffer and `MPIR_CVAR_CH4_MAX_NUM_PACK_BUFFERS` for the pool-wide maximum. When active, this CVAR sets the chunk allocation size for the CH4 active-message pack-buffer pool.

* 64 - default, allocate 64 buffers in each pack-buffer pool chunk.
* positive integer - allocate the specified number of buffers in each pack-buffer pool chunk.

Used in functions: MPIDI_init_per_vci

**MPIR_CVAR_CH4_NUM_VCIS** - CH4

Sets how many CH4 VCIs are implicitly used. It is checked when CH4 VCI state is initialized and is applied to `MPI_COMM_WORLD` when communicator VCI setup runs. The requested implicit VCI count is paired with `MPIR_CVAR_CH4_RESERVE_VCIS`, should fit within `MPIDI_CH4_MAX_VCIS`, and the netmod may create fewer VCIs than requested. In CH4 OFI, it is also used with the reserved VCI count to size OFI transmit and receive contexts when scalable endpoints are enabled, capped by the provider context limits. When active, this CVAR sets the size of the implicit VCI range used for CH4 VCI selection.

* 1 - default, use one implicit VCI.
* positive integer greater than 1 - request multiple implicit VCIs, subject to the CH4 maximum VCI configuration, what the netmod creates, and provider context limits when OFI scalable endpoints are enabled.

Used in functions: MPIDI_vci_init, MPID_Comm_commit_post_hook, MPIDI_Comm_set_vcis, MPIDI_OFI_init_local

**MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE** - CH4_OFI

Controls whether CH4 OFI long active-message sends use the pipeline path instead of the default RDMA-read path. It is used when an active-message send is larger than the eager limit, or when that long-message path is preselected during active-message eager checking. RDMA-read remains the default long-message path only when OFI RMA support is enabled and this CVAR is disabled; otherwise the pipeline path is selected. When active, this CVAR forces long OFI active-message sends to use pipelined transfer.

* false - default, use RDMA-read for long active-message sends when OFI RMA support is enabled; otherwise use pipeline.
* true - force long active-message sends to use pipeline instead of RDMA-read.

Used in functions: MPIDI_NM_am_isend, MPIDI_NM_am_check_eager

**MPIR_CVAR_CH4_OFI_COLL_SELECTION_TUNING_JSON_FILE** - COLLECTIVE

Specifies a tuning JSON file for CH4 OFI collective algorithm selection. It is used by the OFI netmod Bcast entry point only when `MPIR_CVAR_BCAST_OFI_INTRA_ALGORITHM` selects automatic internal algorithm selection. When active, this CVAR sets the tuning-file input that can override the internally selected CH4 OFI Bcast algorithm.

* NULL - default, do not use a CH4 OFI collective-selection tuning JSON file.
* JSON file path - use the specified tuning JSON file to guide CH4 OFI collective algorithm selection.

Used in functions: MPIDI_NM_mpi_bcast

**MPIR_CVAR_CH4_OFI_CONTEXT_ID_BITS** - CH4_OFI

Sets the number of OFI tag-match bits CH4 OFI allocates to MPI context IDs. It is used while OFI runtime settings are initialized, global OFI match-bit limits are checked, and debug settings are printed. It can override the provider capability-set default, but the effective match-bit layout also depends on the source-rank and user-tag bit settings and must fit with the OFI protocol bits in a 64-bit match value. If OFI completion data is disabled and no source-rank match bits are available, CH4 OFI replaces the context, source-rank, and user-tag bit layout with fallback values that include source-rank bits. When active, this CVAR sets the context-ID portion of the OFI tag-matching bit layout.

* -1 - default, use the provider capability-set value.
* nonnegative integer - use this as the number of OFI tag-match bits allocated to MPI context IDs.

Used in functions: MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, update_global_limits, dump_global_settings

**MPIR_CVAR_CH4_OFI_DISABLE_INJECT_WRITE** - CH4_OFI

Controls whether CH4 OFI avoids the `fi_inject_write` path for small contiguous RMA put operations. It is used only after the native OFI RMA put path is selected, the target memory region is available, the origin and target datatypes are contiguous, the origin buffer is not GPU device memory, and the operation size is within the OFI buffered-write limit. No alternate CVAR name is visible in `src/mpid/ch4/netmod/ofi/ofi_rma.h`. When active, this CVAR disables use of libfabric inject write so the operation proceeds through another RMA put path instead.

* false - default, allow eligible small contiguous RMA put operations to use `fi_inject_write`.
* true - avoid `fi_inject_write` for eligible small contiguous RMA put operations.

Used in functions: MPIDI_OFI_do_put

**MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD** - CH4_OFI

Sets the message-size threshold used by CH4 OFI point-to-point tagged sends to choose between direct eager sending and MPICH-level rendezvous. It is applied only for non-active-message sends after the inject path is not selected; active-message sends and injected sends do not use this threshold. The effective eager threshold uses the OFI provider maximum message size when this CVAR is left at its automatic setting. When active, this CVAR sets the cutoff above which CH4 OFI starts a rendezvous handshake before sending the data.

* -1 - default, use the OFI provider maximum message size as the eager threshold.
* integer - use the specified byte threshold for choosing between eager tagged send and MPICH-level rendezvous.

Used in functions: MPIDI_OFI_send

**MPIR_CVAR_CH4_OFI_ENABLE** - CH4_OFI

`MPIR_CVAR_CH4_OFI_ENABLE` is not used as an alternate name for another CVAR in `src/mpid/ch4/netmod/ofi/init_settings.c`. This file uses `MPIR_CVAR_CH4_OFI_ENABLE_*` CVARs as separate per-capability overrides while initializing OFI runtime settings, building provider hints, matching providers, and applying selected-provider capabilities. `MPIR_CVAR_CH4_OFI_ENABLE` itself does not enable or set OFI runtime behavior in this file.

* not used - this CVAR is not read in `src/mpid/ch4/netmod/ofi/init_settings.c`; no valid values are represented there.

Used in functions: none in `src/mpid/ch4/netmod/ofi/init_settings.c`

**MPIR_CVAR_CH4_OFI_ENABLE_AM** - CH4_OFI

Controls whether CH4 OFI uses libfabric active-message support. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, debug settings are printed, per-VCI active-message resources are initialized, active-message receive buffers are posted, active-message buffers are reposted, and active-message resources are finalized. It can override provider capability-set defaults, but the final setting still depends on whether the selected provider supports both message queues and shared receive buffers. When active, this CVAR enables requesting and using OFI message queues with shared receive buffers for CH4 OFI active-message communication.

* -1 - default, use the provider capability-set default and keep active-message support only when the selected provider supports both `FI_MSG` and `FI_MULTI_RECV`.
* 0 - disable OFI active-message support.
* nonzero - request OFI active-message support and require provider support for both `FI_MSG` and `FI_MULTI_RECV`.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, MPIDI_OFI_mpi_finalize_hook, dump_global_settings, am_init, am_post_recv, MPIDI_OFI_am_repost_buffer

**MPIR_CVAR_CH4_OFI_ENABLE_ATOMICS** - CH4_OFI

Controls whether CH4 OFI requests and uses libfabric atomic operation support. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, scalable endpoint transmit and receive contexts are created, and debug settings are printed. It can override provider capability-set defaults, but atomics are only tried when OFI RMA support is enabled; the final setting also depends on whether the selected provider supports `FI_ATOMICS` with the required atomic message ordering. OFI triggered operations also require atomics and tagged-message support to be enabled. When active, this CVAR enables requesting OFI atomics, delivery-complete transmit operation flags for atomic-capable paths, atomic ordering in provider hints, and atomic capabilities on scalable endpoint transmit and receive contexts.

* -1 - default, use the provider capability-set default, try atomics only when OFI RMA support is enabled, and keep atomics only when the selected provider supports `FI_ATOMICS` with the required atomic message ordering.
* 0 - disable OFI atomic operation support.
* nonzero - request OFI atomic operation support, effective only when OFI RMA support is enabled and provider support for `FI_ATOMICS` with the required atomic message ordering is available.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, create_sep_tx, create_sep_rx, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_AV_TABLE** - CH4_OFI

Controls whether CH4 OFI requests and opens libfabric address vectors as tables instead of maps. It is used while OFI runtime settings are initialized, provider hints are built, selected-provider capabilities are applied, local or shared address vectors are opened, and debug settings are printed. It can override provider capability-set defaults, but the final setting still depends on the selected provider returning table address-vector support. When active, this CVAR sets the OFI address-vector type used in hints and in address-vector open calls to `FI_AV_TABLE` rather than `FI_AV_MAP`.

* -1 - default, use the provider capability-set default and keep table address-vector support only when the selected provider returns `FI_AV_TABLE`.
* 0 - use OFI map address vectors.
* nonzero - request OFI table address vectors and keep that setting only when the selected provider returns `FI_AV_TABLE`.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, try_open_shared_av, open_local_av, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_CONTROL_AUTO_PROGRESS** - CH4_OFI

Controls whether CH4 OFI requests and records libfabric automatic control progress. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, and debug settings are printed. It can override provider capability-set defaults, but the final setting still depends on the selected provider reporting automatic control-progress support. When active, this CVAR sets the OFI control-progress mode requested in provider hints to automatic progress.

* -1 - default, use the provider capability-set default and keep automatic control progress only when the selected provider supports it.
* 0 - request manual OFI control progress.
* nonzero - request automatic OFI control progress and require provider support for `FI_PROGRESS_AUTO` control progress.

Used in functions: MPIDI_OFI_set_auto_progress, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_DATA** - CH4_OFI

Controls whether CH4 OFI uses libfabric immediate completion data to carry source-rank information outside the tag-match bits. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, finalize-time flush sends are issued, and debug settings are printed. It can override provider capability-set defaults, but the final setting still depends on the selected provider supporting directed receive and enough completion-data space; when completion data is not enabled and source-rank match bits are unavailable, CH4 OFI falls back to a match-bit layout with source-rank bits. When active, this CVAR enables requesting and using OFI directed receive with completion data for source-rank transmission.

* -1 - default, use the provider capability-set default and keep completion-data support only when the selected provider supports directed receive and sufficient completion-data space.
* 0 - disable OFI completion-data source-rank transmission.
* nonzero - request OFI completion-data source-rank transmission and require provider support for directed receive and sufficient completion-data space.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, flush_send, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_DATA_AUTO_PROGRESS** - CH4_OFI

Controls whether CH4 OFI requests and records libfabric automatic data progress. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, triggered-operation settings are finalized, and debug settings are printed. It can override provider capability-set defaults, but the final setting still depends on the selected provider reporting automatic data-progress support; OFI triggered operations force this setting on after provider capabilities are applied. When active, this CVAR sets the OFI data-progress mode requested in provider hints to automatic progress.

* -1 - default, use the provider capability-set default and keep automatic data progress only when the selected provider supports it.
* 0 - request manual OFI data progress.
* nonzero - request automatic OFI data progress and require provider support for `FI_PROGRESS_AUTO` data progress.

Used in functions: MPIDI_OFI_set_auto_progress, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_HMEM** - CH4_OFI

Controls whether CH4 OFI requests and uses libfabric heterogeneous-memory support for GPU memory transfers. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, configure-time provider settings are updated, memory-registration requirements for GPU direct RDMA are detected, debug settings are printed, and GPU direct RDMA memory registrations are cleaned up. It relies on provider support for `FI_HMEM` when OFI runtime checks are enabled, requests `FI_MR_HMEM` when memory registration mode support is available, and requests `FI_HMEM` in provider hints only when GPU support is enabled through `MPIR_CVAR_ENABLE_GPU`. When active, this CVAR enables CH4 OFI support for direct provider transfers involving GPU memory and associated GPU direct RDMA memory-registration handling.

* -1 - use the provider capability-set default and keep HMEM support only when the selected provider supports it.
* 0 - default, disable CH4 OFI HMEM support.
* nonzero - request CH4 OFI HMEM support and require provider support for `FI_HMEM` when OFI runtime checks are enabled.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, find_provider, MPIDI_OFI_mpi_finalize_hook, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_INJECT** - DEVELOPER

Controls whether CH4 OFI uses libfabric inject operations for eligible small point-to-point tagged sends. It is used in the CH4 OFI send path when tagged messaging is active and before eager/rendezvous selection. The inject path is considered only for non-synchronous, non-initialized sends when deciding whether noncontiguous datatypes may avoid packing, and it is used only for non-synchronous, non-active-message sends whose data size is no larger than `MPIDI_OFI_global.max_buffered_send` and that do not require memory registration. GPU sends may first force host packing when HMEM support, strict device-pointer requirements, or MR HMEM threshold rules require it; noncontiguous sends cannot use no-pack IOV when inject is selected. When active, this CVAR enables CH4 OFI to buffer eligible small sends with `fi_tinject` or `fi_tinjectdata` and complete the MPI send request immediately.

* true - default, allow eligible small point-to-point tagged sends to use the OFI inject path.
* false - disable the OFI inject path so eligible small sends continue through the normal or rendezvous send-path selection.

Used in functions: MPIDI_OFI_send

**MPIR_CVAR_CH4_OFI_ENABLE_MR_ALLOCATED** - CH4_OFI

Controls whether CH4 OFI requests and records libfabric allocated-memory memory-registration mode. It is used while OFI runtime settings are initialized, provider hints are built, selected-provider memory-registration mode is applied, and debug settings are printed. It can override provider capability-set defaults, but the final runtime setting follows the selected provider's memory-registration mode. For OFI 1.5 and later, it requests allocated-memory memory-registration mode when available; for older required OFI versions, it is treated together with `MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS` and `MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY` as the basic memory-registration mode. When active, this CVAR enables CH4 OFI to require OFI memory regions to be backed by allocated physical memory at registration time.

* -1 - default, use the provider capability-set default and keep allocated-memory memory-registration mode only when the selected provider reports it.
* 0 - disable allocated-memory memory-registration mode; for older required OFI versions, use the scalable memory-registration mode together with disabled virtual-address and provider-key modes.
* nonzero - request allocated-memory memory-registration mode; for older required OFI versions, request the basic memory-registration mode together with virtual-address and provider-key modes.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_MR_HMEM** - CH4_OFI

Controls whether CH4 OFI treats heterogeneous-memory buffers as requiring libfabric memory registration for GPU direct RDMA. It is used while OFI runtime settings are initialized, while selected-provider capabilities are applied, while provider-reported `FI_MR_HMEM` support is detected, and during finalize cleanup of GPU direct RDMA memory registrations. It relies on `MPIR_CVAR_CH4_OFI_ENABLE_HMEM` being enabled before provider-reported `FI_MR_HMEM` support is used to choose the runtime setting, and the registration cleanup path is active only when both HMEM support and MR HMEM support are enabled. When active, this CVAR enables CH4 OFI GPU direct RDMA paths that register heterogeneous-memory buffers with the provider.

* -1 - default, use the provider capability-set default, or use the selected provider's `FI_MR_HMEM` support when HMEM support is enabled.
* 0 - disable CH4 OFI MR HMEM support.
* nonzero - enable CH4 OFI MR HMEM support.

Used in functions: MPIDI_OFI_init_settings, find_provider, MPIDI_OFI_mpi_finalize_hook

**MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY** - CH4_OFI

Controls whether CH4 OFI requests and records libfabric provider-key memory-registration mode. It is used while OFI runtime settings are initialized, provider hints are built, selected-provider memory-registration mode is applied, and debug settings are printed. It can override provider capability-set defaults, but the final runtime setting follows the selected provider's memory-registration mode. For OFI 1.5 and later, it requests provider-supplied memory-region keys when available; for older required OFI versions, it is treated together with `MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS` and `MPIR_CVAR_CH4_OFI_ENABLE_MR_ALLOCATED` as the basic memory-registration mode. When active, this CVAR enables CH4 OFI to use provider-supplied keys for OFI memory regions.

* -1 - default, use the provider capability-set default and keep provider-key memory-registration mode only when the selected provider reports it.
* 0 - disable provider-key memory-registration mode; for older required OFI versions, use the scalable memory-registration mode together with disabled virtual-address and allocated memory-region modes.
* nonzero - request provider-key memory-registration mode; for older required OFI versions, request the basic memory-registration mode together with virtual-address and allocated memory-region modes.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_MR_REGISTER_NULL** - CH4_OFI

Controls whether CH4 OFI records support for libfabric memory registration calls with NULL addresses. It is used while OFI runtime settings are initialized and debug settings are printed. It can override provider capability-set defaults; no dependency on another CVAR or mode is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/init_settings.c`. When active, this CVAR enables the CH4 OFI capability setting that indicates memory registration supports NULL addresses.

* -1 - default, use the provider capability-set default for NULL-address memory registration support.
* 0 - disable the CH4 OFI NULL-address memory registration support setting.
* nonzero - enable the CH4 OFI NULL-address memory registration support setting.

Used in functions: MPIDI_OFI_init_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_MR_VIRT_ADDRESS** - CH4_OFI

Controls whether CH4 OFI requests and records libfabric virtual-address memory-registration mode. It is used while OFI runtime settings are initialized, provider hints are built, selected-provider memory-registration mode is applied, and debug settings are printed. It can override provider capability-set defaults, but the final runtime setting follows the selected provider's memory-registration mode. For OFI 1.5 and later, it requests `FI_MR_VIRT_ADDR` when available; for older required OFI versions, it is treated together with `MPIR_CVAR_CH4_OFI_ENABLE_MR_PROV_KEY` and `MPIR_CVAR_CH4_OFI_ENABLE_MR_ALLOCATED` as the `FI_MR_BASIC` memory-registration mode. When active, this CVAR enables CH4 OFI to use virtual addresses in OFI memory-region registration mode.

* -1 - default, use the provider capability-set default and keep virtual-address memory-registration mode only when the selected provider reports it.
* 0 - disable virtual-address memory-registration mode; for older required OFI versions, use the scalable memory-registration mode.
* nonzero - request virtual-address memory-registration mode; for older required OFI versions, request the basic memory-registration mode together with provider keys and allocated memory regions.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_HASHING** - CH4

Controls whether CH4 OFI assigns point-to-point traffic across multiple NICs by hashing. It is used when communicators are committed and when communicator hints are updated. Multi-NIC hashing can be enabled only when more than one NIC is available, no preferred NIC hint is set, and both no-any-tag and no-any-source communicator hints are active; the communicator hint `MPIR_COMM_HINT_ENABLE_MULTI_NIC_HASHING` overrides this CVAR when present. When active, this CVAR enables CH4 OFI communicator hashing to select NICs for sending and receiving messages.

* 0 - default, disable multi-NIC hashing.
* positive integer - enable multi-NIC hashing when the communicator and NIC conditions allow it.
* -1 - let MPICH automatically determine whether to use multi-NIC hashing.

Used in functions: update_multi_nic_hints, MPIDI_OFI_mpi_comm_commit_pre_hook

**MPIR_CVAR_CH4_OFI_ENABLE_PT2PT_NOPACK** - CH4_OFI

Controls whether CH4 OFI records the point-to-point no-pack capability setting. It is used while OFI runtime settings are initialized and debug settings are printed. It can override the provider capability-set default; no dependency on another CVAR or mode is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/init_settings.c`. When active, this CVAR enables the CH4 OFI capability setting for point-to-point transfers without packing.

* -1 - default, use the provider capability-set default for point-to-point no-pack support.
* 0 - disable the CH4 OFI point-to-point no-pack capability setting.
* nonzero - enable the CH4 OFI point-to-point no-pack capability setting.

Used in functions: MPIDI_OFI_init_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_RMA** - CH4_OFI

Controls whether CH4 OFI enables native OFI RMA support for MPI RMA operations. It is used while OFI runtime settings are initialized, candidate providers are scored, selected-provider capabilities are applied, provider hints are built, scalable endpoint transmit and receive contexts are created, and debug settings are printed. It can override provider capability-set defaults, but the final setting still depends on whether the selected provider supports `FI_RMA`; OFI atomics are only tried when RMA is enabled. CH4 OFI still requires basic OFI RMA capability in provider hints for active-message large transfers and native modes. When active, this CVAR enables CH4 OFI to request delivery-complete operation flags for direct RMA paths and to add OFI RMA capabilities to scalable endpoint transmit and receive contexts.

* -1 - default, use the provider capability-set default and keep native RMA support only when the selected provider supports `FI_RMA`.
* 0 - disable native CH4 OFI RMA support for MPI RMA operations, while still requiring basic OFI RMA capability for active-message large transfers and native modes.
* nonzero - request native CH4 OFI RMA support for MPI RMA operations and require provider support for `FI_RMA`.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, create_sep_tx, create_sep_rx, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_SCALABLE_ENDPOINTS** - CH4_OFI

Controls whether CH4 OFI requests and uses libfabric scalable endpoints. It is used while OFI runtime settings are initialized, provider hints are built, selected-provider capabilities are applied, VCI endpoint resources are sized, VCI contexts are created and destroyed, RMA windows choose their per-window endpoint path, and debug settings are printed. It can override provider capability-set defaults, but the final runtime setting still depends on the selected provider supporting multiple transmit contexts and named receive contexts. For RMA windows, the per-window scalable-endpoint path also requires a positive `MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX`; otherwise CH4 OFI falls back to shared transmit contexts or the global endpoint path. When active, this CVAR enables CH4 OFI to create scalable endpoints with separate transmit and receive contexts for VCI communication and, when the RMA scalable-endpoint context limit permits it, per-window RMA transmit contexts.

* -1 - default, use the provider capability-set default and keep scalable endpoint support only when the selected provider supports multiple transmit contexts and named receive contexts.
* 0 - disable CH4 OFI scalable endpoints.
* nonzero - request CH4 OFI scalable endpoints and keep that setting only when the selected provider supports multiple transmit contexts and named receive contexts.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, MPIDI_OFI_init_local, MPIDI_OFI_create_vci_context, destroy_vci_context, win_init, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_SHARED_AV** - CH4_OFI

Controls whether CH4 OFI tries to open a pre-existing named shared libfabric address vector during VCI domain creation. It is used only for NIC 0; if the shared address vector is not requested, the NIC is not NIC 0, or opening the named address vector fails, CH4 OFI opens a local address vector instead. The shared-address-vector open uses the address-vector type selected by `MPIDI_OFI_ENABLE_AV_TABLE`, so the table-or-map mode affects the shared AV attributes. When active and the named address vector opens successfully, this CVAR enables CH4 OFI to populate root OFI AV addresses from the shared mapped table and mark that a named AV was obtained.

* false - default, do not try to open a shared address vector; open a local address vector.
* true - try to open a named shared address vector for NIC 0 and fall back to a local address vector if it cannot be opened.

Used in functions: create_vci_domain

**MPIR_CVAR_CH4_OFI_ENABLE_SHARED_CONTEXTS** - CH4_OFI

Controls whether CH4 OFI requests shared transmit contexts for OFI RMA. It is used while OFI runtime settings are initialized and debug settings are printed. It can override provider capability-set defaults; when left to the provider capability set and the provider supports shared contexts, CH4 OFI tries to use them and falls back later if they are unavailable. When active, this CVAR enables CH4 OFI to try OFI shared contexts for RMA communication.

* -1 - use the provider capability-set default; if the provider supports shared contexts, try to use them and fall back later if unavailable.
* 0 - default, disable CH4 OFI shared contexts.
* nonzero - request CH4 OFI shared contexts and fall back later if unavailable.

Used in functions: MPIDI_OFI_init_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_ENABLE_TAGGED** - CH4_OFI

Controls whether CH4 OFI uses libfabric tagged-message support. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, and scalable endpoint transmit and receive contexts are created. It can override provider capability-set defaults, but the final setting still depends on whether the selected provider supports `FI_TAGGED`; OFI triggered operations also require tagged support to be enabled. When active, this CVAR enables requesting and using the OFI tagged interface for tag-matched communication.

* -1 - default, use the provider capability-set default and keep tagged support only when the selected provider supports it.
* 0 - disable OFI tagged-message support.
* nonzero - request OFI tagged-message support and require provider support for `FI_TAGGED`.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, create_sep_tx, create_sep_rx

**MPIR_CVAR_CH4_OFI_ENABLE_TRIGGERED** - CH4_OFI

Controls whether CH4 OFI requests and uses libfabric triggered operations for MPI collectives. It is used while OFI runtime settings are initialized, provider hints are built, candidate providers are scored, selected-provider capabilities are applied, and debug settings are printed. It is only copied into OFI runtime settings for the sockets provider; when enabled, it requires OFI atomics and tagged-message support to be enabled, requires the selected sockets provider to report the needed triggered-operation, RMA-event, directed-receive, send, receive, read, write, remote-read, remote-write, atomics, and RMA capabilities, and forces OFI data auto-progress on after provider capabilities are applied. When active, this CVAR enables CH4 OFI to request OFI triggered operations, RMA event memory-registration mode, and automatic data progress for collective-operation support.

* -1 - default, use the provider capability-set default for the sockets provider and keep triggered operations only when the selected sockets provider reports the required support; for non-sockets providers, triggered operations remain disabled.
* 0 - disable CH4 OFI triggered operations.
* nonzero - request CH4 OFI triggered operations for the sockets provider, effective only when atomics and tagged-message support are enabled and the selected sockets provider reports the required support.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_match_provider, MPIDI_OFI_update_global_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_FETCH_ATOMIC_IOVECS** - CH4_OFI

Specifies the maximum number of iovecs CH4 OFI records for libfabric fetch-atomic operations. It is used while OFI runtime settings are initialized and debug settings are printed. It can override provider capability-set defaults, but when left unset CH4 OFI uses the selected provider capability-set value. No dependency on another CVAR or mode is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/init_settings.c`. When active, this CVAR sets the CH4 OFI capability value that limits fetch-atomic iovec use.

* -1 - default, use the provider capability-set value.
* nonnegative integer - use this as the maximum number of iovecs for fetch-atomic operations.

Used in functions: MPIDI_OFI_init_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_GPU_RDMA_THRESHOLD** - CH4_OFI

Sets the message-size threshold for using GPU direct RDMA with registered GPU memory in CH4 OFI point-to-point transfers. It is used on GPU send and receive paths only when OFI HMEM support and MR HMEM support are enabled; sends also require a strict GPU device buffer and contiguous datatype before the threshold can select the direct RDMA path, and receives additionally require a contiguous datatype and strict GPU device receive buffer before registering memory. When active, this CVAR sets the cutoff at which CH4 OFI stops staging GPU data through host packing for MR HMEM-capable GPU direct RDMA paths.

* 0 - default, allow GPU direct RDMA registration for all eligible GPU messages.
* positive integer - use the specified byte threshold; eligible messages at or above the threshold use GPU direct RDMA registration, while smaller eligible messages use host staging.
* negative integer - prevent the threshold comparison from selecting GPU direct RDMA registration, so otherwise eligible GPU messages use host staging.

Used in functions: MPIDI_OFI_send, MPIDI_OFI_do_irecv

**MPIR_CVAR_CH4_OFI_GPU_RECEIVE_ENGINE_TYPE** - CH4_OFI

Sets the GPU engine type used for CH4 OFI GPU point-to-point transfers on the receiver side. It is used by the OFI GPU receive-engine selector to translate the CVAR setting into an MPL GPU engine type. No alternate CVAR name, dependency on another CVAR, or additional active mode requirement is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/ofi_impl.h`; the setting is meaningful for GPU point-to-point receiver-side paths. When active, this CVAR selects which GPU transfer engine CH4 OFI uses on the receiver side.

* copy_low_latency - default, use a low-latency copy engine.
* compute - use a compute engine.
* copy_high_bandwidth - use a high-bandwidth copy engine.
* yaksa - use Yaksa.

Used in functions: MPIDI_OFI_gpu_get_recv_engine_type

**MPIR_CVAR_CH4_OFI_GPU_SEND_ENGINE_TYPE** - CH4_OFI

Sets the GPU engine type used for CH4 OFI GPU point-to-point transfers on the sender side. It is used by the OFI GPU send-engine selector to translate the CVAR setting into an MPL GPU engine type. No alternate CVAR name, dependency on another CVAR, or additional active mode requirement is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/ofi_impl.h`; the setting is meaningful for GPU point-to-point sender-side paths. When active, this CVAR selects which GPU transfer engine CH4 OFI uses on the sender side.

* copy_low_latency - default, use a low-latency copy engine.
* compute - use a compute engine.
* copy_high_bandwidth - use a high-bandwidth copy engine.
* yaksa - use Yaksa.

Used in functions: MPIDI_OFI_gpu_get_send_engine_type

**MPIR_CVAR_CH4_OFI_MAJOR_VERSION** - CH4_OFI

Sets the major component of the required OFI/libfabric version used by CH4 OFI. It is read while finding an OFI provider when runtime OFI settings are compiled in, then the resulting required version is used for provider discovery, provider-specific settings initialization, and provider hint construction. It also seeds the runtime OFI capability settings, where it can override the provider capability-set default. This CVAR relies on `MPIR_CVAR_CH4_OFI_MINOR_VERSION` also being set before it changes the required version passed to libfabric; otherwise CH4 OFI uses the current libfabric version. When active, this CVAR sets the OFI/libfabric major version used to form the required minimum version for `fi_getinfo`.

* -1 - default, do not override the provider capability-set major version and do not change the required libfabric version unless a capability set supplies both version components.
* nonnegative integer - use this as the required OFI/libfabric major version when the minor version is also set.

Used in functions: find_provider, MPIDI_OFI_get_required_version, MPIDI_OFI_init_hints, MPIDI_OFI_init_settings

**MPIR_CVAR_CH4_OFI_MAX_EAGAIN_RETRY** - CH4_OFI

Sets the retry limit used by CH4 OFI retry wrappers when a libfabric operation returns `FI_EAGAIN`. It is used by the OFI retry macros before retry progress is made and the operation is attempted again; in the reviewed OFI initialization file those retry macros are used for finalize-time flush send and receive operations. When active, this CVAR sets the number of `FI_EAGAIN` retries allowed before CH4 OFI returns `MPIX_ERR_EAGAIN` instead of continuing to retry.

* -1 - default, continue retrying `FI_EAGAIN` operations without using this CVAR as a retry limit.
* 0 - continue retrying `FI_EAGAIN` operations without using this CVAR as a retry limit.
* positive integer - allow the specified number of `FI_EAGAIN` retry attempts before returning `MPIX_ERR_EAGAIN`.

Used in functions: MPIDI_OFI_CALL_RETRY, MPIDI_OFI_CALL_RETRY_RETURN, flush_send, flush_recv

**MPIR_CVAR_CH4_OFI_MAX_NICS** - CH4

Sets the maximum number of physical NICs CH4 OFI uses after libfabric providers have been discovered and usable NICs have been counted. It is used only when libfabric NIC information is available and at least one PCI NIC is detected; otherwise CH4 OFI falls back to a single selected provider. The selected NIC count is capped by the number of detected NICs and by the compile-time `MPIDI_OFI_MAX_NICS` limit, while NIC ordering can also be affected by `MPIR_CVAR_CH4_OFI_PREF_NIC`, process locality, and topology-specific NIC assignment. When active, this CVAR sets how many detected physical NICs CH4 OFI keeps available for communication.

* 1 - default, use one physical NIC.
* -1 - use all detected physical NICs up to the compile-time limit.
* positive integer greater than 1 - use up to the specified number of physical NICs, capped by the number detected.
* 0 or negative integer other than -1 - use one physical NIC.

Used in functions: MPIDI_OFI_init_multi_nic, setup_multi_nic

**MPIR_CVAR_CH4_OFI_MAX_RMA_SEP_CTX** - CH4_OFI

Sets the maximum number of transmit contexts CH4 OFI may allocate for per-window RMA scalable endpoints. It is used when an OFI RMA window is initialized, after native OFI RMA is enabled, and only when scalable endpoints are enabled and this CVAR is positive; otherwise CH4 OFI skips the per-window RMA scalable-endpoint path and falls back to shared transmit contexts or the global endpoint path. The effective number of RMA scalable-endpoint transmit contexts is also capped by the selected provider's maximum endpoint transmit-context count. When active, this CVAR enables CH4 OFI to create a separate scalable endpoint for RMA windows and to hand out one transmit context from that endpoint to each window until the configured/provider-limited pool is exhausted.

* 0 - default, disable the per-window RMA scalable-endpoint path.
* positive integer - enable the per-window RMA scalable-endpoint path and use up to this many RMA scalable-endpoint transmit contexts, capped by the provider limit.
* negative integer - disable the per-window RMA scalable-endpoint path.

Used in functions: win_init, win_init_sep

**MPIR_CVAR_CH4_OFI_MINOR_VERSION** - CH4_OFI

Sets the minor component of the required OFI/libfabric version used by CH4 OFI. It is read while finding an OFI provider when runtime OFI settings are compiled in, then the resulting required version is used for provider discovery, provider-specific settings initialization, and provider hint construction. It also seeds the runtime OFI capability settings, where it can override the provider capability-set default. This CVAR relies on `MPIR_CVAR_CH4_OFI_MAJOR_VERSION` also being set before it changes the required version passed to libfabric; otherwise CH4 OFI uses the current libfabric version. When active, this CVAR sets the OFI/libfabric minor version used to form the required minimum version for `fi_getinfo`.

* -1 - default, do not override the provider capability-set minor version and do not change the required libfabric version unless a capability set supplies both version components.
* nonnegative integer - use this as the required OFI/libfabric minor version when the major version is also set.

Used in functions: find_provider, MPIDI_OFI_get_required_version, MPIDI_OFI_init_hints, MPIDI_OFI_init_settings

**MPIR_CVAR_CH4_OFI_MULTIRECV_BUFFER_SIZE** - CH4

Sets the size of each CH4 OFI active-message multi-receive buffer. It is used when active-message receive buffers are preposted; OFI active messages must be enabled for those buffers to be allocated and posted. The internal active-message buffer-size macro maps directly to this CVAR, and no alternate CVAR name or dependency on another CVAR is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/ofi_types.h`. When active, this CVAR sets the byte size of each buffer used for FI multi-receive active-message receives.

* 2097152 - default, use 2097152-byte active-message multi-receive buffers.
* integer - use the specified byte size for each active-message multi-receive buffer.

Used in functions: am_post_recv

**MPIR_CVAR_CH4_OFI_MULTI_NIC_STRIPING_THRESHOLD** - CH4

Sets the global message-size threshold used by CH4 OFI multi-NIC striping. It is initialized from this CVAR while OFI global limits are updated, and it is applied again when a communicator enables multi-NIC striping. Communicator striping is enabled only when more than one NIC is available and no preferred NIC hint is set; the communicator hint `MPIR_COMM_HINT_ENABLE_MULTI_NIC_STRIPING` overrides `MPIR_CVAR_CH4_OFI_ENABLE_MULTI_NIC_STRIPING` when present. When active, this CVAR sets the message-size cutoff above which CH4 OFI strips messages across multiple NICs.

* 1048576 - default, stripe messages larger than 1048576 bytes.
* integer - use the specified byte threshold for multi-NIC striping.

Used in functions: update_global_limits, update_multi_nic_hints

**MPIR_CVAR_CH4_OFI_NUM_AM_BUFFERS** - CH4_OFI

Sets the number of CH4 OFI active-message receive buffers allocated and initially posted for each VCI. It is used while OFI runtime settings are initialized, debug settings are printed, and per-VCI active-message receive buffers are allocated and posted. It can override the provider capability-set default; the initialized value is clamped to the supported active-message buffer range. The buffers are used only when OFI active-message support is enabled, which requires the selected provider to support both message queues and shared receive buffers. When active-message support is enabled, this CVAR sets how many shared-receive `FI_MULTI_RECV` buffers CH4 OFI uses for active-message receives.

* -1 - default, use the provider capability-set value before clamping to the supported range.
* negative integer - treated as zero active-message receive buffers.
* 0 - use zero active-message receive buffers.
* positive integer up to the supported maximum - use this many active-message receive buffers.
* positive integer above the supported maximum - use the supported maximum number of active-message receive buffers.

Used in functions: MPIDI_OFI_init_settings, dump_global_settings, am_post_recv

**MPIR_CVAR_CH4_OFI_NUM_OPTIMIZED_MEMORY_REGIONS** - CH4_OFI

Sets the number of CH4 OFI optimized memory-region keys available for low-overhead, unordered RMA operations. It is used while OFI runtime settings are initialized and debug settings are printed. It can override the provider capability-set default, and no dependency on another CVAR or mode is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/init_settings.c`. When active, this CVAR sets how many optimized memory regions the provider is considered to support for the optimized RMA memory-region path.

* -1 - use the provider capability-set default.
* 0 - default, no optimized memory regions are available.
* positive integer - use this many optimized memory regions.

Used in functions: MPIDI_OFI_init_settings, dump_global_settings

**MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ** - CH4_OFI

Sets the chunk size used by CH4 OFI pipeline data transfer and the per-VCI pipeline buffer pool. It is used when per-VCI OFI resources are initialized, when noncontiguous pack buffers can fit in the pipeline pool, and when OFI active-message pipeline sends and receives divide data into chunks. The pipeline transfer path is used for long active-message transfers when selected instead of RDMA-read, including when `MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE` forces that path or when OFI RMA support is not used for the long-message path. The pipeline buffer pool size also depends on `MPIR_CVAR_CH4_OFI_PIPELINE_NUM_CHUNKS` and `MPIR_CVAR_CH4_OFI_PIPELINE_MAX_CHUNKS`. When active, this CVAR sets the byte size of each pipeline chunk and of each cell in the pipeline buffer pool.

* 1048576 - default, use 1048576-byte pipeline chunks and pipeline buffer-pool cells.
* positive integer - use the specified byte size for pipeline chunks and pipeline buffer-pool cells.

Used in functions: MPIDI_OFI_alloc_pack_buf, MPIDI_OFI_init_per_vci, pipeline_send_poll, send_copy_complete, pipeline_recv_poll

**MPIR_CVAR_CH4_OFI_PIPELINE_MAX_CHUNKS** - CH4_OFI

Sets the maximum number of chunk buffers that each CH4 OFI per-VCI pipeline buffer pool may reserve. It is used when per-VCI OFI resources are initialized, together with `MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ` for each buffer size and `MPIR_CVAR_CH4_OFI_PIPELINE_NUM_CHUNKS` for the initial buffer count. The pool supports pipeline data transfer, including long active-message pipeline transfers selected instead of RDMA-read, and temporary pack buffers that fit in the pipeline pool. When active, this CVAR sets the growth limit for pipeline chunk buffers available per VCI.

* 1024 - default, allow each per-VCI pipeline buffer pool to reserve up to 1024 chunk buffers.
* positive integer - allow each per-VCI pipeline buffer pool to reserve up to the specified number of chunk buffers.

Used in functions: MPIDI_OFI_init_per_vci

**MPIR_CVAR_CH4_OFI_PIPELINE_NUM_CHUNKS** - CH4_OFI

Sets the number of initial chunk buffers created for each CH4 OFI per-VCI pipeline buffer pool. It is used when per-VCI OFI resources are initialized, together with `MPIR_CVAR_CH4_OFI_PIPELINE_CHUNK_SZ` for each buffer size and `MPIR_CVAR_CH4_OFI_PIPELINE_MAX_CHUNKS` for the pool growth limit. The pool supports pipeline data transfer, including long active-message pipeline transfers selected instead of RDMA-read, and temporary pack buffers that fit in the pipeline pool. When active, this CVAR sets the initial number of pipeline chunk buffers available per VCI.

* 32 - default, create 32 initial chunk buffers in each per-VCI pipeline buffer pool.
* positive integer - create the specified number of initial chunk buffers in each per-VCI pipeline buffer pool.

Used in functions: MPIDI_OFI_init_per_vci

**MPIR_CVAR_CH4_OFI_PREF_NIC** - CH4_OFI

Sets the user-preferred physical NIC index used by CH4 OFI after usable libfabric NICs have been discovered, filtered, and sorted by provider domain name. It is used only when libfabric NIC information is available, at least one PCI NIC is detected, and the requested index is within the detected NIC count; otherwise CH4 OFI keeps its normal NIC ordering. The detected NIC set can be affected by `MPIR_CVAR_OFI_SKIP_IPV6` provider filtering, and the number of NICs kept available is determined separately by `MPIR_CVAR_CH4_OFI_MAX_NICS`. When active, this CVAR moves the selected detected NIC to the front of the CH4 OFI NIC ordering and bypasses the usual locality, SNC4 CXI, and local-rank NIC ordering choices for that selection.

* -1 - default, do not set a preferred NIC; use the normal CH4 OFI NIC ordering.
* 0 through detected NIC count minus 1 - select the NIC at that index in the domain-name-sorted detected NIC list as the preferred NIC.

Used in functions: setup_multi_nic

**MPIR_CVAR_CH4_OFI_RANK_BITS** - CH4_OFI

Sets the number of OFI tag-match bits CH4 OFI allocates to MPI source ranks. It is used while OFI runtime settings are initialized, provider hints are built, global OFI match-bit limits are checked, and debug settings are printed. It can override the provider capability-set default, but the effective match-bit layout also depends on the context-ID and user-tag bit settings and must fit with the OFI protocol bits in a 64-bit match value. When OFI completion data is enabled, source-rank bits are not included in the provider tag-format hint; when OFI completion data is disabled and no source-rank match bits are available, CH4 OFI replaces the context, source-rank, and user-tag bit layout with fallback values that include source-rank bits. When active, this CVAR sets the source-rank portion of the OFI tag-matching bit layout and determines the maximum supported rank count checked during OFI initialization.

* -1 - default, use the provider capability-set value.
* nonnegative integer - use this as the number of OFI tag-match bits allocated to MPI source ranks.

Used in functions: MPIDI_OFI_init_hints, MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, update_global_limits, dump_global_settings

**MPIR_CVAR_CH4_OFI_RMA_IOVEC_MAX** - CH4_OFI

Sets the maximum number of iovec entries CH4 OFI allocates at a time when processing noncontiguous RMA put and get operations. It is used by both direct no-pack put/get handling and packed put/get request setup, after the total target and, for no-pack operations, origin iovec counts have been computed from the datatypes. No alternate CVAR name, dependency on another CVAR, or additional active mode requirement is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/ofi_rma.c`; the setting is meaningful for CH4 OFI noncontiguous RMA paths. When active, this CVAR sets the per-allocation cap for temporary iovec arrays used to walk noncontiguous RMA buffers.

* 16384 - default, allocate up to 16384 iovec entries at a time for noncontiguous RMA buffer traversal.
* positive integer - allocate up to the specified number of iovec entries at a time.

Used in functions: MPIDI_OFI_nopack_putget, MPIDI_OFI_pack_put, MPIDI_OFI_pack_get

**MPIR_CVAR_CH4_OFI_RMA_PROGRESS_INTERVAL** - CH4_OFI

Sets the interval at which CH4 OFI manually drives progress for outstanding RMA operations on a window. It is used only when OFI data auto-progress is not enabled and RMA progress interval handling is not disabled; otherwise it is ignored. When active, this CVAR causes CH4 OFI to count RMA progress-trigger opportunities on each window and call the blocking window progress routine when the configured interval is reached.

* 100 - default, manually progress outstanding window RMA operations every 100 trigger opportunities.
* positive integer - manually progress outstanding window RMA operations after the specified number of trigger opportunities.
* 0 or negative integer other than -1 - manually progress outstanding window RMA operations whenever the trigger routine is called.
* -1 - disable interval-based manual RMA progress triggering.

Used in functions: MPIDI_OFI_win_trigger_rma_progress

**MPIR_CVAR_CH4_OFI_RNDV_PROTOCOL** - CH4_OFI

Sets the large-message protocol used by CH4 OFI rendezvous transfers after message size exceeds `MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD`. It is used when the receiver handles an OFI rendezvous message and when the sender receives the clear-to-send reply. Protocol selection also depends on whether the sender or receiver buffer/datatype requires packing and on whether the receiver data size fits within the OFI provider maximum message size; if a requested protocol cannot satisfy those conditions, CH4 OFI falls back to automatic protocol selection. When active, this CVAR selects the data-transfer protocol used after the rendezvous handshake.

* auto - default, choose the rendezvous protocol from buffer packing requirements and provider message-size limits.
* pipeline - use the pipeline protocol with packing and unpacking.
* read - use RDMA read.
* write - use RDMA write.
* direct - send data directly with libfabric after the rendezvous handshake.

Used in functions: get_rndv_protocol, MPIDI_OFI_recv_rndv_event, MPIDI_OFI_rndv_cts_event

**MPIR_CVAR_CH4_OFI_TAG_BITS** - CH4_OFI

Sets the number of OFI tag-match bits CH4 OFI allocates to MPI user tags. It is used while OFI runtime settings are initialized, global OFI match-bit limits are checked, the CH4-visible tag-bit count is returned from OFI initialization, and debug settings are printed. It can override the provider capability-set default, but the effective match-bit layout also depends on the context-ID and source-rank bit settings and must fit with the OFI protocol bits in a 64-bit match value. If OFI completion data is disabled and no source-rank match bits are available, CH4 OFI replaces the context, source-rank, and user-tag bit layout with fallback values that include source-rank bits. When active, this CVAR sets the user-tag portion of the OFI tag-matching bit layout and determines the maximum MPI tag value reported by OFI initialization.

* -1 - default, use the provider capability-set value.
* nonnegative integer - use this as the number of OFI tag-match bits allocated to MPI user tags.

Used in functions: MPIDI_OFI_init_settings, MPIDI_OFI_update_global_settings, MPIDI_OFI_init_local, update_global_limits, dump_global_settings

**MPIR_CVAR_CH4_PACK_BUFFER_SIZE** - CH4

Sets the byte size of each buffer in the CH4 per-VCI pack-buffer pool used for packing and unpacking active-message data. It is used when the pool is created, when unexpected eager active-message receives allocate a temporary pack buffer, and during initialization checks that the buffer is large enough for active-message eager limits. The pool allocation also uses `MPIR_CVAR_CH4_NUM_PACK_BUFFERS_PER_CHUNK` and `MPIR_CVAR_CH4_MAX_NUM_PACK_BUFFERS`; the required minimum size depends on the active-message eager buffer limit from the netmod, or the maximum of the shm and netmod limits when CH4 is not direct netmod. CH4 OFI also requires this size to cover the OFI default short-send size. When active, this CVAR sets the per-buffer byte capacity available for CH4 active-message pack-buffer pool cells.

* 16384 - default, use 16384-byte pack-buffer pool cells.
* positive integer - use the specified byte size for each pack-buffer pool cell; it must be large enough for the active-message eager limits and CH4 OFI short-send requirement.

Used in functions: MPIDI_init_per_vci, allocate_unexp_req_pack_buf, MPIDIG_am_check_init, MPIDI_OFI_init_local

**MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE** - COLLECTIVE

Sets the source for the POSIX collective-selection tuning table used by CH4 shared-memory POSIX collectives. It is used during POSIX local initialization to create the non-GPU collective-selection tree; POSIX barrier, broadcast, allreduce, and reduce consult that tree only when their POSIX intranode algorithm CVARs are set to automatic selection. Broadcast also chooses between the non-GPU and GPU selection trees based on `MPIR_CVAR_COLL_HYBRID_MEMORY` and the buffer memory type; the GPU selection tree is configured by `MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE_GPU`. When active through automatic POSIX collective selection, this CVAR sets whether POSIX collectives use the built-in generic tuning table or a tuning table loaded from a JSON file.

* "" - default, use the built-in generic POSIX collective-selection tuning table.
* string - use the specified JSON file as the POSIX collective-selection tuning table.

Used in functions: posix_coll_init

**MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE_GPU** - COLLECTIVE

Sets the source for the GPU POSIX collective-selection tuning table used by CH4 shared-memory POSIX collectives. It is used during POSIX local initialization to create the GPU collective-selection tree. In `src/mpid/ch4/shm/posix/posix_init.c`, this initialization is unconditional alongside the non-GPU collective-selection tree and does not rely on another CVAR or mode being active. When active, this CVAR sets whether the GPU POSIX collective-selection tree uses the built-in generic tuning table or a tuning table loaded from a JSON file.

* "" - default, use the built-in generic POSIX collective-selection tuning table for GPU collective selection.
* string - use the specified JSON file as the GPU POSIX collective-selection tuning table.

Used in functions: posix_coll_init

**MPIR_CVAR_CH4_PROGRESS_THROTTLE** - CH4

Controls whether CH4 progress throttles repeated progress checks that do not make progress. It is used at the end of `MPIDI_progress_test`; successful progress resets the consecutive no-progress counter, while unsuccessful progress increments it until it exceeds `MPIR_CVAR_CH4_PROGRESS_THROTTLE_NO_PROGRESS_COUNT`. This CVAR is used only when enabled, and the threshold for when throttling begins is set by `MPIR_CVAR_CH4_PROGRESS_THROTTLE_NO_PROGRESS_COUNT`. When active, this CVAR enables yielding during CH4 progress after enough consecutive polls fail to make progress.

* false - default, do not throttle CH4 progress checks after repeated no-progress polls.
* true - yield during CH4 progress after the consecutive no-progress count exceeds `MPIR_CVAR_CH4_PROGRESS_THROTTLE_NO_PROGRESS_COUNT`.

Used in functions: MPIDI_progress_test

**MPIR_CVAR_CH4_PROGRESS_THROTTLE_NO_PROGRESS_COUNT** - CH4

Sets the consecutive no-progress polling threshold used by CH4 progress throttling. It is used at the end of `MPIDI_progress_test` only when `MPIR_CVAR_CH4_PROGRESS_THROTTLE` is enabled; successful progress resets the counter, while unsuccessful progress increments it until it exceeds this CVAR and then yields on repeated no-progress checks. When active, this CVAR sets how many consecutive CH4 progress polls may fail to make progress before throttling yields during CH4 progress.

* 4096 - default, allow 4096 consecutive no-progress poll checks before throttling begins.
* non-negative integer - allow the specified number of consecutive no-progress poll checks before throttling begins.
* negative integer - yield on every no-progress poll check while throttling is enabled.

Used in functions: MPIDI_progress_test

**MPIR_CVAR_CH4_RESERVE_VCIS** - CH4

Controls how many CH4 VCIs are reserved for explicit user allocation, such as stream allocation. It is used when CH4 VCI state is initialized and when multiple VCIs are enabled for a communicator; the reserved VCIs are the VCIs above the implicit VCI range set by `MPIR_CVAR_CH4_NUM_VCIS`. The requested implicit and reserved VCIs should fit within `MPIDI_CH4_MAX_VCIS`, and allocation of reserved VCIs requires a build with more than one CH4 VCI. When active, this CVAR sets the number of VCIs available for explicit allocation.

* 0 - default, do not reserve VCIs for explicit allocation.
* positive integer - number of VCIs to reserve for explicit allocation, subject to the CH4 maximum VCI configuration and what the netmod creates.

Used in functions: MPIDI_vci_init, MPIDI_Comm_set_vcis, MPID_Allocate_vci

**MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL** - CH4

Sets the static polling interval for CH4 RMA target-side active-message progress when dynamic RMA AM progress is not active. It is used by the internal RMA active-message polling decision that is consulted by `MPI_Win_flush`, `MPI_Win_flush_all`, `MPI_Win_flush_local`, and `MPI_Win_flush_local_all`; the interval is counted globally across all windows. This CVAR is used only when `MPIR_CVAR_CH4_RMA_ENABLE_DYNAMIC_AM_PROGRESS` is false. When active, this CVAR sets how often the static RMA AM progress path polls for incoming target-side RMA active messages during passive-target flush synchronization.

* 1 - default, poll once on every relevant synchronization call.
* integer greater than 1 - poll once every specified number of relevant synchronization calls.
* integer less than or equal to 0 - disable this polling.

Used in functions: MPIDIG_rma_need_poll_am, MPIDIG_mpi_win_flush, MPIDIG_mpi_win_flush_all, MPIDIG_mpi_win_flush_local, MPIDIG_mpi_win_flush_local_all

**MPIR_CVAR_CH4_RMA_AM_PROGRESS_LOW_FREQ_INTERVAL** - CH4

Sets the low-frequency polling interval for CH4 RMA target-side active-message progress while dynamic RMA AM progress is active and no target-side RMA active message has been observed yet. It is used by the internal RMA active-message polling decision that is consulted by `MPI_Win_flush`, `MPI_Win_flush_all`, `MPI_Win_flush_local`, and `MPI_Win_flush_local_all`; the interval is counted globally across all windows. Once a target-side RMA active message has arrived, dynamic progress polls at every relevant synchronization call instead of using this interval. This CVAR is used only when `MPIR_CVAR_CH4_RMA_ENABLE_DYNAMIC_AM_PROGRESS` is true; otherwise, static polling is controlled by `MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL`. When active, this CVAR sets how often the dynamic RMA AM progress path performs low-frequency polling before switching to every-call polling after target-side RMA active-message arrival.

* 100 - default, poll once every 100 relevant synchronization calls while in the low-frequency dynamic polling phase.
* positive integer - poll once every specified number of relevant synchronization calls while in the low-frequency dynamic polling phase.

Used in functions: MPIDIG_rma_need_poll_am, MPIDIG_mpi_win_flush, MPIDIG_mpi_win_flush_all, MPIDIG_mpi_win_flush_local, MPIDIG_mpi_win_flush_local_all

**MPIR_CVAR_CH4_RMA_ENABLE_DYNAMIC_AM_PROGRESS** - CH4

Controls whether CH4 RMA passive-target synchronization dynamically adjusts progress polling for incoming RMA active messages received on the target process. It is used by the internal RMA active-message polling decision that is consulted by `MPI_Win_flush`, `MPI_Win_flush_all`, `MPI_Win_flush_local`, and `MPI_Win_flush_local_all`; the polling interval is counted globally across all windows. Dynamic polling uses `MPIR_CVAR_CH4_RMA_AM_PROGRESS_LOW_FREQ_INTERVAL` before any target-side RMA active message has arrived, then polls at every relevant synchronization call after one has arrived. When dynamic polling is active, `MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL` is not used. When active, this CVAR sets the RMA synchronization polling mode from static interval polling to adaptive low-frequency-then-every-call polling for target-side RMA active-message progress.

* false - default, use the static polling behavior controlled by `MPIR_CVAR_CH4_RMA_AM_PROGRESS_INTERVAL`.
* true - use dynamic polling, initially at the low-frequency interval controlled by `MPIR_CVAR_CH4_RMA_AM_PROGRESS_LOW_FREQ_INTERVAL` and then once on every relevant synchronization call after any target-side RMA active message has arrived.

Used in functions: MPIDIG_rma_need_poll_am, MPIDIG_mpi_win_flush, MPIDIG_mpi_win_flush_all, MPIDIG_mpi_win_flush_local, MPIDIG_mpi_win_flush_local_all

**MPIR_CVAR_CH4_RMA_MEM_EFFICIENT** - CH4

Controls CH4 RMA per-target object lifetime for synchronization epochs that allocate per-target bookkeeping. It is used when completing PSCW start-complete epochs, ending individual lock-unlock epochs, and ending lock_all-unlock_all epochs; individual lock epoch checks also depend on it when deciding whether a cached, unlocked target object may be reused for another target lock. When active, this CVAR sets whether per-target objects are released at epoch end or kept cached for later reuse until window finalization.

* false - default, keep allocated per-target objects cached after epoch end for reuse and free them at window finalization.
* true - release per-target objects at the end of the relevant RMA synchronization epoch.

Used in functions: MPIDIG_mpi_win_lock, MPIDIG_mpi_win_complete, MPIDIG_mpi_win_unlock, MPIDIG_mpi_win_unlock_all

**MPIR_CVAR_CH4_ROOTS_ONLY_PMI** - CH4

Enables optimized CH4 PMI business-card exchange for node-root processes only. In the reviewed CH4 OFI path, it is used while computing address-vector table indexes for root endpoint addresses: when enabled, node-root ranks are indexed first and non-node-root ranks are offset around them. No alternate CVAR name or dependency on another CVAR is visible in `src/mpid/ch4/src/ch4_init.c` or `src/mpid/ch4/netmod/ofi/ofi_vci.c`; the OFI use is relevant when address-vector table indexing for root endpoint addresses is active.

* true - default, use node-root-only PMI exchange behavior and root address-vector table indexing with node-root ranks first.
* false - use rank-order PMI exchange behavior and root address-vector table indexing by rank.

Used in functions: get_root_av_table_index

**MPIR_CVAR_CH4_SHM** - CH4

Selects the CH4 shared-memory module during device initialization. In `src/mpid/ch4/src/ch4_init.c`, CH4 calls shared-memory local initialization from `MPID_Init` only when it is not built in direct-netmod mode; the selected shared-memory module contributes to the CH4 tag-bit limit together with the selected network module. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_init.c`. When active, this CVAR sets which CH4 shared-memory module is used for intra-node shared-memory communication.

* "" - default, use the default CH4 shared-memory module.
* registered CH4 shared-memory module name - use the matching CH4 shared-memory module.

Used in functions: MPID_Init

**MPIR_CVAR_CH4_SHM_POSIX_EAGER** - CH4

Selects the CH4 POSIX shared-memory eager module during POSIX local initialization. If this CVAR is empty, the first registered POSIX eager module is selected as the default; otherwise, the configured string is matched against the registered POSIX eager module names, and initialization fails if no registered module name matches. No alternate CVAR name or dependency on another CVAR is visible in `src/mpid/ch4/shm/posix/posix_init.c`. When active, this CVAR sets which POSIX eager function table is used for CH4 shared-memory eager communication.

* "" - default, use the first registered POSIX eager module.
* registered POSIX eager module name - use the matching POSIX eager module.

Used in functions: choose_posix_eager

**MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE** - CH4

Sets the size, in bytes, of each cell in each CH4 POSIX iqueue shared-memory cell pool. It is used when calculating the shared-memory slab size, when creating the iqueue transport cell pool, and when determining the eager buffer and payload limits; the payload limit is the cell size minus the iqueue cell header size. The shared-memory allocation also depends on `MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS`, the local process count, and, for nonzero VCI pairs, the configured VCI count. If CH4 POSIX topology support is enabled through `MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE`, the pool is created with both MPSC and MPMC free-queue types, otherwise it is created with only the MPSC free-queue type. When active, this CVAR sets the per-cell storage size used by CH4 POSIX eager iqueue communication.

* 8192 - default, use 8192-byte cells in each iqueue cell pool.
* positive aligned integer - use the specified aligned byte size for each cell in each iqueue cell pool.

Used in functions: MPIDI_POSIX_eager_payload_limit, MPIDI_POSIX_eager_buf_limit, init_transport, MPIDI_POSIX_iqueue_shm_size, MPIDI_POSIX_iqueue_init

**MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS** - CH4

Sets the number of cells in each CH4 POSIX iqueue shared-memory cell pool. It is used when calculating the shared-memory slab size and when creating the iqueue transport cell pool; if CH4 POSIX topology support is enabled through `MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE`, the pool is created with both MPSC and MPMC free-queue types, otherwise it is created with only the MPSC free-queue type. The total shared-memory allocation also depends on `MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE`, the local process count, and, for nonzero VCI pairs, the configured VCI count. When active, this CVAR sets the depth of the iqueue cell pool used by CH4 POSIX eager communication.

* 64 - default, use 64 cells in each iqueue cell pool.
* positive integer - use the specified number of cells in each iqueue cell pool.

Used in functions: init_transport, MPIDI_POSIX_iqueue_shm_size

**MPIR_CVAR_CH4_SHM_POSIX_TOPO_ENABLE** - CH4

Controls topology-aware behavior in the CH4 POSIX shared-memory path. It is used during POSIX local initialization to collect hardware-topology identifiers, during `MPI_COMM_WORLD` commit to exchange local-rank topology data and classify local-rank distances when there is more than one local process, and during iqueue setup to include topology-aware free-queue support in the shared-memory cell pool. The iqueue shared-memory allocation also depends on `MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_CELL_SIZE`, `MPIR_CVAR_CH4_SHM_POSIX_IQUEUE_NUM_CELLS`, the local process count, and the VCI count for nonzero VCI pairs. When active, this CVAR enables POSIX topology data collection, local-rank distance classification by cache and NUMA locality, and topology-aware iqueue pool setup.

* true - default, enable POSIX topology data collection, local-rank distance classification, and iqueue pools with MPSC and MPMC free-queue support.
* false - skip POSIX topology data collection and distance classification, leave local-rank distances as local, and create iqueue pools with only MPSC free-queue support.

Used in functions: MPIDI_POSIX_mpi_comm_commit_post_hook, init_transport, MPIDI_POSIX_iqueue_shm_size, MPIDI_POSIX_init_local

**MPIR_CVAR_CH4_UCX_ENABLE_UCC** - DEVELOPER

Controls whether CH4 UCX initializes UCC collective support during UCX local initialization. It is used only when MPICH is built with UCC support; when enabled, the UCC configuration also uses `MPIR_CVAR_CH4_UCX_UCC_VERBOSITY_LEVEL` and `MPIR_CVAR_CH4_UCX_UCC_ENABLE_DEBUG`, and threaded builds set the UCC threaded flag. When active, this CVAR enables UCC support for CH4 UCX.

* false - default, do not enable UCC support for CH4 UCX.
* true - enable UCC support for CH4 UCX when MPICH is built with UCC support.

Alternate name: MPIR_CVAR_CH4_UCC_ENABLE

Used in functions: MPIDI_UCX_init_local

**MPIR_CVAR_CH4_UCX_UCC_ENABLE_DEBUG** - DEVELOPER

Controls whether CH4 UCX passes the UCC wrapper debug setting into common UCC initialization. It is used only when MPICH is built with UCC support and `MPIR_CVAR_CH4_UCX_ENABLE_UCC` is enabled during UCX local initialization. When active, this CVAR enables additional debug output for UCC wrappers.

* false - default, do not enable additional debug output for UCC wrappers.
* true - enable additional debug output for UCC wrappers when UCC support is initialized.

Alternate name: MPIR_CVAR_CH4_UCC_ENABLE_DEBUG

Used in functions: MPIDI_UCX_init_local

**MPIR_CVAR_CH4_UCX_UCC_VERBOSITY_LEVEL** - CH4_UCX

Sets the verbosity output level passed into common UCC initialization for CH4 UCX UCC wrappers. It is used only when MPICH is built with UCC support and `MPIR_CVAR_CH4_UCX_ENABLE_UCC` is enabled during UCX local initialization. When active, this CVAR sets the UCC wrapper verbosity level using both the configured string and its integer conversion.

* 0 - default, use UCC wrapper verbosity level 0.
* integer string - set the UCC wrapper verbosity level to the specified integer value when UCC support is initialized.

Alternate name: MPIR_CVAR_CH4_UCC_VERBOSITY_LEVEL

Used in functions: MPIDI_UCX_init_local

**MPIR_CVAR_CH4_XPMEM_ENABLE** - CH4

Controls whether the CH4 XPMEM shared-memory IPC path may be initialized and selected for intranode point-to-point communication. It is used only when MPICH is built with the XPMEM shared-memory submodule; initialization is skipped when there is only one local process, and MPICH may force this setting off at runtime if XPMEM setup fails and silent fallback is enabled. XPMEM IPC selection also requires messages at least as large as `MPIR_CVAR_CH4_IPC_XPMEM_P2P_THRESHOLD`, no active upper-threshold exclusion from `MPIR_CVAR_CH4_IPC_XPMEM_P2P_UPPER_THRESHOLD`, and repeated-address detection for the buffer. When active, this CVAR enables use of XPMEM-based single-copy IPC transfers.

* true - default, allow eligible intranode point-to-point messages to use XPMEM-based single-copy IPC transfers.
* false - disable XPMEM initialization and selection of the XPMEM-based IPC transfer path.

Used in functions: MPIDI_XPMEM_init_local, MPIDI_XPMEM_comm_bootstrap, MPIDI_XPMEM_get_ipc_attr

**MPIR_CVAR_CHOP_ERROR_STACK** - ERROR_HANDLING

Controls the line width used when formatting MPI error stack output. It is used when `MPIR_Err_print_stack_string` emits error-ring entries for a printed error stack; that formatting path is reached when `MPIR_CVAR_PRINT_ERROR_STACK` enables full stack output. During error-stack initialization, a request for the sensible default is resolved before stack output is printed. When active, this CVAR sets the width used to chop long error stack message lines and indent continuation text under the stack location prefix.

* 0 - default, do not chop error stack output lines.
* negative integer - use a sensible default width, resolved during error-stack initialization.
* positive integer - chop long error stack message lines to the requested width.

Used in functions: MPIR_Err_stack_init, MPIR_Err_print_stack_string

**MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE** - COLLECTIVE

Sets the chunk size, in bytes, for circ_graph collective pipeline data transfer. It is used when initializing circ_graph broadcast, allgather, and reduce queues after a circ_graph collective algorithm is selected. It determines how data is split into blocks for broadcast and allgather, determines reduction block sizes rounded to whole datatype elements, and controls whether the registered host chunk-buffer pool is created for pack buffers. The chunk-buffer pool sizing also depends on `MPIR_CVAR_CIRC_GRAPH_NUM_CHUNKS` and `MPIR_CVAR_CIRC_GRAPH_MAX_CHUNKS`, and pack buffers are used only when the queue needs packing for non-contiguous datatypes or GPU buffers.

* 0 - Disables creation of the registered host chunk-buffer pool and uses a single chunk for each transfer.
* positive integer - default is 131072, represents the target chunk size in bytes for pipelined circ_graph transfers and enables creation of the registered host chunk-buffer pool.

Used in functions: MPIR_cga_init, MPIR_cga_finalize, MPII_cga_init_bcast_queue, MPII_cga_init_allgather_queue, MPII_cga_init_reduce_queue

**MPIR_CVAR_CIRC_GRAPH_MAX_CHUNKS** - COLLECTIVE

Sets the maximum number of chunk buffers reserved for circ_graph collective pipeline data transfer. It is used when creating the registered host chunk-buffer pool during circ_graph initialization. This CVAR is effective only when `MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE` is greater than zero, which enables creation of the chunk-buffer pool, and it caps the pool initialized with `MPIR_CVAR_CIRC_GRAPH_NUM_CHUNKS`.

* integer - default is 1024, represents the maximum number of chunk buffers for the registered host chunk-buffer pool.

Used in functions: MPIR_cga_init

**MPIR_CVAR_CIRC_GRAPH_NUM_CHUNKS** - COLLECTIVE

Sets the initial number of chunk buffers reserved for circ_graph collective pipeline data transfer. It is used when creating the registered host chunk-buffer pool during circ_graph initialization. This CVAR is effective only when `MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE` is greater than zero, which enables creation of the chunk-buffer pool, and the pool is also capped by `MPIR_CVAR_CIRC_GRAPH_MAX_CHUNKS`.

* integer - default is 32, represents the initial number of chunk buffers for the registered host chunk-buffer pool.

Used in functions: MPIR_cga_init

**MPIR_CVAR_CIRC_GRAPH_Q_LEN** - COLLECTIVE

Sets the request queue length for the circ_graph collective algorithm runtime. It is used when initializing circ_graph broadcast, allgather, and reduce queues, and controls how many nonblocking send or receive requests can be tracked in the queue at once. It works with `MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE`, which determines the chunking used to initialize the same queues.

* integer less than 2 - Uses a minimum queue length of 2 to avoid circular dependency issues.
* integer 2 or greater - default is 8, represents the request queue length.

Used in functions: MPII_cga_init_bcast_queue, MPII_cga_init_allgather_queue, MPII_cga_init_reduce_queue

**MPIR_CVAR_CLIQUES_BY_BLOCK** - NODEMAP

Controls how MPICH assigns processes to multiple local cliques on a single local node for nodemap debugging. It is used by `MPIR_build_nodemap` only after process-manager node mapping finds a single node and local-clique partitioning is active; local-clique partitioning is active when `MPIR_CVAR_NUM_CLIQUES` requests more than one clique, or when the deprecated `MPIR_CVAR_ODD_EVEN_CLIQUES` requests two cliques and `MPIR_CVAR_NUM_CLIQUES` does not override it. It is not used when `MPIR_CVAR_NOLOCAL` forces one process per node. When active, this CVAR enables assigning processes to cliques by uniform blocks instead of the default round-robin assignment.

* false - default, assign processes to local cliques in round-robin order.
* true - assign processes to local cliques by uniform blocks.

Used in functions: MPIR_build_nodemap

**MPIR_CVAR_COLLECTIVE_FALLBACK** - COLLECTIVE

Controls how MPICH handles collective fallback when a selected collective path cannot be used. It is used when a user-selected collective algorithm is not usable for the supplied arguments, when POSIX release-gather intra-node collectives would exceed `MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE`, and when topology-aware intra-node trees request an unsupported `knomial_2` tree. The topology-aware tree case depends on topology-aware intra-node trees being active, which requires `MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES`, user-provided process binding, and initialized hardware topology support. When active, this CVAR sets the policy for whether MPICH reports an error or falls back to an internally selected collective path, optionally with a diagnostic message.

* error - throw an error.
* print - print an error message and fallback to the internally selected algorithm.
* silent - default, silently fallback to the internally selected algorithm.

Used in functions: MPII_COLLECTIVE_FALLBACK_CHECK, MPIDI_SHM_create_template_tree, MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init

**MPIR_CVAR_COLL_ALIAS_CHECK** - COLLECTIVE

Enables checking for aliased buffers in collective-operation error checks. It is used when the collective alias-check macro is invoked; it does not rely on other CVAR values in `mpir_err.h`.

* 0 - Disables collective alias checking.
* 1 - default, enables collective alias checking.

Used in functions: MPIR_ERRTEST_ALIAS_COLL

**MPIR_CVAR_COLL_SCHED_DUMP** - COLLECTIVE

Controls whether MPIDU schedule state is printed for nonblocking collective schedules. It is used when a schedule is started and while pending schedules are scanned for progress; it does not rely on another CVAR or mode being active beyond schedule start and progress reaching these paths. When active, this CVAR enables dumping schedule details to `stderr` for debugging nonblocking collective schedule execution.

* false - default, do not print MPIDU schedule dumps.
* true - print MPIDU schedule dumps to `stderr` when schedules start and during schedule progress.

Used in functions: MPIDU_Sched_start, MPIDU_Sched_progress_state

**MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE** - COLLECTIVE

Sets the source of collective-selection tuning data during collective initialization. MPICH uses this data to build the collective selection tree, either from the built-in generic tuning data or from a user-provided JSON tuning file. This selection tree is used by internal collective algorithm selection, including per-collective algorithm CVARs when they are in automatic selection mode; forcing a specific per-collective algorithm bypasses automatic collective selection for that collective.

* "" - default, uses the built-in generic collective-selection tuning data.
* non-empty string - represents the path to a JSON tuning file used to build the collective selection tree.

Used in functions: MPII_Coll_init

**MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE** - COLLECTIVE

Sets the maximum shared memory per node that POSIX shared-memory release-gather collectives may allocate for optimized intranode collectives. It is checked when blocking release-gather communicator state needs flags, broadcast buffers, or reduce buffers, and when nonblocking release-gather communicator state needs broadcast or reduce flags and buffers. The requested allocation depends on the current per-node shared-memory counter, communicator size, page-aligned flag storage, `MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE`, `MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE`, and, for nonblocking flag storage, `MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS` or `MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS`. If the request would exceed this limit, initialization fails and the collective falls back; `MPIR_CVAR_COLLECTIVE_FALLBACK` controls whether a diagnostic message is printed. When active, this CVAR sets the shared-memory allocation cap used by POSIX release-gather collective initialization.

* integer - default is 65536, represents the per-node shared-memory allocation limit in KB.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init

**MPIR_CVAR_COLL_TREE_DUMP** - COLLECTIVE

Enables dumping MPIR tree algorithm topology information for each rank to `colltree[rank].json` in the current folder when a tree is created, and prints the tree type and output file name to stdout. It is used only when a collective path creates an MPIR treealgo tree. The tree creation paths used by collectives are influenced by tree-type CVARs initialized from `MPIR_CVAR_IALLREDUCE_TREE_TYPE`, `MPIR_CVAR_ALLREDUCE_TREE_TYPE`, `MPIR_CVAR_IBCAST_TREE_TYPE`, `MPIR_CVAR_BCAST_TREE_TYPE`, and `MPIR_CVAR_IREDUCE_TREE_TYPE`.

* false - default, disables tree dumping.
* true - enables tree dumping for regular, topology-aware, and topology-wave MPIR treealgo tree creation.

Used in functions: MPIR_Treealgo_tree_create, MPIR_Treealgo_tree_create_topo_aware, MPIR_Treealgo_tree_create_topo_wave

**MPIR_CVAR_COMM_SPLIT_USE_QSORT** - COMMUNICATOR

Controls whether `MPI_Comm_split` uses the qsort-based sorting path when ordering processes by key. It is used while constructing the rank order for a new communicator after the split color has selected participating local processes, and for intercommunicators it is also used when ordering the matching remote group. This path requires qsort support to be available at build time; otherwise the insertion-sort path is used. When active, this CVAR enables qsort-based stable ordering of split participants by key.

* true - default, use the qsort-based sorting path when qsort support is available.
* false - use the insertion-sort path.

Used in functions: MPIU_Sort_inttable, MPIR_Comm_split_impl

**MPIR_CVAR_COORDINATES_DUMP** - COLLECTIVE

Controls whether rank 0 writes the network coordinates to a file named `coords` in the current working directory during PMI initialization. The dump occurs only on rank 0 and uses the coordinates currently stored in `MPIR_Process.coords`; those coordinates may come from `MPIR_CVAR_COORDINATES_FILE` when a coordinates file is configured, or from other coordinate initialization paths. When active, this CVAR enables debug output of the process network coordinates.

* false - default, do not dump the network coordinates.
* true - dump the network coordinates from rank 0 to `coords`.

Used in functions: MPIR_pmi_init

**MPIR_CVAR_COORDINATES_FILE** - COLLECTIVE

Sets the input file used to load network coordinates during PMI initialization. It is parsed after MPICH has initialized PMI process information, node mapping, and locality data, and it does not require another CVAR or collective mode to be active before parsing. When active, this CVAR populates `MPIR_Process.coords` and `MPIR_Process.coords_dims` with per-rank network coordinate data for later topology-aware collective logic and coordinate dumping.

* "" - default, disables loading network coordinates from an input file.
* non-empty string - represents the path to a coordinates file to parse for per-rank network coordinates.

Used in functions: MPIR_pmi_init, parse_coord_file

**MPIR_CVAR_COREDUMP_ON_ABORT** - ERROR_HANDLING

Controls whether MPICH calls libc `abort()` from the device abort path. It is used after abort diagnostics are emitted and debugger abort state is set, before the normal process-manager abort or direct exit handling. It does not rely on any other CVAR or mode being active. When active, this CVAR enables aborting through libc so the process can generate a core dump.

* false - default, continue through the normal MPICH abort path without calling libc `abort()`.
* true - call libc `abort()` from the device abort path.

Used in functions: MPID_Abort

**MPIR_CVAR_CTXID_EAGER_SIZE** - THREADS

Controls how many words of the context ID mask are reserved for the eager context ID allocation protocol. It is initialized when context ID allocation state is set up and is used by the first allocation attempt for participating processes that need a context ID; processes that ignore the ID do not use the eager mask. The selected size must leave mask space for the base allocation protocol, and this CVAR does not depend on any other CVAR or mode being active. When active, this CVAR sets the size of the eager mask segment used to try allocating a context ID during the initial synchronization step before falling back to the base protocol.

* 0 - disable the eager allocation mask segment.
* positive integer less than `MPIR_MAX_CONTEXT_MASK - 1` - number of context ID mask words reserved for eager allocation.

Used in functions: gcn_init, gcn_copy_mask

**MPIR_CVAR_DATALOOP_FAST_SEEK** - DATALOOP

Controls whether dataloop segment repositioning can use the datatype-specialized fast seek path instead of advancing through the generic segment manipulation path. It is used when segment manipulation needs to reposition to a later stream offset, and the fast path is available only from the start of a segment with local element sizes. When active, this CVAR enables shortcut setup of the segment cursor for the requested noncontiguous buffer position.

* 0 - use the generic segment manipulation fallback path for seeking.
* nonzero - default, use the datatype-specialized fast seek path when its preconditions are met.

Used in functions: segment_seek

**MPIR_CVAR_DEBUG_SUMMARY** - DEVELOPER

Controls whether MPICH prints internal debug-summary information. It is used during memory tracing finalization to print memory allocation by category, and each layer may print its own summary information. When active, this CVAR enables internal debug summaries, including memory category information.

* 0 - default, do not print internal debug-summary information.
* 1 - print internal debug-summary information, such as memory allocation by category.
* 2 - also print the preferred NIC for each rank.

Alternate name: MPIR_CVAR_MEM_CATEGORY_INFORMATION
Alternate name: MPIR_CVAR_CH4_RUNTIME_CONF_DEBUG

Used in functions: MPII_finalize_memory_tracing

**MPIR_CVAR_DEFAULT_THREAD_LEVEL** - THREADS

Sets the default MPI thread-support level requested by `MPI_Init`. It is used only when initializing MPI through `MPI_Init`; explicit thread-level requests through `MPI_Init_thread` or session initialization choose their thread level through their own arguments or info hints. The value is read from the environment before calling the common initialization path and is case-insensitive.

* MPI_THREAD_SINGLE - Only one thread will execute.
* MPI_THREAD_FUNNELED - The process may be multi-threaded, but only the main thread will make MPI calls.
* MPI_THREAD_SERIALIZED - The process may be multi-threaded, and multiple threads may make MPI calls, but only one at a time.
* MPI_THREAD_MULTIPLE - Multiple threads may call MPI, with no restrictions.

Used in functions: MPI_Init
**MPIR_CVAR_DEVICE_COLLECTIVES** - COLLECTIVE

Controls whether the device can override MPIR-level collective algorithms. When set to use per-collective control, the corresponding per-collective device CVARs decide whether device override is allowed for each collective. When active, this CVAR sets the global policy for preferring device collective implementations over MPIR-level implementations.

* all - Always prefer device collectives.
* none - Never select device collectives.
* percoll - default, use the per-collective CVARs to decide whether device collectives are selected.

Used in functions: MPID_Init, devcollstr

**MPIR_CVAR_DIMS_VERBOSE** - DIMS

Controls whether MPICH prints verbose diagnostics while computing dimensions for `MPI_Dims_create`. It is used in the MPIR-level dims-create implementation when factoring the remaining node count, handling special decompositions, checking divisor candidates, pruning balance searches, and reporting initial and final decompositions. It does not rely on another CVAR or mode being active. When active, this CVAR enables verbose output about the internal `MPI_Dims_create` decomposition process.

* false - default, do not print verbose `MPI_Dims_create` diagnostics.
* true - print verbose diagnostics while computing `MPI_Dims_create` dimensions.

Used in functions: MPIR_Dims_create_impl, optbalance

**MPIR_CVAR_ENABLE_FT** - FT

Enables fault-tolerance helper paths for CH3/Nemesis large-message transfer request tracking, cleanup of pending RTS requests when a virtual connection terminates, and detection of incomplete anysource requests whose communicator no longer has anysource receives enabled. The LMT RTS tracking is used after a large-message transfer is initiated through a VC that provides LMT functions, and anysource mismatch detection also depends on the request being incomplete, marked anysource, and associated with a communicator where anysource receives are disabled.

* false - default, disables the FT-specific LMT RTS queue tracking and cleanup, and disables anysource mismatch detection.
* true - enables the FT-specific LMT RTS queue tracking and cleanup, and enables anysource mismatch detection.

Used in functions: MPID_nem_lmt_RndvSend, pkt_CTS_handler, MPIR_Request_is_anysrc_mismatched, MPID_nem_lmt_shm_vc_terminated

**MPIR_CVAR_ENABLE_GPU** - GPU

Controls whether MPICH initializes GPU support, enables GPU pointer detection and GPU-aware helper paths, reports supported GPU memory kinds, initializes Yaksa with GPU support, enables CH4 IPC GPU setup when the GPU SHM module is built, and requests OFI HMEM capability when OFI HMEM is enabled. During initialization, MPICH may set this CVAR to disabled if no GPU devices are found. Compile-time GPU support and per-thread GPU disable state can also prevent GPU helper paths from being used.

* 0 - Disables MPICH GPU support and treats buffers as host memory for internal GPU queries.
* 1 - default, enables MPICH GPU support when GPU support is available and devices are present.

Used in functions: MPII_init_gpu, MPII_finalize_gpu, MPIR_get_supported_memory_kinds, init_gpu_kinds, MPIDI_IPC_comm_bootstrap, MPID_Init, MPIDI_OFI_init_hints, MPIR_Typerep_init, MPIR_GPU_query_support_impl, MPIDI_GPU_init_local

**MPIR_CVAR_ENABLE_GPU_REGISTER** - GPU

Controls whether MPICH registers and unregisters host buffers with the GPU runtime in the GPU host-buffer registration helpers. Registration is attempted only when GPU support is active through `MPIR_CVAR_ENABLE_GPU`, compile-time GPU support is available, and the per-thread GPU disable state is not set.

* false - Skips GPU runtime host-buffer registration and unregistration.
* true - default, enables GPU runtime host-buffer registration and unregistration when GPU support is active.

Used in functions: MPIR_gpu_register_host, MPIR_gpu_unregister_host

**MPIR_CVAR_ENABLE_HEAVY_YIELD** - THREADS

Controls whether MPIDU thread yielding uses a heavier yield operation. It is used in `MPIDU_Thread_yield`; the reviewed source does not show it as an alternate name for another CVAR or as depending on another CVAR. When active, this CVAR enables yielding through a short `nanosleep` so other threads have a chance to acquire a lock, instead of using the regular MPL thread yield operation. This may not work with some thread runtimes, such as non-preemptive user-level threads.

* false - default, use the regular MPL thread yield operation.
* true - use a short `nanosleep` for thread yielding.

Used in functions: MPIDU_Thread_yield

**MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES** - COLLECTIVE

Enables topology-aware tree creation for POSIX shared-memory release-gather broadcast and reduce trees. It is used when blocking or nonblocking release-gather tree state is initialized for a communicator, and topology-aware tree creation is attempted only when user-provided process binding is present and hardware topology support is initialized. The attempted topology-aware trees use the intranode broadcast and normal-size reduce tree type and k-value CVARs; if topology-aware creation is not attempted or does not succeed, the corresponding non-topology-aware trees are created instead. When active, this CVAR enables construction of package-aware intranode release-gather trees that separate package-local ranks from package leaders for broadcast and reduce.

* 1 - default, attempt topology-aware intranode release-gather broadcast and reduce tree creation when binding and hardware topology requirements are met.
* 0 - disable topology-aware intranode release-gather tree creation and use non-topology-aware trees.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init

**MPIR_CVAR_ENABLE_YAKSA_REDUCTION** - COLLECTIVE

Controls whether yaksa-based reductions are considered for local typerep reductions. It is checked first when MPICH determines whether a reduction operation and datatype can use yaksa; when it is enabled, yaksa is still used only if the message size is allowed by `MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD` for nonzero counts and the datatype and operation pass the remaining yaksa support checks, including the GPU floating-point and complex datatype support CVARs. When active, this CVAR enables the yaksa reduction path instead of forcing the fallback reduction path.

* 0 - disable yaksa-based reductions and use the fallback reduction path.
* nonzero - default, allow yaksa-based reductions when the threshold, datatype, and operation support checks pass.

Used in functions: MPIR_Typerep_reduce_is_supported

**MPIR_CVAR_ERROR_CHECKING** - ERROR_HANDLING

Controls whether MPICH performs runtime error checks, typically to validate inputs to MPI routines. It is used during local process attribute initialization to set `MPIR_Process.do_error_checks`, and it does not rely on any other CVAR being enabled. This CVAR is effective only when MPICH is configured with runtime error-checking control; builds without error checking disable these checks, and builds with non-runtime error checking enable them unconditionally. When active, this CVAR enables runtime MPI error checking.

* true - default, enable runtime MPI error checking when MPICH is configured for runtime error-checking control.
* false - disable runtime MPI error checking when MPICH is configured for runtime error-checking control.

Used in functions: MPII_init_local_proc_attrs

**MPIR_CVAR_EXSCAN_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Exscan` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Exscan.

* true - default, allows the device to override MPIR-level `MPI_Exscan` collective algorithms.
* false - disables the device override for `MPI_Exscan`.

Used in functions: MPI_Exscan

**MPIR_CVAR_EXSCAN_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Exscan_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Exscan_init.

* true - default, allows the device to override MPIR-level `MPI_Exscan_init` collective algorithms.
* false - disables the device override for `MPI_Exscan_init`.

Used in functions: MPI_Exscan_init

**MPIR_CVAR_EXSCAN_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Exscan algorithm. It is used for `MPI_Exscan` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Exscan implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.
* recursive_doubling - forces the recursive-doubling algorithm.

Used in functions: MPI_Exscan

**MPIR_CVAR_FINALIZE_WAIT** - COLLECTIVE

Controls whether MPI finalization waits for pending references on built-in communicators to clear. It is used while finalizing MPI_COMM_SELF, MPI_COMM_WORLD, and MPI_COMM_PARENT, after inactive requests are freed and only when a built-in communicator still has more than its final internal reference. When active, this CVAR enables progress polling during finalization until the built-in communicator reference count drops to the releasable reference.

* false - default, report pending references without waiting for them to clear.
* true - poll progress until pending references on the built-in communicator clear.

Used in functions: finalize_builtin_comm

**MPIR_CVAR_GATHERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Gatherv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Gatherv.

* true - default, allows the device to override MPIR-level `MPI_Gatherv` collective algorithms.
* false - disables the device override for `MPI_Gatherv`.

Used in functions: MPI_Gatherv

**MPIR_CVAR_GATHERV_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Gatherv_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Gatherv_init.

* true - default, allows the device to override MPIR-level `MPI_Gatherv_init` collective algorithms.
* false - disables the device override for `MPI_Gatherv_init`.

Used in functions: MPI_Gatherv_init

**MPIR_CVAR_GATHERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Gatherv algorithm. It is used for `MPI_Gatherv` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* linear - Forces the linear Gatherv algorithm.
* nb - Forces the nonblocking Gatherv algorithm.

Used in functions: MPI_Gatherv

**MPIR_CVAR_GATHERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Gatherv algorithm. It is used for `MPI_Gatherv` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* linear - Forces the linear Gatherv algorithm.
* nb - Forces the nonblocking Gatherv algorithm.

Used in functions: MPI_Gatherv

**MPIR_CVAR_GATHER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Gather` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Gather.

* true - default, allows the device to override MPIR-level `MPI_Gather` collective algorithms.
* false - disables the device override for `MPI_Gather`.

Used in functions: MPI_Gather

**MPIR_CVAR_GATHER_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Gather_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Gather_init.

* true - default, allows the device to override MPIR-level `MPI_Gather_init` collective algorithms.
* false - disables the device override for `MPI_Gather_init`.

Used in functions: MPI_Gather_init

**MPIR_CVAR_GATHER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Gather algorithm. It is used for `MPI_Gather` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* linear - Forces the linear Gather algorithm.
* local_gather_remote_send - Forces the local-gather-remote-send Gather algorithm.
* nb - Forces the nonblocking Gather algorithm.

Used in functions: MPI_Gather

**MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE** - COLLECTIVE

Sets the short-message threshold, in bytes, used by sched-auto nonblocking inter-communicator Gather. It is used when the sched-based inter-communicator Igather automatic path is selected, such as by forcing `MPIR_CVAR_IGATHER_INTER_ALGORITHM` to `sched_auto` or by internal collective selection. The total message size is computed from the receive datatype, receive count, and remote group size on the root side, or from the send datatype, send count, and local group size on the remote side; messages below this threshold use the short inter-communicator Gather schedule, and other messages use the long inter-communicator Gather schedule.

* integer - default is 2048, represents the inter-communicator Gather short-message threshold in bytes.

Used in functions: MPIR_Igather_inter_sched_auto

**MPIR_CVAR_GATHER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Gather algorithm. It is used for `MPI_Gather` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. The selected Gather implementation may also use `MPIR_CVAR_GATHER_VSMALL_MSG_SIZE` for its very-small-message threshold.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* binomial - Forces the binomial Gather algorithm.
* nb - Forces the nonblocking Gather algorithm.

Used in functions: MPI_Gather

**MPIR_CVAR_GATHER_VSMALL_MSG_SIZE** - COLLECTIVE

Sets the very-small-message threshold, in bytes, used by the intra-communicator binomial Gather algorithms. It is used only when the blocking binomial Gather implementation or sched-based nonblocking binomial Igather implementation is selected, such as by forcing `MPIR_CVAR_GATHER_INTRA_ALGORITHM` to `binomial`, forcing `MPIR_CVAR_IGATHER_INTRA_ALGORITHM` to `sched_binomial`, or by internal collective selection. When active, messages below this threshold use a temporary contiguous buffer for the local contribution and gathered subtree data, allowing nonzero-root gathers to reorder the data before copying it into the receive buffer; messages at or above this threshold use the non-very-small binomial path.

* integer less than or equal to 0 - Disables the very-small-message temporary-buffer path.
* positive integer - default is 1024, represents the very-small-message threshold in bytes.

Used in functions: MPIR_Gather_intra_binomial, MPIR_Igather_intra_sched_binomial

**MPIR_CVAR_GENQ_SHMEM_POOL_FREE_QUEUE_SENDER_SIDE** - CH4

Controls which genq shared-memory cell pool supplies cells when a cell is removed from a free queue and passed to another process. It is used by the genq shared-memory pool path when choosing between sender-side and receiver-side cells; the reviewed source does not show it as an alternate name for another CVAR or as depending on another CVAR. Receiver-side cells are described as useful with the "avx" fast configure option, which can use AVX streaming copy intrinsics when available to avoid polluting the sender's cache, but sender-side queues remain the default until the performance impact is verified. When active, this CVAR sets which side's pool supplies cells for genq shared-memory transfers.

* true - default, use sender-side cells and an MPSC lock for the free queue.
* false - use receiver-side cells and an MPMC lock for the free queue.

Used in functions: none directly visible in `src/mpid/common/genq/mpidu_genq_shmem_pool.c`

**MPIR_CVAR_GPU_DOUBLE_SUPPORT** - COLLECTIVE

Controls whether yaksa-based reductions are allowed for double-precision floating-point datatypes on GPU. It is used when MPICH checks whether a reduction operation can use yaksa; this check first requires `MPIR_CVAR_ENABLE_YAKSA_REDUCTION` to be enabled and, for nonzero counts, the packed data size must not exceed a positive `MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD`. When active, this CVAR enables double-precision floating-point reductions to be accepted for the yaksa reduction path when the remaining operation and datatype support checks pass.

* 0 - default, treat double-precision floating-point reductions as unsupported by yaksa on GPU.
* nonzero - allow double-precision floating-point reductions to use yaksa on GPU when the other yaksa reduction support checks pass.

Used in functions: MPIR_Typerep_reduce_is_supported

**MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE** - CH4

Sets the maximum message or window-buffer size for using the GPU fast-copy path where this general threshold applies. It is used for CH4 GPU IPC paths only when the GPU IPC shmmod is enabled, and the mmap-based fast-copy mapping is selected only in ZE builds. It is also used by GPU localcopy for contiguous copies when the copy direction is not covered by the H2D, D2H, or D2D fast-copy thresholds. GPU IPC use still depends on the relevant IPC path being active, including GPU IPC buffer eligibility and any collective IPC size threshold for collective IPC read paths. When active, this CVAR sets the maximum size for selecting GPU fast memcpy or mmap-based GPU IPC fast-copy mapping in those paths.

* 4096 - default, use GPU fast memcpy or mmap-based GPU IPC fast-copy mapping for applicable copies or mappings up to 4096 bytes.
* nonnegative integer - maximum applicable copy or mapping size in bytes for selecting the GPU fast-copy path.
* negative integer - do not select the GPU fast-copy path for positive-size copies or mappings through this threshold.

Used in functions: MPIDI_GPU_copy_data_async, MPIDI_GPU_write_data_async, MPIDI_IPC_mpi_win_create_hook, MPIDI_POSIX_mpi_bcast_gpu_ipc_read, do_localcopy_gpu

**MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE_D2D** - CH4

Sets the maximum device-to-device copy size for using the GPU fast-copy path. It is used by GPU localcopy for contiguous copies when GPU support is compiled in, the GPU engine type is available, the ZE GPU backend fast-copy implementation is available, and the copy direction is device-to-device. For larger eligible device-to-device copies, GPU localcopy uses the GPU asynchronous memcpy path instead. When active, this CVAR sets the maximum size for selecting GPU fast memcpy for device-to-device localcopy operations.

* 128 - default, use GPU fast memcpy for applicable device-to-device localcopy operations up to 128 bytes.
* nonnegative integer - maximum device-to-device localcopy size in bytes for selecting GPU fast memcpy.
* negative integer - do not select GPU fast memcpy for positive-size device-to-device localcopy operations through this threshold.

Used in functions: do_localcopy_gpu

**MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE_D2H** - CH4

Sets the maximum device-to-host send-copy size for using the GPU fast-copy path. It is used by GPU localcopy for contiguous copies when GPU support is compiled in, the GPU engine type is available, the ZE GPU backend fast-copy implementation is available, and the copy direction is device-to-host. For larger eligible device-to-host copies, GPU localcopy uses the GPU asynchronous memcpy path instead. When active, this CVAR sets the maximum size for selecting GPU fast memcpy for device-to-host localcopy operations.

* 256 - default, use GPU fast memcpy for applicable device-to-host localcopy operations up to 256 bytes.
* nonnegative integer - maximum device-to-host localcopy size in bytes for selecting GPU fast memcpy.
* negative integer - do not select GPU fast memcpy for positive-size device-to-host localcopy operations through this threshold.

Used in functions: do_localcopy_gpu

**MPIR_CVAR_GPU_FAST_COPY_MAX_SIZE_H2D** - CH4

Sets the maximum host-to-device receive-copy size for using the GPU fast-copy path. It is used by GPU localcopy for contiguous copies when GPU support is compiled in, the GPU engine type is available, the ZE GPU backend fast-copy implementation is available, and the copy direction is host-to-device. For larger eligible host-to-device copies, GPU localcopy uses the GPU asynchronous memcpy path instead. When active, this CVAR sets the maximum size for selecting GPU fast memcpy for host-to-device localcopy operations.

* 4096 - default, use GPU fast memcpy for applicable host-to-device localcopy operations up to 4096 bytes.
* nonnegative integer - maximum host-to-device localcopy size in bytes for selecting GPU fast memcpy.
* negative integer - do not select GPU fast memcpy for positive-size host-to-device localcopy operations through this threshold.

Used in functions: do_localcopy_gpu

**MPIR_CVAR_GPU_HAS_WAIT_KERNEL** - GPU

Tells Yaksa during typerep initialization that GPU wait kernels are in use. It is only passed to Yaksa when `MPIR_CVAR_ENABLE_GPU` is enabled; otherwise MPICH initializes Yaksa with GPU support disabled and this CVAR has no effect. When active, it sets the Yaksa `yaksa_has_wait_kernel` info key so temporary-buffer handling can avoid GPU-registered host buffers that may deadlock with stream work queues and GPU wait kernels.

* 0 - default, does not set the Yaksa wait-kernel info key.
* 1 - sets the Yaksa wait-kernel info key.

Used in functions: MPIR_Typerep_init

**MPIR_CVAR_GPU_LONG_DOUBLE_SUPPORT** - COLLECTIVE

Controls whether yaksa-based reductions are allowed for floating-point datatypes larger than double precision on GPU. It is used when MPICH checks whether a reduction operation can use yaksa; this check first requires `MPIR_CVAR_ENABLE_YAKSA_REDUCTION` to be enabled and, for nonzero counts, the packed data size must not exceed a positive `MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD`. When active, this CVAR enables long-double-sized floating-point reductions to be accepted for the yaksa reduction path when the remaining operation and datatype support checks pass.

* 0 - default, treat floating-point reductions larger than double precision as unsupported by yaksa on GPU.
* nonzero - allow floating-point reductions larger than double precision to use yaksa on GPU when the other yaksa reduction support checks pass.

Used in functions: MPIR_Typerep_reduce_is_supported

**MPIR_CVAR_GPU_ROUND_ROBIN_COMMAND_QUEUES** - GPU

Controls whether the GPU backend is requested to use command queues in a round-robin fashion. It is used during GPU initialization only when `MPIR_CVAR_ENABLE_GPU` is enabled; the selected value is stored in `MPL_gpu_info.roundrobin_cmdq` before MPL GPU initialization. When active, this CVAR enables round-robin use of GPU command queues.

* false - default, use only command queues of index 0.
* true - use command queues in a round-robin fashion.

Used in functions: MPII_init_gpu

**MPIR_CVAR_GPU_USE_IMMEDIATE_COMMAND_LIST** - GPU

Controls whether the GPU backend is requested to use immediate command lists for copying. It is used during GPU initialization only when `MPIR_CVAR_ENABLE_GPU` is enabled; the selected value is stored in `MPL_gpu_info.use_immediate_cmdlist` before MPL GPU initialization. When active, this CVAR enables use of immediate command lists for GPU copy operations.

* false - default, do not request immediate command lists for GPU copy operations.
* true - request immediate command lists for GPU copy operations.

Used in functions: MPII_init_gpu

**MPIR_CVAR_HIERARCHY_DUMP** - COLLECTIVE

Enables dumping the topology-aware hierarchy data structure for each rank to `hierarchy[rank]` in the current folder when topology hierarchy population exits. It is used by topology-aware and topology-wave MPIR tree initialization paths that build the hierarchy. Hierarchy construction relies on topology coordinates being available through `MPIR_CVAR_COORDINATES_FILE`; if hierarchy construction cannot use those coordinates, the topology-aware paths fall back to kary tree building after this dump point.

* false - default, disables hierarchy dumping.
* true - enables hierarchy dumping for each rank.

Used in functions: MPII_Treeutil_hierarchy_populate

**MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL** - COLLECTIVE

Sets the radix value for the generic transport Brucks-based Iallgatherv algorithm. It is used when the Brucks-based generic transport Iallgatherv path is selected, such as by forcing `MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM` to `tsp_brucks` or by internal collective selection. When active, it controls the radix used by the Brucks-based Iallgatherv implementation.

* integer - default is 2, represents the radix value used by the Brucks-based Iallgatherv algorithm.

Used in functions: MPIR_TSP_Iallgatherv_sched_intra_brucks

**MPIR_CVAR_IALLGATHERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Iallgatherv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Iallgatherv.

* true - default, allows the device to override MPIR-level `MPI_Iallgatherv` collective algorithms.
* false - disables the device override for `MPI_Iallgatherv`.

Used in functions: MPI_Iallgatherv

**MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Allgatherv algorithm. It is used for Iallgatherv on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based selection or forces a sched-based inter-communicator implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_remote_gather_local_bcast - Forces the sched-based remote-gather-local-bcast algorithm.

Used in functions: MPIR_Iallgatherv_inter_sched_auto, MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast

**MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Allgatherv algorithm. Its enum values are also reused by the TSP Ibcast scatterv/allgatherv algorithms to select the Allgatherv phase after the scatter phase. When the TSP Ibcast scatterv/allgatherv paths are active, that choice is made through `MPIR_CVAR_IBCAST_INTRA_ALGORITHM` or collective selection rather than by reading this CVAR directly; those paths also use the Ibcast scatterv and Allgatherv recursive-exchange k-value CVARs for their tree and recursive-exchange branching factors. The recursive-exchange TSP Iallgatherv algorithms require ordered displacements, and their radix is controlled by `MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL`; the TSP Brucks algorithm uses `MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_brucks - Forces the sched-based Brucks algorithm.
* sched_recursive_doubling - Forces the sched-based recursive-doubling algorithm.
* sched_ring - Forces the sched-based ring algorithm.
* tsp_recexch_doubling - Forces the generic transport recursive-exchange algorithm with neighbors doubling in distance in each phase.
* tsp_recexch_halving - Forces the generic transport recursive-exchange algorithm with neighbors halving in distance in each phase.
* tsp_ring - Forces the generic transport ring algorithm.
* tsp_brucks - Forces the generic transport Brucks-based algorithm.

Used in functions: MPIR_Iallgatherv_intra_sched_brucks, MPIR_Iallgatherv_intra_sched_recursive_doubling, MPIR_Iallgatherv_intra_sched_ring, MPIR_TSP_Iallgatherv_sched_intra_recexch, MPIR_TSP_Iallgatherv_sched_intra_ring, MPIR_TSP_Iallgatherv_sched_intra_brucks, MPIR_TSP_Ibcast_sched_intra_scatterv_ring_allgatherv, MPIR_Ibcast_sched_intra_tsp_flat_auto, MPIR_TSP_Ibcast_sched_intra_tsp_auto, MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv

**MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for recursive-exchange Iallgatherv algorithms. It is used when a recursive-exchange Iallgatherv path is selected, such as by forcing `MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM` to `tsp_recexch_doubling` or `tsp_recexch_halving`, or by internal collective selection. When active, it controls the k value used by the recursive-exchange Iallgatherv implementation.

* integer - default is 2, represents the k value used by recursive-exchange Iallgatherv algorithms.

Used in functions: MPIR_TSP_Iallgatherv_sched_intra_recexch

**MPIR_CVAR_IALLGATHER_BRUCKS_KVAL** - COLLECTIVE

Sets the radix value for the generic transport Brucks-based Iallgather algorithm. It is used when the Brucks-based generic transport Iallgather path is selected, such as by forcing `MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM` to `tsp_brucks` or by internal collective selection. When active, it controls the radix used by the Brucks-based Iallgather implementation.

* integer - default is 2, represents the radix value used by the Brucks-based Iallgather algorithm.

Used in functions: MPI_Iallgather

**MPIR_CVAR_IALLGATHER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Iallgather` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Iallgather.

* true - default, allows the device to override MPIR-level `MPI_Iallgather` collective algorithms.
* false - disables the device override for `MPI_Iallgather`.

Used in functions: MPI_Iallgather

**MPIR_CVAR_IALLGATHER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Iallgather algorithm. It is used for `MPI_Iallgather` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; the available inter-communicator paths are sched-based and do not rely on the Iallgather k-value CVARs.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_local_gather_remote_bcast - forces the local-gather-remote-bcast algorithm.

Used in functions: MPI_Iallgather

**MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Iallgather algorithm. Automatic selection may be overridden by `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; selected generic transport recursive-exchange and Brucks paths use the Iallgather k-value CVARs. When active, this CVAR sets which Iallgather implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden by a collective selection tuning JSON file.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_ring - forces the ring algorithm.
* sched_brucks - forces the Brucks algorithm.
* sched_recursive_doubling - forces the recursive-doubling algorithm.
* tsp_ring - forces the generic transport ring algorithm.
* tsp_brucks - forces the generic transport Brucks algorithm.
* tsp_recexch_doubling - forces generic transport recursive exchange with neighbors doubling in distance in each phase.
* tsp_recexch_halving - forces generic transport recursive exchange with neighbors halving in distance in each phase.

Used in functions: MPI_Iallgather

**MPIR_CVAR_IALLGATHER_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for recursive-exchange Iallgather algorithms. It is used when a generic transport recursive-exchange Iallgather path is selected, such as by forcing `MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM` to `tsp_recexch_doubling` or `tsp_recexch_halving`, or by internal collective selection. When active, it controls the k value used by the recursive-exchange Iallgather implementation.

* integer - default is 2, represents the k value used by recursive-exchange Iallgather algorithms.

Used in functions: MPI_Iallgather

**MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Iallreduce` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Iallreduce.

* true - default, allows the device to override MPIR-level `MPI_Iallreduce` collective algorithms.
* false - disables the device override for `MPI_Iallreduce`.

Used in functions: MPI_Iallreduce

**MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Iallreduce algorithm. It is used for Iallreduce on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which inter-communicator Iallreduce implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_remote_reduce_local_bcast - forces the remote-reduce-local-bcast algorithm.

Used in functions: MPI_Iallreduce

**MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Iallreduce algorithm. It is used by the generic transport scheduler path for intra-communicator Iallreduce when this CVAR directly forces a generic transport algorithm; otherwise, it falls through to internal collective selection, which may be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`, and then to a recursive-exchange fallback if no generic transport container is selected. When active, this CVAR sets which intra-communicator Iallreduce implementation is forced or whether internal collective selection chooses the implementation. The forced recursive-exchange paths use the Iallreduce recursive-exchange k-value CVAR. The forced tree path uses the selected Iallreduce tree type, tree k value, tree pipeline chunk size, and tree buffer-per-child CVARs, and it falls back to a recursive-exchange path if it cannot be applied. The forced ring and reduce-scatter/allgatherv paths also fall back to recursive exchange if their operation or count requirements are not met.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_naive - forces the naive algorithm.
* sched_smp - forces the SMP algorithm.
* sched_recursive_doubling - forces the recursive-doubling algorithm.
* sched_reduce_scatter_allgather - forces the reduce-scatter-allgather algorithm.
* tsp_recexch_single_buffer - forces generic transport recursive exchange with a single buffer for receives.
* tsp_recexch_multiple_buffer - forces generic transport recursive exchange with multiple buffers for receives.
* tsp_tree - forces the generic transport tree algorithm.
* tsp_ring - forces the generic transport ring algorithm.
* tsp_recexch_reduce_scatter_recexch_allgatherv - forces generic transport recursive exchange with reduce-scatter and allgatherv.

Used in functions: MPIR_TSP_Iallreduce_sched_intra_tsp_auto

**MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for generic transport recursive-exchange Iallreduce algorithms. It is used when `MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM` forces `tsp_recexch_single_buffer`, `tsp_recexch_multiple_buffer`, or `tsp_recexch_reduce_scatter_recexch_allgatherv`; the reduce-scatter/allgatherv path is applied only for commutative operations with count at least the communicator size, otherwise it falls back to a recursive-exchange Iallreduce path. It is also used by the fallback path when no generic transport collective-selection container is selected, and when the forced tree path cannot be applied because the operation is non-commutative and the selected tree type is not knomial_1. When automatic collective selection chooses a recursive-exchange Iallreduce path, the selected container may supply the k value instead of this CVAR.

* integer - default is 2, represents the k value used by generic transport recursive-exchange Iallreduce algorithms.

Used in functions: MPIR_TSP_Iallreduce_sched_intra_tsp_auto

**MPIR_CVAR_IALLREDUCE_TREE_BUFFER_PER_CHILD** - COLLECTIVE

Controls whether the generic transport tree-based intra-communicator Iallreduce algorithm allocates a dedicated receive buffer for each child or reuses one receive buffer across children. It is used when `MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM` forces `tsp_tree`; that path also uses the selected Iallreduce tree type, tree k value, and tree pipeline chunk size. The forced tree path is applied only for commutative operations or when the selected tree type is knomial_1; otherwise it falls back to a recursive-exchange Iallreduce path. When automatic collective selection chooses the TSP tree Iallreduce path, the selected container may supply the buffer-per-child setting instead of this CVAR.

* 0 - default, uses one receive buffer for data from all children, serializing receives so only one receive is posted at a time.
* 1 - allocates a dedicated receive buffer for each child, allowing receives to be preposted and reducing unexpected messages at the cost of higher memory use.

Used in functions: MPIR_TSP_Iallreduce_sched_intra_tsp_auto

**MPIR_CVAR_IALLREDUCE_TREE_KVAL** - COLLECTIVE

Sets the k value for the generic transport tree-based intra-communicator Iallreduce algorithm. It is used when `MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM` forces `tsp_tree`; that path also uses the selected Iallreduce tree type, tree pipeline chunk size, and tree buffer-per-child CVARs. The forced tree path is applied only for commutative operations or when the selected tree type is knomial_1; otherwise it falls back to a recursive-exchange Iallreduce path. When automatic collective selection chooses the TSP tree Iallreduce path, the selected container may supply the k value instead of this CVAR.

* integer - default is 2, represents the k value used by the generic transport tree-based Iallreduce algorithm.

Used in functions: MPIR_TSP_Iallreduce_sched_intra_tsp_auto

**MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE** - COLLECTIVE

Sets the maximum pipeline chunk size, in bytes, for the generic transport tree-based intra-communicator Iallreduce algorithm. It is used when `MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM` forces `tsp_tree`; that path also uses the selected Iallreduce tree type, tree k value, and buffer-per-child CVARs. The forced tree path is applied only for commutative operations or when the selected tree type is knomial_1; otherwise it falls back to a recursive-exchange Iallreduce path. When automatic collective selection chooses the TSP tree Iallreduce path, the selected container may supply the chunk size instead of this CVAR.

* 0 - default, disables pipelining in tree-based Iallreduce.
* positive integer - represents the maximum pipeline chunk size in bytes for tree-based Iallreduce.

Used in functions: MPIR_TSP_Iallreduce_sched_intra_tsp_auto

**MPIR_CVAR_IALLREDUCE_TREE_TYPE** - COLLECTIVE

Sets the tree type for the generic transport tree-based intra-communicator Iallreduce algorithm. It is initialized during collective initialization and is used when `MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM` forces `tsp_tree`; that path also uses the Iallreduce tree k value, pipeline chunk size, and buffer-per-child CVARs. The forced tree path is applied only for commutative operations or when the selected tree type permits non-commutative operations; otherwise it falls back to a recursive-exchange Iallreduce path. When automatic collective selection chooses the TSP tree Iallreduce path, the selected container may supply the tree type instead of this CVAR.

* kary - default, represents a k-ary tree type.
* knomial_1 - represents a knomial_1 tree type.
* knomial_2 - represents a knomial_2 tree type.

Used in functions: MPII_Coll_init, MPIR_TSP_Iallreduce_sched_intra_tsp_auto

**MPIR_CVAR_IALLTOALLV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ialltoallv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ialltoallv.

* true - default, allows the device to override MPIR-level `MPI_Ialltoallv` collective algorithms.
* false - disables the device override for `MPI_Ialltoallv`.

Used in functions: MPI_Ialltoallv

**MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Alltoallv algorithm. It is used for `MPI_Ialltoallv` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based automatic selection or a specific pairwise-exchange Ialltoallv implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_pairwise_exchange - Forces the sched-based pairwise-exchange algorithm.

Used in functions: MPI_Ialltoallv

**MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Alltoallv algorithm. It is used for `MPI_Ialltoallv` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; selected scattered paths may also use `MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS` and `MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE`, and selected blocked paths may also use Alltoall-family throttling. When active, this CVAR sets which Ialltoallv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_blocked - Forces the sched-based blocked algorithm.
* sched_inplace - Forces the sched-based inplace algorithm.
* tsp_scattered - Forces the generic transport based scattered algorithm.
* tsp_blocked - Forces the generic transport blocked algorithm.
* tsp_inplace - Forces the generic transport inplace algorithm.

Used in functions: MPI_Ialltoallv

**MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE** - COLLECTIVE

Sets the number of send and receive task completions the generic transport scattered Ialltoallv algorithm waits for before posting another batch of send and receive tasks of that size. It is used only when the intra-communicator Ialltoallv scattered TSP path is selected, such as by forcing `MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM` to `tsp_scattered` or by internal collective selection. The scattered path also uses `MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS` to limit how many send and receive tasks may be outstanding at a time.

* integer - default is 4, represents the number of completed send and receive tasks waited for before another batch is posted.

Used in functions: MPI_Ialltoallv

**MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS** - COLLECTIVE

Sets the maximum number of outstanding send and receive tasks posted at a time by the generic transport scattered Ialltoallv algorithm. It is used only when the intra-communicator Ialltoallv scattered TSP path is selected, such as by forcing `MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM` to `tsp_scattered` or by internal collective selection. The scattered path also uses `MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE` to determine how many completed tasks to wait for before posting another batch.

* integer - default is 64, represents the maximum number of outstanding sends and receives posted at a time.

Used in functions: MPI_Ialltoallv

**MPIR_CVAR_IALLTOALLW_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ialltoallw` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ialltoallw.

* true - default, allows the device to override MPIR-level `MPI_Ialltoallw` collective algorithms.
* false - disables the device override for `MPI_Ialltoallw`.

Used in functions: MPI_Ialltoallw

**MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Alltoallw algorithm. It is used for `MPI_Ialltoallw` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based automatic selection or a specific pairwise-exchange Ialltoallw implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_pairwise_exchange - Forces the sched-based pairwise-exchange algorithm.

Used in functions: MPI_Ialltoallw

**MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Alltoallw algorithm. It is used for `MPI_Ialltoallw` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Ialltoallw implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_blocked - Forces the sched-based blocked algorithm.
* sched_inplace - Forces the sched-based inplace algorithm.
* tsp_blocked - Forces the generic transport based blocked algorithm.
* tsp_inplace - Forces the generic transport based inplace algorithm.

Used in functions: MPI_Ialltoallw

**MPIR_CVAR_IALLTOALL_BRUCKS_BUFFER_PER_NBR** - COLLECTIVE

Controls whether the generic transport Brucks-based intra-communicator Ialltoall algorithm uses dedicated temporary send and receive buffers for each neighbor in each Brucks phase. It is used only when the generic transport Brucks Ialltoall path is selected, such as by forcing `MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM` to `tsp_brucks` or by internal collective selection. The Brucks path also uses `MPIR_CVAR_IALLTOALL_BRUCKS_KVAL` to determine the radix and number of neighbors considered in each phase. When enabled, this CVAR sets the buffer allocation strategy to use separate buffers per neighbor per phase instead of reusing buffers across phases with additional task dependencies.

* false - default, reuses one send buffer and one receive buffer per neighbor across Brucks phases.
* true - allocates dedicated send and receive buffers for each neighbor in each Brucks phase.

Used in functions: MPIR_TSP_Ialltoall_sched_intra_brucks

**MPIR_CVAR_IALLTOALL_BRUCKS_KVAL** - COLLECTIVE

Sets the radix for the generic transport Brucks-based intra-communicator Ialltoall algorithm. It is used only when the generic transport Brucks Ialltoall path is selected, such as by forcing `MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM` to `tsp_brucks` or by internal collective selection. When active, it controls the base used to split Ialltoall data movement into phases and determines how many nonzero digit exchanges are attempted in each phase.

* integer 2 or greater - default is 2, represents the radix used by the generic transport Brucks Ialltoall algorithm.

Used in functions: MPIR_TSP_Ialltoall_sched_intra_brucks

**MPIR_CVAR_IALLTOALL_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ialltoall` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ialltoall.

* true - default, allows the device to override MPIR-level `MPI_Ialltoall` collective algorithms.
* false - disables the device override for `MPI_Ialltoall`.

Used in functions: MPI_Ialltoall

**MPIR_CVAR_IALLTOALL_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Ialltoall algorithm. It is used for `MPI_Ialltoall` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based automatic selection or a specific pairwise-exchange Ialltoall implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_pairwise_exchange - Forces the sched-based pairwise-exchange algorithm.

Used in functions: MPI_Ialltoall

**MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ialltoall algorithm. It is used for `MPI_Ialltoall` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based automatic selection may also use the Alltoall short-message and medium-message thresholds.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_brucks - Forces the Brucks algorithm.
* sched_inplace - Forces the inplace algorithm.
* sched_pairwise - Forces the pairwise algorithm.
* sched_permuted_sendrecv - Forces the permuted-sendrecv algorithm.
* tsp_ring - Forces the generic transport based ring algorithm.
* tsp_brucks - Forces the generic transport based Brucks algorithm.
* tsp_scattered - Forces the generic transport based scattered algorithm.

Used in functions: MPI_Ialltoall

**MPIR_CVAR_IALLTOALL_SCATTERED_BATCH_SIZE** - COLLECTIVE

Sets how many send and receive task pairs the generic transport scattered Ialltoall algorithm waits to complete before posting another group of send and receive tasks. It is used only when the intra-communicator Ialltoall scattered TSP path is selected, such as by forcing `MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM` to `tsp_scattered` or by internal collective selection. The scattered path also uses `MPIR_CVAR_IALLTOALL_SCATTERED_OUTSTANDING_TASKS` to set the maximum number of outstanding send and receive tasks.

* integer - default is 4, represents the number of send and receive task pairs to wait for before posting another group of that size.

Used in functions: MPIR_TSP_Ialltoall_sched_intra_scattered

**MPIR_CVAR_IALLTOALL_SCATTERED_OUTSTANDING_TASKS** - COLLECTIVE

Sets the maximum number of outstanding send and receive tasks posted at a time by the generic transport scattered Ialltoall algorithm. It is used only when the intra-communicator Ialltoall scattered TSP path is selected, such as by forcing `MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM` to `tsp_scattered` or by internal collective selection. The scattered path also uses `MPIR_CVAR_IALLTOALL_SCATTERED_BATCH_SIZE` to determine how many completed tasks to wait for before posting another batch.

* integer - default is 64, represents the maximum number of outstanding sends and receives posted at a time.

Used in functions: MPIR_TSP_Ialltoall_sched_intra_scattered

**MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ibarrier` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ibarrier.

* true - default, allows the device to override MPIR-level `MPI_Ibarrier` collective algorithms.
* false - disables the device override for `MPI_Ibarrier`.

Used in functions: MPI_Ibarrier

**MPIR_CVAR_IBARRIER_DISSEM_KVAL** - COLLECTIVE

Sets the k value for the generic transport high-radix dissemination intra-communicator Ibarrier algorithm. It is used when `MPIR_CVAR_IBARRIER_INTRA_ALGORITHM` forces `tsp_k_dissemination`; when automatic collective selection chooses the k-dissemination Ibarrier path, the selected container supplies the k value instead of this CVAR.

* integer - default is 2, represents the k value used by the generic transport high-radix dissemination Ibarrier algorithm.

Used in functions: MPIR_TSP_Ibarrier_sched_intra_tsp_auto

**MPIR_CVAR_IBARRIER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Barrier algorithm. It is used for `MPI_Ibarrier` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based automatic selection is limited to sched-based algorithms.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_bcast - Forces the sched-based Bcast algorithm.

Used in functions: MPI_Ibarrier

**MPIR_CVAR_IBARRIER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Barrier algorithm for sched-based TSP Ibarrier execution. It is used by the TSP auto Ibarrier scheduler on intra-communicators. When it forces a TSP algorithm, the selected implementation uses the corresponding Ibarrier k-value CVAR for its radix. Otherwise, internal collective selection chooses the TSP Ibarrier algorithm and supplies any selected algorithm parameters; if no matching TSP selection is available, the scheduler falls back to a recursive-exchange allreduce-based barrier schedule.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_recursive_doubling - Forces the sched-based recursive-doubling algorithm.
* tsp_recexch - Forces the generic transport recursive-exchange algorithm.
* tsp_k_dissemination - Forces the generic transport high-radix dissemination algorithm.

Used in functions: MPIR_TSP_Ibarrier_sched_intra_tsp_auto

**MPIR_CVAR_IBARRIER_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for the generic transport recursive-exchange intra-communicator Ibarrier algorithm. It is used when `MPIR_CVAR_IBARRIER_INTRA_ALGORITHM` forces `tsp_recexch`; when automatic collective selection chooses the recursive-exchange Ibarrier path, the selected container supplies the k value instead of this CVAR.

* integer - default is 2, represents the k value used by the generic transport recursive-exchange Ibarrier algorithm.

Used in functions: MPIR_TSP_Ibarrier_sched_intra_tsp_auto

**MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for the recursive-exchange Allgatherv phase of the generic transport intra-communicator Ibcast scatterv followed by recursive-exchange Allgatherv algorithm. It is used when `MPIR_CVAR_IBCAST_INTRA_ALGORITHM` forces `tsp_scatterv_recexch_allgatherv`; when automatic collective selection chooses that TSP Ibcast path, the selected container supplies the Allgatherv k value instead of this CVAR. The selected path also uses `MPIR_CVAR_IBCAST_SCATTERV_KVAL` for the scatterv phase.

* integer - default is 2, represents the k value used by the recursive-exchange Allgatherv phase of the generic transport Ibcast scatterv plus recursive-exchange Allgatherv algorithm.

Used in functions: MPIR_TSP_Ibcast_sched_intra_tsp_auto

**MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ibcast` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ibcast.

* true - default, allows the device to override MPIR-level `MPI_Ibcast` collective algorithms.
* false - disables the device override for `MPI_Ibcast`.

Used in functions: MPI_Ibcast

**MPIR_CVAR_IBCAST_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Bcast algorithm. It is used for Ibcast operations on inter-communicators. Internal algorithm selection can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses internal selection or a specific sched-based inter-communicator Ibcast implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_flat - Forces the sched-based flat algorithm.

Used in functions: MPIR_Ibcast_inter_sched_auto

**MPIR_CVAR_IBCAST_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Bcast algorithm for generic transport TSP Ibcast execution. It is used by the TSP auto Ibcast scheduler on intra-communicators. When it forces a TSP algorithm, the selected implementation may use the Ibcast tree type, tree k-value, tree pipeline chunk size, ring chunk size, scatterv k-value, or Allgatherv recursive-exchange k-value CVARs for its parameters. Otherwise, internal collective selection chooses the TSP Ibcast algorithm and supplies any selected algorithm parameters; if no matching TSP selection is available, the scheduler falls back to a flat auto-selection path that uses the Bcast short-message and minimum-process CVARs.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_binomial - Forces the sched-based Binomial algorithm.
* sched_smp - Forces the sched-based SMP algorithm.
* sched_scatter_recursive_doubling_allgather - Forces the sched-based Scatter Recursive Doubling Allgather algorithm.
* sched_scatter_ring_allgather - Forces the sched-based Scatter Ring Allgather algorithm.
* tsp_tree - Forces the generic transport Tree algorithm.
* tsp_scatterv_recexch_allgatherv - Forces the generic transport Scatterv followed by Recursive Exchange Allgatherv algorithm.
* tsp_scatterv_ring_allgatherv - Forces the generic transport Scatterv followed by Ring Allgatherv algorithm.
* tsp_ring - Forces the generic transport Ring algorithm.
* circ_graph - Forces the queued circulant graph algorithm.

Used in functions: MPIR_TSP_Ibcast_sched_intra_tsp_auto

**MPIR_CVAR_IBCAST_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Defines the algorithm selector for POSIX shared-memory intranode nonblocking broadcast. In `src/mpid/ch4/shm/posix/posix_coll.h`, the POSIX nonblocking broadcast implementation does not read this CVAR and instead calls the MPIR nonblocking broadcast implementation directly. No alternate CVAR name, dependency on another CVAR, or active mode requirement is visible in this file. As implemented in this file, this CVAR does not enable or set POSIX nonblocking broadcast behavior.

* auto - default, represent internal POSIX collective selection, which the CVAR description says can be overridden by `MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE`.
* mpir - represent fallback to MPIR collectives.
* release_gather - represent the POSIX shared-memory release-gather path.

Used in functions: none in `src/mpid/ch4/shm/posix/posix_coll.h`

**MPIR_CVAR_IBCAST_RING_CHUNK_SIZE** - COLLECTIVE

Sets the maximum chunk size, in bytes, for pipelining in the generic transport ring intra-communicator Ibcast algorithm. It is used when `MPIR_CVAR_IBCAST_INTRA_ALGORITHM` forces `tsp_ring`; when automatic collective selection chooses the TSP ring Ibcast path, the selected container supplies the chunk size instead of this CVAR. When active, the ring schedule is created through the generic transport tree scheduler with a k-ary tree type and radix 1.

* 0 - default, disables pipelining in ring Ibcast.
* positive integer - represents the maximum chunk size in bytes for pipelining in ring Ibcast.

Used in functions: MPIR_TSP_Ibcast_sched_intra_tsp_auto

**MPIR_CVAR_IBCAST_SCATTERV_KVAL** - COLLECTIVE

Sets the k value for the scatterv phase of the generic transport intra-communicator Ibcast scatterv followed by recursive-exchange Allgatherv algorithm. It is used when `MPIR_CVAR_IBCAST_INTRA_ALGORITHM` forces `tsp_scatterv_recexch_allgatherv`; when automatic collective selection chooses that TSP Ibcast path, the selected container supplies the scatterv k value instead of this CVAR. The selected path also uses `MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL` for the recursive-exchange Allgatherv phase.

* integer - default is 2, represents the k value used by the scatterv phase of the generic transport Ibcast scatterv plus recursive-exchange Allgatherv algorithm.

Used in functions: MPIR_TSP_Ibcast_sched_intra_tsp_auto

**MPIR_CVAR_IBCAST_TREE_KVAL** - COLLECTIVE

Sets the k value for the generic transport tree-based intra-communicator Ibcast algorithm. It is used when `MPIR_CVAR_IBCAST_INTRA_ALGORITHM` forces `tsp_tree`; when automatic collective selection chooses the TSP tree Ibcast path, the selected container supplies the k value instead of this CVAR. When active, the tree schedule also uses the Ibcast tree type from `MPIR_CVAR_IBCAST_TREE_TYPE` and the pipeline chunk size from `MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE`.

* integer - default is 2, represents the k value used by the generic transport tree-based Ibcast algorithm.

Used in functions: MPIR_TSP_Ibcast_sched_intra_tsp_auto

**MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE** - COLLECTIVE

Sets the maximum chunk size, in bytes, for pipelining in the generic transport tree-based intra-communicator Ibcast algorithm. It is used when `MPIR_CVAR_IBCAST_INTRA_ALGORITHM` forces `tsp_tree`; when automatic collective selection chooses the TSP tree Ibcast path, the selected container supplies the chunk size instead of this CVAR. When active, the tree schedule also uses the tree type from `MPIR_CVAR_IBCAST_TREE_TYPE` and the k value from `MPIR_CVAR_IBCAST_TREE_KVAL`.

* 0 - default, disables pipelining in tree-based Ibcast.
* positive integer - represents the maximum chunk size in bytes for pipelining in tree-based Ibcast.

Used in functions: MPIR_TSP_Ibcast_sched_intra_tsp_auto

**MPIR_CVAR_IBCAST_TREE_TYPE** - COLLECTIVE

Sets the tree type used by the generic transport tree-based intra-communicator Ibcast algorithm. It is read during collective initialization and stored as the global Ibcast tree type. It is used when `MPIR_CVAR_IBCAST_INTRA_ALGORITHM` forces `tsp_tree`, and may also be used when internal collective selection chooses the TSP tree Ibcast path. The tree schedule also uses `MPIR_CVAR_IBCAST_TREE_KVAL` for the tree k value and `MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE` for pipeline chunking.

* kary - default, represents a k-ary tree type.
* knomial_1 - represents a knomial tree type.
* knomial_2 - represents a knomial tree type.

Used in functions: MPII_Coll_init

**MPIR_CVAR_IEXSCAN_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Iexscan` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Iexscan.

* true - default, allows the device to override MPIR-level `MPI_Iexscan` collective algorithms.
* false - disables the device override for `MPI_Iexscan`.

Used in functions: MPI_Iexscan

**MPIR_CVAR_IEXSCAN_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Iexscan algorithm. It is used for `MPI_Iexscan` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_IEXSCAN_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Iexscan implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_recursive_doubling - forces the recursive-doubling algorithm.

Used in functions: MPI_Iexscan

**MPIR_CVAR_IGATHERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Igatherv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Igatherv.

* true - default, allows the device to override MPIR-level `MPI_Igatherv` collective algorithms.
* false - disables the device override for `MPI_Igatherv`.

Used in functions: MPI_Igatherv

**MPIR_CVAR_IGATHERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Gatherv algorithm. It is used for `MPI_Igatherv` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based automatic selection or a specific linear Igatherv implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_linear - Forces the sched-based linear algorithm.
* tsp_linear - Forces the generic transport linear algorithm.

Used in functions: MPI_Igatherv

**MPIR_CVAR_IGATHERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Gatherv algorithm. It is used for `MPI_Igatherv` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_linear - Forces the sched-based linear algorithm.
* tsp_linear - Forces the generic transport linear algorithm.

Used in functions: MPI_Igatherv

**MPIR_CVAR_IGATHER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Igather` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Igather.

* true - default, allows the device to override MPIR-level `MPI_Igather` collective algorithms.
* false - disables the device override for `MPI_Igather`.

Used in functions: MPI_Igather

**MPIR_CVAR_IGATHER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Gather algorithm. It is used for `MPI_Igather` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based automatic selection uses `MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE` to choose between the short and long inter-communicator Gather schedules.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_long - Forces the sched-based long inter-communicator Gather algorithm.
* sched_short - Forces the sched-based short inter-communicator Gather algorithm.

Used in functions: MPI_Igather

**MPIR_CVAR_IGATHER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Gather algorithm. It is used for `MPI_Igather` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. The selected implementation may also use `MPIR_CVAR_GATHER_VSMALL_MSG_SIZE` for sched-based binomial Gather behavior or `MPIR_CVAR_IGATHER_TREE_KVAL` for the generic transport tree algorithm.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_binomial - Forces the sched-based binomial algorithm.
* tsp_tree - Forces the generic transport tree algorithm.

Used in functions: MPI_Igather

**MPIR_CVAR_IGATHER_TREE_KVAL** - COLLECTIVE

Sets the k value for the generic transport tree-based intra-communicator Igather algorithm. It is used when `MPIR_CVAR_IGATHER_INTRA_ALGORITHM` forces `tsp_tree`; when automatic collective selection chooses the TSP tree Igather path, the selected container may supply the k value instead of this CVAR.

* integer - default is 2, represents the k value used by the generic transport tree-based Igather algorithm.

Used in functions: MPIR_TSP_Igather_sched_intra_tree

**MPIR_CVAR_INEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ineighbor_allgatherv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ineighbor_allgatherv.

* true - default, allows the device to override MPIR-level `MPI_Ineighbor_allgatherv` collective algorithms.
* false - disables the device override for `MPI_Ineighbor_allgatherv`.

Used in functions: MPI_Ineighbor_allgatherv

**MPIR_CVAR_INEIGHBOR_ALLGATHERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Ineighbor_allgatherv algorithm. It is used for `MPI_Ineighbor_allgatherv` on inter-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_INEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Ineighbor_allgatherv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_linear - forces the linear algorithm.
* tsp_linear - forces the generic transport based linear algorithm.

Used in functions: MPI_Ineighbor_allgatherv

**MPIR_CVAR_INEIGHBOR_ALLGATHERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ineighbor_allgatherv algorithm. It is used for `MPI_Ineighbor_allgatherv` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_INEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Ineighbor_allgatherv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_linear - forces the linear algorithm.
* tsp_linear - forces the generic transport based linear algorithm.

Used in functions: MPI_Ineighbor_allgatherv

**MPIR_CVAR_INEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ineighbor_allgather` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ineighbor_allgather.

* true - default, allows the device to override MPIR-level `MPI_Ineighbor_allgather` collective algorithms.
* false - disables the device override for `MPI_Ineighbor_allgather`.

Used in functions: MPI_Ineighbor_allgather

**MPIR_CVAR_INEIGHBOR_ALLGATHER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Ineighbor_allgather algorithm. It is used for `MPI_Ineighbor_allgather` on inter-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_INEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Ineighbor_allgather implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_linear - forces the linear algorithm.
* tsp_linear - forces the generic transport based linear algorithm.

Used in functions: MPI_Ineighbor_allgather

**MPIR_CVAR_INEIGHBOR_ALLGATHER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ineighbor_allgather algorithm. It is used for `MPI_Ineighbor_allgather` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_INEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Ineighbor_allgather implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_linear - forces the linear algorithm.
* tsp_linear - forces the generic transport based linear algorithm.

Used in functions: MPI_Ineighbor_allgather

**MPIR_CVAR_INEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ineighbor_alltoallv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ineighbor_alltoallv.

* true - default, allows the device to override MPIR-level `MPI_Ineighbor_alltoallv` collective algorithms.
* false - disables the device override for `MPI_Ineighbor_alltoallv`.

Used in functions: MPI_Ineighbor_alltoallv

**MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ineighbor_alltoallv algorithm. It is used for `MPI_Ineighbor_alltoallv` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_INEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Ineighbor_alltoallv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_linear - forces the linear algorithm.
* tsp_linear - forces the generic transport based linear algorithm.

Alternate names: MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTER_ALGORITHM

Used in functions: MPI_Ineighbor_alltoallv

**MPIR_CVAR_INEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ineighbor_alltoallw` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ineighbor_alltoallw.

* true - default, allows the device to override MPIR-level `MPI_Ineighbor_alltoallw` collective algorithms.
* false - disables the device override for `MPI_Ineighbor_alltoallw`.

Used in functions: MPI_Ineighbor_alltoallw

**MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ineighbor_alltoallw algorithm. It is used for `MPI_Ineighbor_alltoallw` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_INEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Ineighbor_alltoallw implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_linear - forces the linear algorithm.
* tsp_linear - forces the generic transport based linear algorithm.

Alternate names: MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM

Used in functions: MPI_Ineighbor_alltoallw

**MPIR_CVAR_INEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ineighbor_alltoall` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ineighbor_alltoall.

* true - default, allows the device to override MPIR-level `MPI_Ineighbor_alltoall` collective algorithms.
* false - disables the device override for `MPI_Ineighbor_alltoall`.

Used in functions: MPI_Ineighbor_alltoall

**MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ineighbor_alltoall algorithm. It is used for `MPI_Ineighbor_alltoall` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_INEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Ineighbor_alltoall implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_linear - forces the linear algorithm.
* tsp_linear - forces the generic transport based linear algorithm.

Alternate names: MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM

Used in functions: MPI_Ineighbor_alltoall

**MPIR_CVAR_INIT_SKIP_PMI_BARRIER** - DEBUGGER

Controls whether MPICH skips the standalone PMI barrier during MPI initialization. It is used after device initialization, where MPICH may otherwise issue a PMI barrier to ensure the debugger process-acquisition mechanism has an opportunity to attach; this is separate from any PMI barrier the device may perform during business-card exchange. It does not rely on another CVAR being enabled, but it is relevant to the MPI initialization path after the device has initialized. When active, this CVAR enables skipping that standalone PMI barrier.

* true - default, skip the standalone PMI barrier during MPI initialization.
* false - issue the standalone PMI barrier during MPI initialization.

Used in functions: MPII_Init_thread

**MPIR_CVAR_IREDUCE_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Ireduce algorithm. It is used for `MPI_Ireduce` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based automatic selection or a specific local-reduce-remote-send Ireduce implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_local_reduce_remote_send - forces the local-reduce-remote-send algorithm.

Used in functions: MPI_Ireduce

**MPIR_CVAR_IREDUCE_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ireduce algorithm. It is used by the generic transport scheduler path for intra-communicator Ireduce when this CVAR directly forces a generic transport tree or ring algorithm; otherwise, it falls through to internal collective selection, which may be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`, and then to a flat tree fallback if no generic transport container is selected. When active, this CVAR sets which intra-communicator Ireduce implementation is forced or whether internal collective selection chooses the implementation. The forced tree path uses the selected Ireduce tree type, tree k value, tree pipeline chunk size, and tree buffer-per-child CVARs, and it falls back to the ring-shaped tree path if that tree path cannot be applied. The forced ring path uses the Ireduce ring chunk size and tree buffer-per-child CVARs.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_smp - forces the SMP algorithm.
* sched_binomial - forces the binomial algorithm.
* sched_reduce_scatter_gather - forces the reduce-scatter-gather algorithm.
* tsp_tree - forces the generic transport tree algorithm.
* tsp_ring - forces the generic transport ring algorithm.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tsp_auto

**MPIR_CVAR_IREDUCE_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Defines the algorithm selector for POSIX shared-memory intranode nonblocking reduce. In `src/mpid/ch4/shm/posix/posix_coll.h`, the POSIX nonblocking reduce implementation does not read this CVAR and instead calls the MPIR nonblocking reduce implementation directly. No alternate CVAR name, dependency on another CVAR, or active mode requirement is visible in this file. As implemented in this file, this CVAR does not enable or set POSIX nonblocking reduce behavior.

* auto - default, represent internal POSIX collective selection, which the CVAR description says can be overridden by `MPIR_CVAR_CH4_POSIX_COLL_SELECTION_TUNING_JSON_FILE`.
* mpir - represent fallback to MPIR collectives.
* release_gather - represent the POSIX shared-memory release-gather path.

Used in functions: none in `src/mpid/ch4/shm/posix/posix_coll.h`

**MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE** - COLLECTIVE

Sets the maximum pipeline chunk size, in bytes, for the generic transport ring-based intra-communicator Ireduce algorithm. It is used when `MPIR_CVAR_IREDUCE_INTRA_ALGORITHM` forces `tsp_ring`, and is also used by the fallback from the forced TSP tree path when that tree path cannot be applied. The ring path also uses `MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD`. When automatic collective selection chooses the TSP ring Ireduce path, the selected container may supply the chunk size instead of this CVAR.

* 0 - default, disables pipelining.
* positive integer - represents the maximum pipeline chunk size in bytes.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tsp_auto

**MPIR_CVAR_IREDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ireduce_scatter_block` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ireduce_scatter_block.

* true - default, allows the device to override MPIR-level `MPI_Ireduce_scatter_block` collective algorithms.
* false - disables the device override for `MPI_Ireduce_scatter_block`.

Used in functions: MPI_Ireduce_scatter_block

**MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Ireduce_scatter_block algorithm. It is used for `MPI_Ireduce_scatter_block` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which inter-communicator Ireduce_scatter_block implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_remote_reduce_local_scatterv - forces the remote-reduce-local-scatterv algorithm.

Used in functions: MPI_Ireduce_scatter_block

**MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ireduce_scatter_block algorithm. It is used for `MPI_Ireduce_scatter_block` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based automatic selection may also use `MPIR_CVAR_IREDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE`, and the generic transport recursive-exchange path uses `MPIR_CVAR_IREDUCE_SCATTER_BLOCK_RECEXCH_KVAL`. When active, this CVAR sets which Ireduce_scatter_block implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_noncommutative - forces the noncommutative algorithm.
* sched_recursive_doubling - forces the recursive-doubling algorithm.
* sched_pairwise - forces the pairwise algorithm.
* sched_recursive_halving - forces the recursive-halving algorithm.
* tsp_recexch - forces the generic transport recursive-exchange algorithm.

Used in functions: MPI_Ireduce_scatter_block

**MPIR_CVAR_IREDUCE_SCATTER_BLOCK_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for the generic transport recursive-exchange intra-communicator Ireduce_scatter_block algorithm. It is used when the recursive-exchange Ireduce_scatter_block path is selected, such as by forcing `MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM` to `tsp_recexch` or by internal collective selection. In automatic mode, selection may be overridden by `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based and inter-communicator Ireduce_scatter_block paths do not use this CVAR.

* integer - default is 2, represents the k value used by the recursive-exchange Ireduce_scatter_block algorithm.

Used in functions: MPI_Ireduce_scatter_block

**MPIR_CVAR_IREDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE** - COLLECTIVE

Sets the long-message threshold, in bytes, for commutative-operation sched-auto intra-communicator Ireduce_scatter and Ireduce_scatter_block algorithm selection. It is used only when the operation is commutative and the sched-auto intra-communicator path is active. For Ireduce_scatter, the message size is the sum of all receive counts multiplied by the datatype size; for Ireduce_scatter_block, it is the block receive count multiplied by the communicator size and datatype size. When active, messages below this threshold select recursive halving, while messages at or above this threshold select pairwise; noncommutative operations bypass this CVAR and select among noncommutative and recursive-doubling algorithms based on communicator size and block regularity.

* integer - default is 524288, represents the commutative Ireduce_scatter and Ireduce_scatter_block long-message threshold in bytes.

Used in functions: MPIR_Ireduce_scatter_intra_sched_auto, MPIR_Ireduce_scatter_block_intra_sched_auto

**MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Ireduce_scatter` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Ireduce_scatter.

* true - default, allows the device to override MPIR-level `MPI_Ireduce_scatter` collective algorithms.
* false - disables the device override for `MPI_Ireduce_scatter`.

Used in functions: MPI_Ireduce_scatter

**MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Ireduce_scatter algorithm. It is used for `MPI_Ireduce_scatter` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Ireduce_scatter implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_remote_reduce_local_scatterv - forces the remote-reduce-local-scatterv algorithm.

Used in functions: MPI_Ireduce_scatter

**MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Ireduce_scatter algorithm. It is used for `MPI_Ireduce_scatter` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based automatic selection may also use `MPIR_CVAR_IREDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE`, and the generic transport recursive-exchange path uses `MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL`. When active, this CVAR sets which Ireduce_scatter implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_noncommutative - forces the noncommutative algorithm.
* sched_recursive_doubling - forces the recursive-doubling algorithm.
* sched_pairwise - forces the pairwise algorithm.
* sched_recursive_halving - forces the recursive-halving algorithm.
* tsp_recexch - forces the generic transport recursive-exchange algorithm.

Used in functions: MPI_Ireduce_scatter

**MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL** - COLLECTIVE

Sets the k value for the generic transport recursive-exchange intra-communicator Ireduce_scatter algorithm. It is used when the recursive-exchange Ireduce_scatter path is selected, such as by forcing `MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM` to `tsp_recexch` or by internal collective selection. In automatic mode, selection may be overridden by `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based and inter-communicator Ireduce_scatter paths do not use this CVAR.

* integer - default is 2, represents the k value used by the recursive-exchange Ireduce_scatter algorithm.

Used in functions: MPI_Ireduce_scatter

**MPIR_CVAR_IREDUCE_TOPO_DIFF_GROUPS** - COLLECTIVE

Sets the topology-wave latency cost between different groups for generic transport tree-based intra-communicator Ireduce. It is used only when the Ireduce tree type is topology-wave, along with the Ireduce topology reorder setting and the topology-wave overhead, different-switches, and same-switches cost CVARs. When collective selection supplies topology-wave tuning data for the selected TSP tree Ireduce container, that tuning data overrides this CVAR for the tree construction call.

* integer - default is 2800, represents the topology-wave latency cost between different groups for Ireduce tree construction.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tree

**MPIR_CVAR_IREDUCE_TOPO_DIFF_SWITCHES** - COLLECTIVE

Sets the topology-wave latency cost between different switches in the same topology group for generic transport tree-based intra-communicator Ireduce. It is used only when the Ireduce tree type is topology-wave, along with the Ireduce topology reorder setting and the topology-wave overhead, different-groups, and same-switches cost CVARs. When collective selection supplies topology-wave tuning data for the selected TSP tree Ireduce container, that tuning data overrides this CVAR for the tree construction call.

* integer - default is 1900, represents the topology-wave latency cost between different switches in the same group for Ireduce tree construction.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tree

**MPIR_CVAR_IREDUCE_TOPO_OVERHEAD** - COLLECTIVE

Sets the topology-wave overhead cost for generic transport tree-based intra-communicator Ireduce. It is used only when the Ireduce tree type is topology-wave, along with the Ireduce topology reorder setting and the topology latency CVARs for different groups, different switches, and the same switch. When collective selection supplies topology-wave tuning data for the selected TSP tree Ireduce container, that tuning data overrides this CVAR for the tree construction call.

* integer - default is 200, represents the topology-wave overhead cost for Ireduce tree construction.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tree

**MPIR_CVAR_IREDUCE_TOPO_REORDER_ENABLE** - COLLECTIVE

Enables reordering leaders based on the number of ranks in each group when creating topology-aware Ireduce trees. It is used only by the generic transport tree-based intra-communicator Ireduce implementation when `MPIR_CVAR_IREDUCE_TREE_TYPE` selects a topology-aware, topology-aware-k, or topology-wave tree. For topology-wave trees, the tree construction also uses the Ireduce topology cost CVARs for overhead and inter-group, inter-switch, and same-switch latencies.

* false - Disables leader reordering for topology-aware Ireduce tree construction.
* true - default, enables leader reordering for topology-aware Ireduce tree construction.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tree

**MPIR_CVAR_IREDUCE_TOPO_SAME_SWITCHES** - COLLECTIVE

Sets the topology-wave latency cost within the same switch for generic transport tree-based intra-communicator Ireduce. It is used only when the Ireduce tree type is topology-wave, along with the Ireduce topology reorder setting and the topology-wave overhead, different-groups, and different-switches cost CVARs. When collective selection supplies topology-wave tuning data for the selected TSP tree Ireduce container, that tuning data overrides this CVAR for the tree construction call.

* integer - default is 1600, represents the topology-wave latency cost within the same switch for Ireduce tree construction.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tree

**MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD** - COLLECTIVE

Sets whether the generic transport tree-based intra-communicator Ireduce implementation allocates a dedicated receive buffer for each child. It is used when `MPIR_CVAR_IREDUCE_INTRA_ALGORITHM` forces `tsp_tree` or `tsp_ring`, and is also used by the fallback from the forced TSP tree path when that tree path cannot be applied. The forced tree path also relies on the selected Ireduce tree type, `MPIR_CVAR_IREDUCE_TREE_KVAL`, and `MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE`; the forced ring path uses `MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE`. When automatic collective selection chooses a TSP tree or ring Ireduce path, the selected container may supply the buffer-per-child setting instead of this CVAR.

* false - default, uses one receive buffer for all children, so receives from children are serialized.
* true - allocates a dedicated receive buffer for each child, enabling receives to be preposted at the cost of additional memory.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tsp_auto

**MPIR_CVAR_IREDUCE_TREE_KVAL** - COLLECTIVE

Sets the k value for the generic transport tree-based intra-communicator Ireduce algorithm. It is used when `MPIR_CVAR_IREDUCE_INTRA_ALGORITHM` forces `tsp_tree`; that path also uses the selected Ireduce tree type, tree pipeline chunk size, and tree buffer-per-child CVARs. When automatic collective selection chooses the TSP tree Ireduce path, the selected container may supply the k value instead of this CVAR.

* integer - default is 2, represents the k value used by the generic transport tree-based Ireduce algorithm.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tsp_auto

**MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE** - COLLECTIVE

Sets the maximum pipeline chunk size, in bytes, for the generic transport tree-based intra-communicator Ireduce algorithm. It is used when `MPIR_CVAR_IREDUCE_INTRA_ALGORITHM` forces `tsp_tree`; that path also uses the selected Ireduce tree type, `MPIR_CVAR_IREDUCE_TREE_KVAL`, and `MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD`. When automatic collective selection chooses the TSP tree Ireduce path, the selected container may supply the chunk size instead of this CVAR.

* -1 - default CVAR value.
* 0 - disables pipelining.
* positive integer - represents the maximum pipeline chunk size in bytes.

Used in functions: MPIR_TSP_Ireduce_sched_intra_tsp_auto

**MPIR_CVAR_IREDUCE_TREE_TYPE** - COLLECTIVE

Sets the tree type for tree-based Ireduce. It is initialized during collective setup and used for generic transport tree-based Ireduce when `MPIR_CVAR_IREDUCE_INTRA_ALGORITHM` selects the tree path or when internal collective selection chooses that path. Topology-aware tree choices also rely on the Ireduce topology CVARs for leader reordering and topology cost settings.

* kary - default, represents a k-ary tree.
* knomial_1 - represents a knomial_1 tree.
* knomial_2 - represents a knomial_2 tree.
* topology_aware - represents a topology-aware tree.
* topology_aware_k - represents a topology-aware tree with branching factor k.
* topology_wave - represents a topology-wave tree.

Used in functions: MPII_Coll_init

**MPIR_CVAR_ISCAN_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Iscan` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Iscan.

* true - default, allows the device to override MPIR-level `MPI_Iscan` collective algorithms.
* false - disables the device override for `MPI_Iscan`.

Used in functions: MPI_Iscan

**MPIR_CVAR_ISCAN_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Iscan algorithm. It is used for `MPI_Iscan` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Iscan implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - uses internal algorithm selection for sched-based algorithms.
* sched_smp - forces the SMP algorithm.
* sched_recursive_doubling - forces the recursive-doubling algorithm.
* tsp_recursive_doubling - forces the generic transport recursive-doubling algorithm.

Used in functions: MPI_Iscan

**MPIR_CVAR_ISCATTERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Iscatterv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Iscatterv.

* true - default, allows the device to override MPIR-level `MPI_Iscatterv` collective algorithms.
* false - disables the device override for `MPI_Iscatterv`.

Used in functions: MPI_Iscatterv

**MPIR_CVAR_ISCATTERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Scatterv algorithm. It is used for `MPI_Iscatterv` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based automatic selection or a specific linear Iscatterv implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_linear - Forces the sched-based linear algorithm.
* tsp_linear - Forces the generic transport linear algorithm.

Used in functions: MPI_Iscatterv

**MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator nonblocking Scatterv algorithm. It is used for `MPI_Iscatterv` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; otherwise, this CVAR sets whether MPICH uses sched-based automatic selection or a specific linear Iscatterv implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_linear - Forces the sched-based linear algorithm.
* tsp_linear - Forces the generic transport linear algorithm.

Used in functions: MPI_Iscatterv

**MPIR_CVAR_ISCATTER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Iscatter` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Iscatter.

* true - default, allows the device to override MPIR-level `MPI_Iscatter` collective algorithms.
* false - disables the device override for `MPI_Iscatter`.

Used in functions: MPI_Iscatter

**MPIR_CVAR_ISCATTER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator nonblocking Scatter algorithm. It is used for `MPI_Iscatter` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; sched-based automatic selection uses `MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE` to choose between the remote-send-local-scatter and linear inter-communicator Scatter schedules.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_linear - Forces the sched-based linear algorithm.
* sched_remote_send_local_scatter - Forces the sched-based remote-send-local-scatter algorithm.

Used in functions: MPI_Iscatter

**MPIR_CVAR_ISCATTER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Iscatter algorithm. It is used for `MPI_Iscatter` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When the tree implementation is selected, the tree radix is controlled by `MPIR_CVAR_ISCATTER_TREE_KVAL`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* sched_auto - Uses internal algorithm selection for sched-based algorithms.
* sched_binomial - Forces the sched-based binomial Iscatter algorithm.
* tsp_tree - Forces the generic transport based tree Iscatter algorithm.

Used in functions: MPI_Iscatter

**MPIR_CVAR_ISCATTER_TREE_KVAL** - COLLECTIVE

Sets the tree k value for the generic transport based tree intra-communicator Iscatter algorithm. It is used when the tree implementation is selected, such as by forcing `MPIR_CVAR_ISCATTER_INTRA_ALGORITHM` to `tsp_tree` or by internal collective selection.

* integer - default is 2, represents the k value used by the tree based Iscatter algorithm.

Used in functions: MPI_Iscatter

**MPIR_CVAR_MEMDUMP** - DEVELOPER

Controls whether MPICH dumps memory that remains allocated when `MPI_Finalize` completes. It is used during memory tracing finalization and is effective only when MPICH is configured with memory tracing support. When active, this CVAR enables reporting of memory allocated by MPICH that remains allocated after finalization.

* true - default, dump remaining MPICH memory allocations when memory tracing finalizes.
* false - do not dump remaining MPICH memory allocations when memory tracing finalizes.

Used in functions: MPII_finalize_memory_tracing

**MPIR_CVAR_NAMESERV_FILE_PUBDIR** - PROCESS_MANAGER

Sets the base directory used by the file nameserv implementation for MPI service publishing and lookup. It is used when a file nameserv handle is created for connect/accept based applications; publish, lookup, and unpublish operations then use service-name files under the selected nameserv directory. The source defines `MPIR_CVAR_NAMEPUB_DIR` as an alternate environment variable, but `MPIR_CVAR_NAMESERV_FILE_PUBDIR` is not an alternate name for another CVAR. When active, this CVAR sets where file-based MPI service publishing data is stored.

* NULL - default, use `HOME` as the base directory when available, otherwise use the current directory.
* directory path - use the specified directory as the base directory for the `.mpinamepub` nameserv directory.

Alternate name: MPIR_CVAR_NAMEPUB_DIR

Used in functions: MPID_NS_Create

**MPIR_CVAR_NEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_allgatherv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_allgatherv.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_allgatherv` collective algorithms.
* false - disables the device override for `MPI_Neighbor_allgatherv`.

Used in functions: MPI_Neighbor_allgatherv

**MPIR_CVAR_NEIGHBOR_ALLGATHERV_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_allgatherv_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_allgatherv_init.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_allgatherv_init` collective algorithms.
* false - disables the device override for `MPI_Neighbor_allgatherv_init`.

Used in functions: MPI_Neighbor_allgatherv_init

**MPIR_CVAR_NEIGHBOR_ALLGATHERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Neighbor_allgatherv algorithm. It is used for `MPI_Neighbor_allgatherv` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Neighbor_allgatherv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.

Used in functions: MPI_Neighbor_allgatherv

**MPIR_CVAR_NEIGHBOR_ALLGATHERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Neighbor_allgatherv algorithm. It is used for `MPI_Neighbor_allgatherv` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Neighbor_allgatherv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.

Used in functions: MPI_Neighbor_allgatherv

**MPIR_CVAR_NEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_allgather` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_allgather.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_allgather` collective algorithms.
* false - disables the device override for `MPI_Neighbor_allgather`.

Used in functions: MPI_Neighbor_allgather

**MPIR_CVAR_NEIGHBOR_ALLGATHER_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_allgather_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_allgather_init.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_allgather_init` collective algorithms.
* false - disables the device override for `MPI_Neighbor_allgather_init`.

Used in functions: MPI_Neighbor_allgather_init

**MPIR_CVAR_NEIGHBOR_ALLGATHER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Neighbor_allgather algorithm. It is used for `MPI_Neighbor_allgather` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Neighbor_allgather implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.

Used in functions: MPI_Neighbor_allgather

**MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Neighbor_allgather algorithm. It is used for `MPI_Neighbor_allgather` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Neighbor_allgather implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.

Used in functions: MPI_Neighbor_allgather

**MPIR_CVAR_NEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_alltoallv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_alltoallv.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_alltoallv` collective algorithms.
* false - disables the device override for `MPI_Neighbor_alltoallv`.

Used in functions: MPI_Neighbor_alltoallv

**MPIR_CVAR_NEIGHBOR_ALLTOALLV_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_alltoallv_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_alltoallv_init.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_alltoallv_init` collective algorithms.
* false - disables the device override for `MPI_Neighbor_alltoallv_init`.

Used in functions: MPI_Neighbor_alltoallv_init

**MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Neighbor_alltoallv algorithm. It is used for `MPI_Neighbor_alltoallv` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_NEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Neighbor_alltoallv implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.

Alternate names: MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTER_ALGORITHM

Used in functions: MPI_Neighbor_alltoallv

**MPIR_CVAR_NEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_alltoallw` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_alltoallw.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_alltoallw` collective algorithms.
* false - disables the device override for `MPI_Neighbor_alltoallw`.

Used in functions: MPI_Neighbor_alltoallw

**MPIR_CVAR_NEIGHBOR_ALLTOALLW_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_alltoallw_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_alltoallw_init.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_alltoallw_init` collective algorithms.
* false - disables the device override for `MPI_Neighbor_alltoallw_init`.

Used in functions: MPI_Neighbor_alltoallw_init

**MPIR_CVAR_NEIGHBOR_ALLTOALLW_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Neighbor_alltoallw algorithm. It is used for `MPI_Neighbor_alltoallw` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_NEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Neighbor_alltoallw implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.

Alternate names: MPIR_CVAR_NEIGHBOR_ALLTOALLW_INTER_ALGORITHM

Used in functions: MPI_Neighbor_alltoallw

**MPIR_CVAR_NEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_alltoall` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_alltoall.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_alltoall` collective algorithms.
* false - disables the device override for `MPI_Neighbor_alltoall`.

Used in functions: MPI_Neighbor_alltoall

**MPIR_CVAR_NEIGHBOR_ALLTOALL_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Neighbor_alltoall_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Neighbor_alltoall_init.

* true - default, allows the device to override MPIR-level `MPI_Neighbor_alltoall_init` collective algorithms.
* false - disables the device override for `MPI_Neighbor_alltoall_init`.

Used in functions: MPI_Neighbor_alltoall_init

**MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Neighbor_alltoall algorithm. It is used for `MPI_Neighbor_alltoall` on intra-communicators when the MPIR-level collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`, `MPIR_CVAR_NEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE` may allow a device override instead of the MPIR-level algorithm. When active, this CVAR sets which Neighbor_alltoall implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - forces the nonblocking algorithm.

Alternate names: MPIR_CVAR_NEIGHBOR_ALLTOALL_INTER_ALGORITHM

Used in functions: MPI_Neighbor_alltoall

**MPIR_CVAR_NEMESIS_ENABLE_CKPOINT** - NEMESIS

Controls whether CH3 nemesis checkpointing support is initialized and progressed. It is used only when MPICH is built with checkpointing support. During nemesis checkpoint initialization, it controls whether the BLCR checkpointing library is initialized, the checkpoint callback is registered, and checkpoint synchronization semaphores are created. During nemesis progress, it controls whether pending checkpoint-start and checkpoint-finish requests raised by the checkpoint callback or checkpoint marker packets are processed. When active, this CVAR enables nemesis checkpoint coordination and integration with the checkpointing library.

* false - default, do not initialize or progress nemesis checkpointing support.
* true - initialize and progress nemesis checkpointing support.

Used in functions: MPIDI_CH3I_Progress, MPIDI_nem_ckpt_init

**MPIR_CVAR_NEMESIS_NETMOD** - NEMESIS

Sets the network module used for CH3 Nemesis communication. It is used while choosing the Nemesis netmod; if unset, Nemesis uses the default compiled netmod, otherwise the configured name is matched case-insensitively against the compiled netmod names. It does not rely on another CVAR or mode being active. When active, this CVAR selects the Nemesis network module implementation.

* "" - default, use the first compiled Nemesis netmod.
* compiled Nemesis netmod name - select the matching compiled Nemesis netmod.

Used in functions: MPID_nem_choose_netmod

**MPIR_CVAR_NEMESIS_SHM_EAGER_MAX_SZ** - NEMESIS

Sets the shared-memory eager-send threshold for CH3 Nemesis. It is used when a virtual connection is local and therefore uses Nemesis shared-memory communication; nonlocal connections use the selected netmod path instead. This CVAR does not rely on another CVAR being enabled, but the ready-send eager threshold follows this value when `MPIR_CVAR_NEMESIS_SHM_READY_EAGER_MAX_SZ` is configured to use the shared-memory eager threshold. When active, this CVAR sets the maximum shared-memory message size sent eagerly before Nemesis uses rendezvous mode.

* -1 - default, choose the shared-memory eager threshold from the Nemesis MPICH data-cell payload size after packet-header space is reserved.
* nonnegative integer - use the specified byte count as the maximum shared-memory eager message size.

Used in functions: MPID_nem_vc_init

**MPIR_CVAR_NEMESIS_SHM_READY_EAGER_MAX_SZ** - NEMESIS

Sets the shared-memory ready-send eager threshold for CH3 Nemesis. It is used when a virtual connection is local and therefore uses Nemesis shared-memory communication; nonlocal connections use the selected netmod path instead. This CVAR does not rely on another mode being active, but it can follow the resolved value of `MPIR_CVAR_NEMESIS_SHM_EAGER_MAX_SZ`. When active, this CVAR sets the maximum shared-memory ready-send message size sent eagerly before Nemesis uses rendezvous mode.

* -2 - default, use the resolved shared-memory eager threshold from `MPIR_CVAR_NEMESIS_SHM_EAGER_MAX_SZ`.
* -1 - always send shared-memory ready-send messages eagerly.
* nonnegative integer - use the specified byte count as the maximum shared-memory ready-send eager message size.

Used in functions: MPID_nem_vc_init

**MPIR_CVAR_NEMESIS_TCP_HOST_LOOKUP_RETRIES** - NEMESIS

Sets how many times the CH3 nemesis TCP netmod retries hostname lookup before giving up. It applies while selecting the socket interface address for the TCP business card when the path must resolve a hostname rather than using an address obtained directly from a selected or discovered interface. This path can be avoided when `MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE` selects an interface successfully or when local interface discovery finds an address directly. When active, this CVAR sets the hostname lookup retry count used by TCP address resolution.

* 10 - default, retry hostname lookup up to 10 times before giving up.
* nonnegative integer - retry hostname lookup up to the requested number of times before giving up.

Used in functions: GetSockInterfaceAddr

**MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE** - NEMESIS

Specifies the pseudo-ethernet interface that the CH3 nemesis TCP netmod uses when selecting the socket address to publish in its business card. It is used while building the TCP business card and only when this CVAR is set; setting it together with `MPIR_CVAR_CH3_INTERFACE_HOSTNAME` is an error. Hydra can also set this CVAR from its selected interface option, unless forced hostname propagation is requested. When active, this CVAR sets the network interface whose address is advertised for TCP connections to this process.

* NULL - default, do not select a network interface through this CVAR.
* string - network interface name to use for TCP connections.

Alternate name: MPIR_CVAR_NETWORK_IFACE

Used in functions: GetSockInterfaceAddr, MPID_nem_tcp_get_business_card, post_process

**MPIR_CVAR_NETLOC_NODE_FILE** - DEBUGGER

Sets the netloc subnet topology JSON file used to initialize MPICH network-topology information. It is used in `MPII_nettopo_init` only when MPICH is built with netloc support; if no non-empty file name is supplied, no netloc topology is parsed and the network topology remains invalid. When active, this CVAR selects the topology file parsed by netloc and used to populate MPIR nettopo network attributes.

* auto - default, use the CVAR's automatic default string for the subnet JSON file.
* "" - do not parse a netloc topology file.
* file path - parse the specified netloc subnet topology JSON file.

Used in functions: MPII_nettopo_init

**MPIR_CVAR_NOLOCAL** - NODEMAP

Forces the nodemap builder to treat every process as being on a separate node. It is used in `MPIR_build_nodemap`; the reviewed source defines `MPIR_CVAR_NO_LOCAL` as an alternate environment variable, but `MPIR_CVAR_NOLOCAL` is not an alternate name for another CVAR. This path is also enabled unconditionally when MPICH is built with `ENABLE_NO_LOCAL`, and it causes local-clique detection to report active local cliques. When active, this CVAR sets the nodemap to one process per node, disabling local shared-memory locality assumptions.

* false - default, use process-manager node information and any active local-clique partitioning.
* true - build a one-process-per-node nodemap.

Alternate name: MPIR_CVAR_NO_LOCAL

Used in functions: MPIR_build_nodemap, MPIR_pmi_has_local_cliques

**MPIR_CVAR_NO_COLLECTIVE_FINALIZE** - COLLECTIVE

Controls whether CH4 netmod finalization performs collective synchronization at `MPI_Finalize`. It is used by the UCX finalize hook to skip the PMI barrier before endpoint disconnect, and by the OFI finalize hook to skip provider-specific collective workarounds that would otherwise run for selected providers; the OFI sockets provider still performs its send-queue flush independently. This CVAR does not rely on another CVAR being enabled, but it is effective only when the selected netmod reaches a finalize path that would otherwise perform one of these collective finalization steps. When active, this CVAR enables non-collective finalization by suppressing those finalization barriers or collective workarounds.

* false - default, allow the netmod finalize path to perform its collective finalization barrier or workaround when applicable.
* true - skip the applicable collective finalization barrier or workaround.

Used in functions: MPIDI_OFI_mpi_finalize_hook, MPIDI_UCX_mpi_finalize_hook

**MPIR_CVAR_NUM_CLIQUES** - NODEMAP

Controls whether MPICH partitions processes on a single local node into multiple local cliques for nodemap debugging. It is used after the PMI nodemap has been built and normalized; the requested clique count is capped at the job size, applied only when the normalized nodemap has one node, and takes precedence over the deprecated `MPIR_CVAR_ODD_EVEN_CLIQUES` setting. `MPIR_CVAR_CLIQUES_BY_BLOCK` controls whether the partitioning is by uniform blocks instead of the default round-robin assignment. Progress-thread affinity detects these local cliques and warns that affinity cannot work correctly with them. When active, this CVAR sets the number of virtual local nodes used to decide which processes are seen as local to each other.

* 1 - default, do not partition the local node into multiple cliques; if `MPIR_CVAR_ODD_EVEN_CLIQUES` is true, use two cliques instead.
* integer greater than 1 - partition processes on a single local node into that many cliques, capped at the job size.

Used in functions: MPIR_build_nodemap, get_option_num_cliques, MPIR_pmi_has_local_cliques, get_thread_affinity

**MPIR_CVAR_NUM_MULTI_LEADS** - COLLECTIVE

Sets the requested number of leader ranks per node for CH4 multi-leader collective compositions. In the reviewed CH4 collective paths, it is used by the allreduce multi-leader delta composition when that composition is selected by `MPIR_CVAR_ALLREDUCE_COMPOSITION` or by the CH4 collective-selection table. The requested count is rounded so the resulting leader count divides the node-local communicator size, within the adjustment range used by the implementation, before the multi-leader allreduce path is run. That path also requires an intracommunicator, a node-balanced communicator, a message count at least as large as the resulting leader count, and a commutative operation. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll_impl.h` or `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets how many per-node leaders the CH4 multi-leader allreduce composition tries to use.

* 4 - default, request four leader ranks per node.
* integer - request the specified number of leader ranks per node; the implementation may adjust the count to fit the node-local communicator size.

Used in functions: MPIDI_Allreduce_allcomm_composition_json, MPID_Allreduce

**MPIR_CVAR_ODD_EVEN_CLIQUES** - NODEMAP

Controls whether the nodemap builder splits processes on a single physical node into two local cliques for debugging. It is used through the local-clique count selected by `get_option_num_cliques`; `MPIR_CVAR_NUM_CLIQUES` takes precedence when it is greater than one, and clique partitioning is applied by `MPIR_build_nodemap` only after process-manager node mapping finds a single node and `MPIR_CVAR_NOLOCAL` is not active. When active, this CVAR sets the local-clique count to two, causing odd-ranked processes on the node to be local to each other and even-ranked processes on the node to be local to each other.

* false - default, do not request odd/even local-clique partitioning.
* true - request two odd/even local cliques when no larger `MPIR_CVAR_NUM_CLIQUES` setting overrides it.

Alternate name: MPIR_CVAR_EVEN_ODD_CLIQUES

Used in functions: MPIR_build_nodemap, MPIR_pmi_has_local_cliques

**MPIR_CVAR_OFI_SKIP_IPV6** - DEVELOPER

Controls whether CH4 OFI skips libfabric providers that use IPv6 socket addresses during multi-NIC initialization. It is used while counting and selecting usable providers, before link-state filtering, hostname preference matching, and NIC collection. No alternate CVAR name, dependency on another CVAR, or additional active mode requirement is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/ofi_nic.c`. When active, this CVAR excludes IPv6-addressed OFI providers from CH4 OFI provider and NIC selection.

* false - default, do not skip IPv6-addressed OFI providers.
* true - skip OFI providers whose address format is IPv6 socket address.

Used in functions: MPIDI_OFI_init_multi_nic

**MPIR_CVAR_OFI_USE_MIN_NICS** - DEVELOPER

Controls whether CH4 OFI can reduce the effective NIC count to the minimum count available across ranks when ranks do not have the same number of NICs. It is used during communicator VCI setup after local VCIs and NIC contexts are initialized and before address exchange. The check uses the current local NIC count initialized from the available NIC count and whether each rank can still change its NIC count; no alternate CVAR name or dependency on another CVAR is visible in `src/mpid/ch4/netmod/ofi/ofi_init.c` or `src/mpid/ch4/netmod/ofi/ofi_vci.c`. When active, this CVAR enables CH4 OFI to fall back to a common smaller NIC count instead of failing on mismatched NIC counts, and falling back to one NIC disables communicator multi-NIC striping.

* true - default, allow the first NIC-count consistency check to reduce the effective NIC count to the minimum available across ranks.
* false - do not allow the NIC-count consistency check to reduce the effective NIC count; mismatched NIC counts produce an OFI NIC-count error.

Used in functions: check_num_nics

**MPIR_CVAR_OFI_USE_PROVIDER** - DEVELOPER

Reports that the MPICH OFI provider-selection CVAR is no longer supported. It is checked while CH4 OFI is finding a libfabric provider, before OFI runtime version setup, provider discovery, provider scoring, or configure-time provider validation. It does not rely on another CVAR or mode being active. When active, this CVAR enables only a diagnostic telling users to select libfabric providers with `FI_PROVIDER` instead.

* NULL - default, do not print the unsupported-CVAR diagnostic.
* string - print the unsupported-CVAR diagnostic; the value is not used to select an OFI provider.

Used in functions: find_provider

**MPIR_CVAR_PMI_DISABLE_GROUP** - PMI

Controls whether MPICH avoids PMI group barrier/fence operations. It is used during PMI initialization to record whether group barrier support is available, during communicator creation from groups when session support is not using `MPI_COMM_WORLD`, and during CH4 business-card exchange across node roots. The CVAR may be set by the user or set automatically when a test PMI group barrier/fence fails. When active, this CVAR enables fallback paths that use full-world or communicator-based synchronization instead of PMI group operations.

* false - default, allows PMI group barrier/fence operations and group-based PMI allgather paths when supported.
* true - disables PMI group barrier/fence operations and uses fallback synchronization paths.

Used in functions: MPIR_pmi_init, MPIR_pmi_allgather_group, MPIR_Comm_create_from_group_impl, MPIDIU_bc_exchange_node_roots

**MPIR_CVAR_PMI_VERSION** - PMI

Selects the process-management interface used by MPICH PMI wrapper routines. It is checked during `MPIR_pmi_init` before PMI initialization; automatic selection depends on process-manager environment variables, and explicit selections depend on the corresponding PMI interface being enabled at build time. When active, this CVAR sets which PMI implementation is used for PMI initialization, finalization, key-value operations, barriers, spawn/publish/lookup operations, nodemap fallback behavior, and PMIx-specific topology and process-set event support.

* 1 - use PMI.
* 2 - use PMI2.
* x - use PMIx.
* auto - default, detect the PMI interface from the runtime environment or choose an available interface for singleton initialization.

Used in functions: check_MPIR_CVAR_PMI_VERSION, MPIR_pmi_finalize_on_exit, MPIR_pmi_init, MPIR_pmi_abort, MPIR_pmi_kvs_put, MPIR_pmi_kvs_get, MPIR_pmi_kvs_parent_get, MPIR_pmi_get_jobattr, MPIR_pmi_build_nodemap, MPIR_pmi_barrier, MPIR_pmi_barrier_only, MPIR_pmi_barrier_local, MPIR_pmi_barrier_group, optimized_put, optimized_get, put_ex, get_ex, optional_bcast_barrier, MPIR_pmi_get_universe_size, MPIR_pmi_spawn_multiple, MPIR_pmi_publish, MPIR_pmi_lookup, MPIR_pmi_unpublish, MPIR_pmi_load_hwloc_topology, MPIR_pmi_pset_event_init, MPIR_pmi_pset_event_finalize

**MPIR_CVAR_PMI_VERSION_1** - PMI

Selects the PMI-1 process-management interface as the active `MPIR_CVAR_PMI_VERSION` enum value. It is used when `MPIR_CVAR_PMI_VERSION` is explicitly set to this value, when auto-detection selects PMI because PMI process-manager environment variables are present, or when singleton initialization falls back to PMI-1 as an available interface. If MPICH is built without PMI-1 support, this value is replaced by another enabled PMI interface when possible, otherwise PMI initialization fails. When active, this value routes MPICH PMI wrapper routines through the PMI-1 implementation for initialization, finalization, key-value operations, barriers, spawn/publish/lookup operations, and fallback nodemap construction.

* MPIR_CVAR_PMI_VERSION_1 - use PMI-1.

Used in functions: check_MPIR_CVAR_PMI_VERSION, MPIR_pmi_finalize_on_exit, MPIR_pmi_init, MPIR_pmi_abort, MPIR_pmi_kvs_put, MPIR_pmi_kvs_get, MPIR_pmi_kvs_parent_get, MPIR_pmi_get_jobattr, MPIR_pmi_build_nodemap, MPIR_pmi_barrier, MPIR_pmi_barrier_only, MPIR_pmi_barrier_local, MPIR_pmi_barrier_group, optimized_put, optimized_get, put_ex, get_ex, optional_bcast_barrier, MPIR_pmi_get_universe_size, MPIR_pmi_spawn_multiple, MPIR_pmi_publish, MPIR_pmi_lookup, MPIR_pmi_unpublish, MPIR_pmi_pset_event_init, MPIR_pmi_pset_event_finalize

**MPIR_CVAR_PMI_VERSION_2** - PMI

Selects the PMI-2 process-management interface as the active `MPIR_CVAR_PMI_VERSION` enum value. It is used when `MPIR_CVAR_PMI_VERSION` is explicitly set to this value, when auto-detection selects PMI because PMI process-manager environment variables are present and PMI-1 support is not enabled, when singleton initialization falls back to PMI-2 as an available interface, or when an unavailable PMI-1 selection is replaced by PMI-2. If MPICH is built without PMI-2 support, selecting this value causes PMI initialization to fail. When active, this value routes MPICH PMI wrapper routines through the PMI-2 implementation for initialization, finalization, key-value operations, barriers, spawn/publish/lookup operations, and fallback nodemap construction.

* MPIR_CVAR_PMI_VERSION_2 - use PMI-2.

Used in functions: check_MPIR_CVAR_PMI_VERSION, MPIR_pmi_finalize_on_exit, MPIR_pmi_init, MPIR_pmi_abort, MPIR_pmi_kvs_put, MPIR_pmi_kvs_get, MPIR_pmi_kvs_parent_get, MPIR_pmi_get_jobattr, MPIR_pmi_build_nodemap, MPIR_pmi_barrier, MPIR_pmi_barrier_only, MPIR_pmi_barrier_local, MPIR_pmi_barrier_group, optimized_put, optimized_get, put_ex, get_ex, optional_bcast_barrier, MPIR_pmi_get_universe_size, MPIR_pmi_spawn_multiple, MPIR_pmi_publish, MPIR_pmi_lookup, MPIR_pmi_unpublish, MPIR_pmi_pset_event_init, MPIR_pmi_pset_event_finalize

**MPIR_CVAR_POLLS_BEFORE_YIELD** - NEMESIS

Sets how many busy-wait loop iterations MPICH performs before yielding the processor in the CH3 nemesis blocking receive path. It is used by the busy-wait macro called from `MPID_nem_mpich_blocking_recv`, after queue, fastbox, and network polling have not found a receivable cell and the loop remains safe to continue. This CVAR is effective only when the build uses the yield-capable busy-wait implementation; builds configured to use nothing for yielding compile the busy-wait macro to a no-op. When active, this CVAR sets the polling interval for calling the thread-yield function during nemesis busy waiting.

* 1000 - default, yield after 1000 busy-wait loop iterations.
* positive integer - yield after the requested number of busy-wait loop iterations.
* 0 - disable yielding from the busy-wait macro.

Used in functions: MPID_nem_mpich_blocking_recv

**MPIR_CVAR_POSIX_NUM_COLLS_THRESHOLD** - COLLECTIVE

Sets the number of POSIX shared-memory release-gather collective calls required before using the optimized release-gather paths. It is used by POSIX release-gather broadcast, reduce, allreduce, and barrier after each participating communicator counts a release-gather collective call; calls below the threshold fall back to the corresponding MPIR implementation. Reduce and allreduce also require the datatype size or extent to fit within the intranode reduce cell size set by `MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE` and `MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS`. When active, this CVAR sets the warmup threshold for enabling POSIX release-gather collectives instead of fallback collectives.

* integer - default is 5, represents the release-gather collective-call threshold.

Used in functions: MPIDI_POSIX_mpi_bcast_release_gather, MPIDI_POSIX_mpi_reduce_release_gather, MPIDI_POSIX_mpi_allreduce_release_gather, MPIDI_POSIX_mpi_barrier_release_gather

**MPIR_CVAR_POSIX_NUM_NB_COLLS_THRESHOLD** - COLLECTIVE

Sets the minimum total number of POSIX release-gather nonblocking collective calls on a communicator before the release-gather nonblocking broadcast or reduce implementation is used. Each attempted release-gather nonblocking broadcast increments the communicator release-gather call count, and release-gather nonblocking reduce increments it when the communicator has more than one rank; calls below this threshold fall back to the MPIR TSP point-to-point algorithm. When the threshold is reached, the release-gather communicator state is lazily initialized for broadcast or reduce, and initialization failure also falls back to the MPIR TSP point-to-point algorithm. When active, this CVAR sets the warmup threshold before POSIX release-gather nonblocking collectives are used.

* integer - default, use the configured call-count threshold for POSIX release-gather nonblocking collectives.

Used in functions: MPIDI_POSIX_ibcast_release_gather, MPIDI_POSIX_ireduce_release_gather

**MPIR_CVAR_POSIX_POLL_FREQUENCY** - COLLECTIVE

Sets how often the POSIX release-gather wait loop calls `MPID_Progress_test` while spinning on shared-memory release or gather flags. It is used only in the release-gather wait macro, so it affects POSIX collectives only when a POSIX intranode release-gather algorithm is active, either because the corresponding POSIX algorithm CVAR selects `release_gather` or because automatic POSIX collective selection chooses it. From `src/mpid/ch4/shm/posix/posix_coll.h`, release-gather barrier and broadcast require MPICH not to be threaded, and release-gather reduce and allreduce require MPICH not to be threaded and the reduction operation to be commutative. Reduce and allreduce release-gather tree selection can also depend on `MPIR_CVAR_REDUCE_INTRANODE_MSG_SIZE_THRESHOLD`, but this CVAR only sets the polling interval for waits.

* 1000 - default, call `MPID_Progress_test` after 1000 spin-loop iterations while waiting.
* 0 - call `MPID_Progress_test` on every wait-loop iteration.
* positive integer - call `MPID_Progress_test` after the specified number of spin-loop iterations while waiting.

Used in functions: MPIDI_POSIX_mpi_release_gather_release, MPIDI_POSIX_mpi_release_gather_gather

**MPIR_CVAR_PRINT_ERROR_STACK** - ERROR_HANDLING

Controls whether the instance-specific portion of an MPI error string includes the full error stack. It is used when `MPIR_Err_get_string` formats an error code with instance-specific error-ring entries; when enabled, the stack is formatted by `MPIR_Err_print_stack_string`, whose line wrapping is controlled by `MPIR_CVAR_CHOP_ERROR_STACK`. When active, this CVAR enables printing the error stack trace instead of only the last specific error message.

* true - default, print the full error stack trace.
* false - print only the last specific error message.

Used in functions: ErrGetInstanceString

**MPIR_CVAR_PROCTABLE_PRINT** - DEBUGGER

Controls whether rank 0 prints the populated MPIR debugger interface proctable entries while waiting for debugger access. It is used only when proctable support is compiled in, after the number of entries to populate has been limited by `MPIR_CVAR_PROCTABLE_SIZE` and the MPI job size. When active, this CVAR enables printing of the populated proctable entries to standard output.

* false - default, do not print proctable entries.
* true - print the populated proctable entries from rank 0.

Used in functions: MPII_Wait_for_debugger

**MPIR_CVAR_PROCTABLE_SIZE** - DEBUGGER

Controls how many ranks populate the MPIR debugger interface proctable. It is used while waiting for debugger access when proctable support is compiled in; the selected size is capped at the MPI job size and does not rely on any other CVAR being enabled. When active, this CVAR sets the number of initial ranks that provide process table entries for debugger inspection.

* 64 - default, populate up to the first 64 ranks.
* nonpositive integer - only rank 0 initializes its own proctable entry; no other rank entries are collected or printed.
* positive integer less than the MPI job size - populate up to that many initial ranks.
* positive integer greater than or equal to the MPI job size - populate entries for all ranks in the job.

Used in functions: MPII_Wait_for_debugger

**MPIR_CVAR_PROGRESS_MAX_COLLS** - COLLECTIVE

Controls how many gentran collective operations the progress engine advances during one progress-hook invocation. It is used when gentran progress is running over the pending collective queue, and the progress hook is active only while gentran collective schedules are queued. When active, this CVAR sets the per-invocation limit on collective operations that report progress before the progress hook stops scanning the queue.

* 0 - default, no limit on the number of collective operations that can report progress during one progress-hook invocation.
* positive integer - maximum number of collective operations that can report progress during one progress-hook invocation.

Used in functions: MPII_Genutil_progress_hook

**MPIR_CVAR_PROGRESS_THREAD_AFFINITY** - THREADS

Specifies affinity for progress threads created by `MPIR_Start_progress_thread_impl`. It is used only in threaded builds with `MPI_THREAD_MULTIPLE` support and only when async thread affinity support is compiled in. When configured, MPICH parses the requested affinity, warns if local clique CVARs are active because they conflict with progress-thread affinity, and applies the selected logical processor binding after creating the progress thread. When active, this CVAR sets the logical processor affinity used for progress threads.

* empty string - default, do not set progress-thread affinity.
* auto - automatically select logical processors for progress-thread affinity.
* comma-separated list of logical processors - assign listed logical processors to progress threads by local-process order, one logical processor per progress thread.

Used in functions: MPIDI_parse_progress_thread_affinity, get_thread_affinity, MPIR_Start_progress_thread_impl

**MPIR_CVAR_PROGRESS_TIMEOUT** - CH4

Sets the timeout, in seconds, used by progress-wait debug checks. When enabled, MPICH records the start time of a progress wait, periodically checks elapsed time, dumps outstanding request information and a backtrace after the timeout is exceeded, and raises a progress timeout error if the wait continues beyond the second timeout interval. It does not rely on other CVAR values or modes being active.

* 0 - default, disables progress timeout checks.
* positive integer - enables progress timeout checks and represents the timeout in seconds.

Alternate name: MPIR_CVAR_DEBUG_PROGRESS_TIMEOUT

Used in functions: DEBUG_PROGRESS_START, DEBUG_PROGRESS_CHECK

**MPIR_CVAR_QMPI_TOOL_LIST** - TOOLS

Controls the QMPI tools that MPICH prepares and initializes. It is used during QMPI pre-initialization after the MPI_T CVAR environment has been initialized; this path is compiled only when QMPI support is enabled. When set, MPICH parses the configured tool list, records the number and order of tools, allocates QMPI dispatch and storage state, registers internal MPI function pointers, and later calls the registered tool initialization callbacks in that order. When active, this CVAR sets the QMPI tool chain used to intercept MPI calls.

* NULL - default, do not configure any QMPI tools.
* colon-separated string - configure the listed QMPI tool names in order.

Used in functions: MPII_qmpi_pre_init, MPII_qmpi_init, MPII_qmpi_teardown

**MPIR_CVAR_REDUCE_COMPOSITION** - COLLECTIVE

Selects the CH4 reduce composition in `MPID_Reduce`. When set to automatic selection, the CH4 collective-selection table is consulted and may fall back to the MPIR reduce implementation if no selection is found. Forced CH4 compositions apply only when their communicator and operation constraints are met; the combined network-module and shared-memory compositions require an intracommunicator, a parent communicator, and a commutative operation, while the network-module-only composition requires an intracommunicator. If a forced composition cannot be applied, the fallback path uses the MPIR reduce implementation for intercommunicators and the network-module-only CH4 composition for intracommunicators. No alternate CVAR name is visible in `src/mpid/ch4/src/ch4_coll.h`. When active, this CVAR sets whether CH4 reduce uses automatic collective selection, a combined network-module and shared-memory composition, or a network-module-only composition.

* 0 - default, use CH4 collective-selection table lookup and MPIR fallback when no selection is found.
* 1 - use the combined network-module and shared-memory intracommunicator composition with explicit send-receive between rank 0 and root when the communicator and operation constraints are met.
* 2 - use the combined network-module and shared-memory intracommunicator composition without explicit send-receive between rank 0 and root when the communicator and operation constraints are met.
* 3 - use the network-module-only intracommunicator composition when the communicator constraints are met.

Used in functions: MPID_Reduce

**MPIR_CVAR_REDUCE_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Reduce` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Reduce.

* true - default, allows the device to override MPIR-level `MPI_Reduce` collective algorithms.
* false - disables the device override for `MPI_Reduce`.

Alternate names: MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE

Used in functions: MPI_Reduce

**MPIR_CVAR_REDUCE_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Reduce_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Reduce_init.

* true - default, allows the device to override MPIR-level `MPI_Reduce_init` collective algorithms.
* false - disables the device override for `MPI_Reduce_init`.

Used in functions: MPI_Reduce_init

**MPIR_CVAR_REDUCE_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Reduce algorithm. It is used for `MPI_Reduce` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which inter-communicator Reduce implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* local_reduce_remote_send - Forces the local-reduce-remote-send Reduce algorithm.
* nb - Forces the nonblocking Reduce algorithm.

Used in functions: MPI_Reduce

**MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE** - COLLECTIVE

Sets the total shared-memory reduce buffer size per rank for POSIX shared-memory release-gather reduce operations. It is captured when release-gather state is initialized for a communicator and is used when allocating reduce buffers for blocking reduce, blocking allreduce, and nonblocking reduce. Together with `MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS`, it determines the per-cell reduce buffer size used for pipelining and the datatype size-or-extent limit checked before blocking reduce and allreduce can use release-gather instead of MPIR fallback. The allocated reduce shared memory also counts against `MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE`; exceeding that limit makes initialization fail and the collective fall back. When active, this CVAR sets the per-rank reduce shared-memory buffer capacity used by POSIX release-gather reduce collectives.

* integer - default is 32768, represents the total per-rank reduce buffer size in bytes.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_mpi_reduce_release_gather, MPIDI_POSIX_mpi_allreduce_release_gather, MPIDI_POSIX_nb_release_gather_comm_init, MPIDI_POSIX_NB_RG_all_datacopy_cb, MPIDI_POSIX_NB_RG_reduce_data_cb, MPIDI_POSIX_NB_RG_reduce_start_sendrecv_completion, MPIDI_POSIX_NB_RG_reduce_finish_sendrecv_completion

**MPIR_CVAR_REDUCE_INTRANODE_MSG_SIZE_THRESHOLD** - COLLECTIVE

Sets the message-size threshold used by POSIX shared-memory release-gather reduce and allreduce to choose between the normal and large reduce trees. It is used during the blocking release-gather gather and release steps, after release-gather reduce or allreduce has been selected and reduce buffers have been initialized. The message size is computed as datatype size times count, and the selected tree also depends on the reduce tree CVARs for normal and large messages. When active, this CVAR sets the cutoff for selecting the normal reduce tree versus the large-message reduce tree in POSIX release-gather reduce collectives.

* integer - default is 2048, represents the reduce message-size threshold in bytes; messages at or below the threshold use the normal reduce tree configured by `MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL` and `MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE`, while larger messages use the large-message reduce tree configured by `MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL_LARGE` and `MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE_LARGE`.

Used in functions: MPIDI_POSIX_mpi_release_gather_release, MPIDI_POSIX_mpi_release_gather_gather

**MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS** - COLLECTIVE

Sets the number of cells in the POSIX shared-memory release-gather reduce buffer. It is captured when blocking release-gather state is initialized for a communicator and is read when nonblocking release-gather reduce state is initialized and scheduled. For nonblocking reduce, it sizes the per-cell flag shared memory, sizes and initializes the last-completed sequence-number tracking array, initializes per-cell gather and release flags, and maps each reduce chunk sequence number to a shared-memory cell. Together with `MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE`, it determines the reduce cell capacity used for pipelining. The associated shared-memory allocations count against `MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE`; exceeding that limit makes initialization fail and the collective fall back. When active, this CVAR sets the number of shared-memory reduce cells used by POSIX release-gather reduce collectives.

* integer - default is 4, represents the number of reduce buffer cells.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init, MPIDI_POSIX_NB_RG_rank0_hold_buf_completion, MPIDI_POSIX_NB_RG_update_release_flags_completion, MPIDI_POSIX_NB_RG_all_datacopy_cb, MPIDI_POSIX_NB_RG_gather_step_completion, MPIDI_POSIX_NB_RG_reduce_data_cb, MPIDI_POSIX_NB_RG_reduce_start_sendrecv_completion, MPIDI_POSIX_NB_RG_reduce_finish_sendrecv_completion

**MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL** - COLLECTIVE

Sets the k value used for POSIX shared-memory release-gather reduce trees for normal-size reduce messages. It is captured when blocking release-gather state is initialized for a communicator and is read when nonblocking release-gather tree state is initialized. Together with `MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE`, it is used to create the reduce tree; blocking reduce and allreduce use this tree when the message size is less than or equal to `MPIR_CVAR_REDUCE_INTRANODE_MSG_SIZE_THRESHOLD`, while larger messages use the large-message reduce tree CVARs. Nonblocking reduce uses this CVAR's k value when its release-gather state is initialized. Topology-aware tree creation is attempted only when `MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES` is enabled, user-provided process binding is present, and hardware topology support is initialized, otherwise the non-topology-aware tree is created. When active, this CVAR sets the reduce tree radix used by POSIX release-gather reduce collectives.

* integer - default is 4, represents the reduce tree k value.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init

**MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL_LARGE** - COLLECTIVE

Sets the k value used for POSIX shared-memory release-gather reduce trees for large reduce messages. It is captured when blocking release-gather state is initialized for a communicator. Together with `MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE_LARGE`, it is used to create the large-message reduce tree; blocking reduce and allreduce use this tree when the message size is larger than `MPIR_CVAR_REDUCE_INTRANODE_MSG_SIZE_THRESHOLD`. Topology-aware tree creation may be attempted for the normal broadcast and reduce trees when `MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES` is enabled, user-provided process binding is present, and hardware topology support is initialized, but the large-message reduce tree is created with the configured large-message tree type and this k value. When active, this CVAR sets the large-message reduce tree radix used by POSIX release-gather reduce collectives.

* integer - default is 2, represents the large-message reduce tree k value.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init

**MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE** - COLLECTIVE

Sets the tree type used for POSIX shared-memory release-gather reduce trees for normal-size reduce messages. It is captured when blocking or nonblocking release-gather state is initialized for a communicator. Together with `MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL`, it is used to create the reduce tree; blocking reduce and allreduce use this tree when the message size is less than or equal to `MPIR_CVAR_REDUCE_INTRANODE_MSG_SIZE_THRESHOLD`, while larger messages use the large-message reduce tree CVARs. Nonblocking reduce uses this CVAR's tree type when its release-gather state is initialized. Topology-aware tree creation is attempted only when `MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES` is enabled, user-provided process binding is present, and hardware topology support is initialized, otherwise the non-topology-aware tree is created. When active, this CVAR selects the reduce tree shape used by POSIX release-gather reduce collectives.

* kary - default, use a k-ary tree type.
* knomial_1 - use a knomial tree type where ranks are added in order from the left side.
* knomial_2 - use a knomial tree type where ranks are added in order from the right side; supported only with non-topology-aware trees.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init, MPIDI_POSIX_nb_release_gather_comm_init

**MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE_LARGE** - COLLECTIVE

Sets the tree type used for POSIX shared-memory release-gather reduce trees for large reduce messages. It is captured when blocking release-gather state is initialized for a communicator. Together with `MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL_LARGE`, it is used to create the large-message reduce tree; the CVAR block in `src/mpid/ch4/shm/posix/release_gather/release_gather.c` states that this large-message tree configuration is used when the message size is larger than `MPIR_CVAR_REDUCE_INTRANODE_MSG_SIZE_THRESHOLD`. Topology-aware tree creation may be attempted for the normal broadcast and reduce trees when `MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES` is enabled, user-provided process binding is present, and hardware topology support is initialized, but the large-message reduce tree is created with this configured tree type and the large-message k value. When active, this CVAR selects the large-message reduce tree shape used by POSIX release-gather reduce collectives.

* kary - default, use a k-ary tree type.
* knomial_1 - use a knomial tree type where ranks are added in order from the left side.
* knomial_2 - use a knomial tree type where ranks are added in order from the right side; supported only with non-topology-aware trees.

Used in functions: MPIDI_POSIX_mpi_release_gather_comm_init

**MPIR_CVAR_REDUCE_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Reduce algorithm. It is used for `MPI_Reduce` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`; selected sched-based nonblocking or automatic paths may use `MPIR_CVAR_REDUCE_SHORT_MSG_SIZE` for size-based algorithm selection. When active, this CVAR sets which Reduce implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* binomial - Forces the binomial Reduce algorithm.
* circ_graph - Forces the circulant graph Reduce algorithm.
* nb - Forces the nonblocking Reduce algorithm.
* smp - Forces the SMP Reduce algorithm.
* reduce_scatter_gather - Forces the reduce-scatter-gather Reduce algorithm.

Used in functions: MPI_Reduce

**MPIR_CVAR_REDUCE_POSIX_INTRA_ALGORITHM** - COLLECTIVE

Selects the algorithm used for POSIX shared-memory intranode reduce. It is used in the POSIX reduce implementation; automatic selection consults the POSIX collective-selection table and can fall back to the MPIR reduce implementation when no POSIX selection is found. The shared-memory release-gather path is used only when MPICH is not threaded and the reduction operation is commutative. When active, this CVAR sets whether POSIX intranode reduce uses MPIR fallback, shared-memory release-gather reduce, or internal POSIX collective selection.

* auto - default, use internal POSIX collective selection.
* mpir - use the MPIR reduce implementation.
* release_gather - use the POSIX shared-memory release-gather reduce path when threading and operation-commutativity conditions allow it.

Used in functions: MPIDI_POSIX_mpi_reduce

**MPIR_CVAR_REDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Reduce_scatter_block` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Reduce_scatter_block.

* true - default, allows the device to override MPIR-level `MPI_Reduce_scatter_block` collective algorithms.
* false - disables the device override for `MPI_Reduce_scatter_block`.

Used in functions: MPI_Reduce_scatter_block

**MPIR_CVAR_REDUCE_SCATTER_BLOCK_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Reduce_scatter_block_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Reduce_scatter_block_init.

* true - default, allows the device to override MPIR-level `MPI_Reduce_scatter_block_init` collective algorithms.
* false - disables the device override for `MPI_Reduce_scatter_block_init`.

Used in functions: MPI_Reduce_scatter_block_init

**MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Reduce_scatter_block algorithm. It is used for `MPI_Reduce_scatter_block` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which inter-communicator Reduce_scatter_block implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* remote_reduce_local_scatter - Forces the remote-reduce-local-scatter algorithm.

Used in functions: MPI_Reduce_scatter_block

**MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Reduce_scatter_block algorithm. It is used for `MPI_Reduce_scatter_block` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Reduce_scatter_block implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* noncommutative - Forces the noncommutative algorithm.
* recursive_doubling - Forces the recursive-doubling algorithm.
* pairwise - Forces the pairwise algorithm.
* recursive_halving - Forces the recursive-halving algorithm.
* nb - Forces the nonblocking algorithm.

Used in functions: MPI_Reduce_scatter_block

**MPIR_CVAR_REDUCE_SCATTER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Reduce_scatter` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Reduce_scatter.

* true - default, allows the device to override MPIR-level `MPI_Reduce_scatter` collective algorithms.
* false - disables the device override for `MPI_Reduce_scatter`.

Used in functions: MPI_Reduce_scatter

**MPIR_CVAR_REDUCE_SCATTER_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Reduce_scatter_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Reduce_scatter_init.

* true - default, allows the device to override MPIR-level `MPI_Reduce_scatter_init` collective algorithms.
* false - disables the device override for `MPI_Reduce_scatter_init`.

Used in functions: MPI_Reduce_scatter_init

**MPIR_CVAR_REDUCE_SCATTER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Reduce_scatter algorithm. It is used for `MPI_Reduce_scatter` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Reduce_scatter implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* remote_reduce_local_scatter - Forces the remote-reduce-local-scatter algorithm.

Used in functions: MPI_Reduce_scatter

**MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Reduce_scatter algorithm. It is used for `MPI_Reduce_scatter` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Reduce_scatter implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* noncommutative - Forces the noncommutative algorithm.
* pairwise - Forces the pairwise algorithm.
* recursive_doubling - Forces the recursive-doubling algorithm.
* recursive_halving - Forces the recursive-halving algorithm.

Used in functions: MPI_Reduce_scatter

**MPIR_CVAR_REDUCE_SHORT_MSG_SIZE** - COLLECTIVE

Sets the Reduce short-message threshold, in bytes, used by sched-auto intra-communicator Ireduce algorithm selection. When active, messages above this threshold select reduce-scatter followed by gather only when the operation is built-in and the count is at least the nearest lower power of two of the communicator size; otherwise the binomial tree algorithm is selected. This CVAR is bypassed when the parent-communicator SMP path is selected for a commutative operation.

* integer - default is 2048, represents the Reduce short-message threshold in bytes.

Used in functions: MPIR_Ireduce_intra_sched_auto

**MPIR_CVAR_REQUEST_BATCH_SIZE** - REQUEST

Sets how many request entries MPICH processes at a time when handling all-request completion routines. It is used by `MPI_Testall` and `MPI_Waitall` to split the request array into batches before computing request properties, calling the device testall or waitall path, and processing completed requests. It does not rely on another CVAR or mode being active. When active, this CVAR sets the request-array batch size used by those all-request completion paths.

* 64 - default, process up to 64 request entries in each batch.
* positive integer - process up to the requested number of request entries in each batch.

Used in functions: MPIR_Testall, MPIR_Waitall

**MPIR_CVAR_REQUEST_ERR_FATAL** - REQUEST

Controls whether MPICH returns a request's own error code immediately instead of aggregating request errors as `MPI_ERR_IN_STATUS` in all/some request completion routines. It is used when `MPI_Testall`, `MPI_Testsome`, `MPI_Waitall`, and `MPI_Waitsome` process completed requests that contain errors, and when nonblocking schedule entries fail while they are being started. The `MPI_Testall` request-error check is compiled under error-checking support; the nonblocking schedule path records collective error state before deciding whether to return immediately. When active, this CVAR enables immediate propagation of the underlying request or schedule-entry error code.

* false - default, report request errors through `MPI_ERR_IN_STATUS` for the all/some completion routines and allow nonblocking schedules to record the failure and continue progressing.
* true - return the underlying request or schedule-entry error code immediately.

Used in functions: MPIDU_Sched_start_entry, MPIR_Testall, MPIR_Testsome, MPIR_Waitall, MPIR_Waitsome

**MPIR_CVAR_REQUEST_POLL_FREQ** - REQUEST

Sets how frequently MPICH polls progress while scanning request arrays in the MPIR-level `MPI_Waitany` and `MPI_Waitsome` wait loops. It is used only in the state routines called by the device waitany/waitsome paths, after active requests have been filtered by the public wait routines; requests with generalized-request poll functions may also be polled during the same scan. The waitsome state routine also performs one progress poll before entering the scan loop. When active, this CVAR sets the interval, in processed request positions, at which MPICH calls the progress engine while looking for completed requests.

* 8 - default, poll progress after every 8 processed request positions while scanning.
* positive integer - poll progress after every requested number of processed request positions while scanning.

Used in functions: MPIR_Waitany_state, MPIR_Waitsome_state

**MPIR_CVAR_SCAN_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Scan` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Scan.

* true - default, allows the device to override MPIR-level `MPI_Scan` collective algorithms.
* false - disables the device override for `MPI_Scan`.

Used in functions: MPI_Scan

**MPIR_CVAR_SCAN_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Scan_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Scan_init.

* true - default, allows the device to override MPIR-level `MPI_Scan_init` collective algorithms.
* false - disables the device override for `MPI_Scan_init`.

Used in functions: MPI_Scan_init

**MPIR_CVAR_SCAN_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Scan algorithm. It is used for `MPI_Scan` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`. When active, this CVAR sets which Scan implementation is forced or whether internal collective selection chooses the implementation.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* nb - Forces the nonblocking algorithm.
* smp - Forces the SMP algorithm.
* recursive_doubling - Forces the recursive-doubling algorithm.

Used in functions: MPI_Scan

**MPIR_CVAR_SCATTERV_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Scatterv` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Scatterv.

* true - default, allows the device to override MPIR-level `MPI_Scatterv` collective algorithms.
* false - disables the device override for `MPI_Scatterv`.

Used in functions: MPI_Scatterv

**MPIR_CVAR_SCATTERV_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Scatterv_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Scatterv_init.

* true - default, allows the device to override MPIR-level `MPI_Scatterv_init` collective algorithms.
* false - disables the device override for `MPI_Scatterv_init`.

Used in functions: MPI_Scatterv_init

**MPIR_CVAR_SCATTERV_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Scatterv algorithm. It is used for `MPI_Scatterv` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* linear - Forces the linear Scatterv algorithm.
* nb - Forces the nonblocking Scatterv algorithm.

Used in functions: MPI_Scatterv

**MPIR_CVAR_SCATTERV_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Scatterv algorithm. It is used for `MPI_Scatterv` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* linear - Forces the linear Scatterv algorithm.
* nb - Forces the nonblocking Scatterv algorithm.

Used in functions: MPI_Scatterv

**MPIR_CVAR_SCATTER_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Scatter` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Scatter.

* true - default, allows the device to override MPIR-level `MPI_Scatter` collective algorithms.
* false - disables the device override for `MPI_Scatter`.

Used in functions: MPI_Scatter

**MPIR_CVAR_SCATTER_INIT_DEVICE_COLLECTIVE** - COLLECTIVE

Controls whether `MPI_Scatter_init` allows the device to override MPIR-level collective algorithms. This CVAR is used only when `MPIR_CVAR_DEVICE_COLLECTIVES` is set to `percoll`; even when device override is allowed, the device may still call MPIR-level algorithms manually. When active, it enables or disables the device-override path for Scatter_init.

* true - default, allows the device to override MPIR-level `MPI_Scatter_init` collective algorithms.
* false - disables the device override for `MPI_Scatter_init`.

Used in functions: MPI_Scatter_init

**MPIR_CVAR_SCATTER_INTER_ALGORITHM** - COLLECTIVE

Selects the inter-communicator Scatter algorithm. It is used for `MPI_Scatter` on inter-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* linear - Forces the linear Scatter algorithm.
* nb - Forces the nonblocking Scatter algorithm.
* remote_send_local_scatter - Forces the remote-send-local-scatter algorithm.

Used in functions: MPI_Scatter

**MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE** - COLLECTIVE

Sets the short-message threshold, in bytes, used by sched-auto nonblocking inter-communicator Scatter. It is used when the sched-based inter-communicator Iscatter automatic path is selected, such as by forcing `MPIR_CVAR_ISCATTER_INTER_ALGORITHM` to `sched_auto` or by internal collective selection. The total message size is computed from the send datatype, send count, and remote group size on the root side, or from the receive datatype, receive count, and local group size on the remote side; messages below this threshold use the remote-send-local-scatter schedule, and other messages use the linear schedule.

* integer - default is 2048, represents the inter-communicator Scatter short-message threshold in bytes.

Used in functions: MPIR_Iscatter_inter_sched_auto

**MPIR_CVAR_SCATTER_INTRA_ALGORITHM** - COLLECTIVE

Selects the intra-communicator Scatter algorithm. It is used for `MPI_Scatter` on intra-communicators when the collective algorithm is chosen directly through this CVAR or through internal collective selection. In automatic mode, MPICH performs internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.

* auto - default, uses internal algorithm selection, which can be overridden with `MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE`.
* binomial - Forces the binomial Scatter algorithm.
* nb - Forces the nonblocking Scatter algorithm.

Used in functions: MPI_Scatter

**MPIR_CVAR_SHM_RANDOM_ADDR_RETRY** - MEMORY

Sets the retry limit for generating a random virtual address for symmetric shared-memory allocation. It is used only in the symmetric heap shared-memory allocation path built with `USE_SYM_HEAP`; within each symmetric heap mapping attempt controlled by `MPIR_CVAR_SHM_SYMHEAP_RETRY`, the rank with the largest requested segment generates the candidate address and broadcasts it to the other processes. When active, this CVAR sets how many random address candidates are retried locally before address generation gives up for that mapping attempt.

* 100 - default, retry up to 100 random address candidates.
* positive integer - retry up to the specified number of random address candidates.

Used in functions: generate_random_addr

**MPIR_CVAR_SHM_SYMHEAP_RETRY** - MEMORY

Sets the retry limit for collectively allocating a symmetric shared-memory heap at the same virtual address across all processes in a communicator. It is used only in the symmetric heap shared-memory allocation path built with `USE_SYM_HEAP`; the rank with the largest requested segment generates each candidate address using `MPIR_CVAR_SHM_RANDOM_ADDR_RETRY`, broadcasts it to the communicator, and all processes attempt to map the shared-memory segment at the computed address. When active, this CVAR sets how many symmetric shared-memory mapping attempts are retried before allocation fails.

* 100 - default, retry up to 100 symmetric shared-memory mapping attempts.
* positive integer - retry up to the specified number of symmetric shared-memory mapping attempts.
* 0 or negative integer - do not attempt symmetric shared-memory mapping.

Used in functions: shm_alloc_symm_all

**MPIR_CVAR_SINGLE_HOST_ENABLED** - DEVELOPER

Controls whether CH4 OFI treats a single-node launch as a single-host run for provider preference. It is used during runtime OFI provider selection, after libfabric providers have been discovered and while each candidate provider is scored. It does not rely on another CVAR value, but it is effective only when OFI runtime checks are enabled and MPICH has detected that all processes are on one node. When active, this CVAR enables deprioritizing the `cxi` provider so single-host jobs avoid using scarce CXI hardware resources.

* true - default, deprioritize the `cxi` provider for single-node runs.
* false - do not apply the single-host `cxi` provider preference adjustment.

Used in functions: provider_preference

**MPIR_CVAR_SUPPRESS_ABORT_MESSAGE** - ERROR_HANDLING

Controls whether MPICH constructs the abort error message passed to the device abort path. It is used in `MPIR_Abort_impl` after selecting the communicator and resolving its name, and it does not rely on any other CVAR or mode being active. When active, this CVAR enables suppression of the abort error message.

* false - default, construct an abort error message that identifies the communicator, error code, and process rank.
* true - pass an empty abort error message to the device abort path.

Used in functions: MPIR_Abort_impl

**MPIR_CVAR_UCX_DT_RECV** - CH4_UCX

Selects the receive method for noncontiguous datatypes in the CH4 UCX receive path. It is used after datatype layout is checked; contiguous receives bypass this setting. When the UCX datatype receive path is not selected, MPICH may use UCX IOV receives when UCX IOV support is available and the datatype density meets `MPIR_CVAR_CH4_IOV_DENSITY_MIN`, otherwise it receives into a pack buffer and unpacks at completion. When active, this CVAR enables UCX datatype receives with pack/unpack callbacks for noncontiguous data.

* false - default, let MPICH choose between UCX IOV receives and receive-then-unpack handling based on datatype density and UCX IOV support.
* true - use UCX datatype receives with pack/unpack callbacks for noncontiguous data.

Used in functions: MPIDI_UCX_recv

**MPIR_CVAR_YAKSA_COMPLEX_SUPPORT** - COLLECTIVE

Controls whether yaksa-based reductions are allowed for complex datatypes. It is used when MPICH checks whether a reduction operation can use yaksa; this check first requires `MPIR_CVAR_ENABLE_YAKSA_REDUCTION` to be enabled and, for nonzero counts, the packed data size must not exceed a positive `MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD`. When active, this CVAR enables complex datatype reductions to be accepted for the yaksa reduction path when the remaining operation and datatype support checks pass.

* 0 - default, treat complex datatype reductions as unsupported by yaksa.
* nonzero - allow complex datatype reductions to use yaksa when the other yaksa reduction support checks pass.

Used in functions: MPIR_Typerep_reduce_is_supported

**MPIR_CVAR_YAKSA_REDUCTION_THRESHOLD** - COLLECTIVE

Sets the maximum packed data size for using Yaksa-based local reduction when Yaksa reduction support is queried with a positive count. It is effective only when `MPIR_CVAR_ENABLE_YAKSA_REDUCTION` is enabled and only for calls that pass a positive count to the support check; calls that pass count zero do not use this threshold decision.

* -1 - default, does not limit Yaksa-based reduction by message size.
* 0 - does not limit Yaksa-based reduction by message size.
* positive integer - disables Yaksa-based reduction for messages whose packed data size is above the threshold.

Used in functions: MPIR_Typerep_reduce_is_supported

