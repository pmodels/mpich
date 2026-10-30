# CH4 Progress Design

MPICH makes progress only when a thread calls into it: a test or wait on a
request, a blocking MPI call, or a progress thread. This page describes how
one such call is spread over the parts of CH4 that need polling: progress
hooks (async things, nonblocking collective schedules, external collective
libraries), the shared memory module and the netmod, each per VCI. It
follows `src/mpid/ch4/src/ch4_progress.h`, where the CH4 progress engine
lives.

The ch3 device has its own progress engine, described in
[CH3 Progress Design](CH3_Progress_Design.md).

## Entry points

| Function | Used by | What it polls |
|---|---|---|
| `MPID_Progress_test(state)` | everything below | the VCIs and parts in `state`; `NULL` means every implicit VCI, every part |
| `MPID_Progress_wait(state)` | blocking loops (`MPIR_Wait_state`, ...) | `MPID_Progress_test(state)`, then a yield. It never blocks. |
| `MPID_Progress_poke()` | | `MPID_Progress_test(NULL)` |
| `MPID_Progress_start(state)` / `_end` | the legacy `MPIR_*_impl` waits | `start` fills `state` for global progress; `end` does nothing |
| `MPID_Test*`, `MPID_Wait*` (`ch4_wait.h`) | `MPI_Test`, `MPI_Wait` and their array forms | only the VCIs of the requests (`MPIDI_set_progress_vci`, `_vci_n`) |
| `MPIDI_progress_test_vci(vci)` | code that waits while holding a VCI lock (`MPIDIU_PROGRESS_WHILE`, RMA) | one VCI, or global progress after releasing the lock |
| `MPIDI_Progress_test(flags)` | | every implicit VCI, only the parts in `flags` |
| `MPID_Stream_progress(stream)` | progress threads | the stream's VCI (and its GPU work queue), or everything for `NULL` |

A blocking wait is a loop around the test:

```c
int MPIR_Wait_state(MPIR_Request * request_ptr, MPI_Status * status, MPID_Progress_state * state)
{
    while (!MPIR_Request_is_complete(request_ptr)) {
        mpi_errno = MPID_Progress_wait(state);    /* test + yield */
        ...
    }
}
```

## Progress state

```c
typedef struct {
    int flag;
    int vci_count;              /* number of vcis that need progress */
    uint8_t vci[MPIDI_CH4_MAX_VCIS];    /* list of vcis that need progress */
} MPID_Progress_state;
```

`flag` (`ch4_types.h`) selects the parts to poll:

| Flag | Part |
|---|---|
| `MPIDI_PROGRESS_HOOKS` | progress hooks |
| `MPIDI_PROGRESS_NM` | the netmod |
| `MPIDI_PROGRESS_SHM` | the shared memory module |
| `MPIDI_PROGRESS_ALL` | all three |
| `MPIDI_PROGRESS_NM_LOCKLESS` | poll the netmod without the VCI lock (lockless MT model) |

`MPIDI_progress_state_init` sets `MPIDI_PROGRESS_ALL` (plus
`MPIDI_PROGRESS_NM_LOCKLESS` under `MPIDI_CH4_MT_LOCKLESS`) and lists every
implicit VCI, `0 .. n_vcis - 1`. `MPIDI_set_progress_vci` lists the VCI of
one request; `MPIDI_set_progress_vci_n` the distinct VCIs of the active,
incomplete requests in an array.

## `MPIDI_progress_test`

One call goes through these steps, in order:

1. **Failed processes.** If a `SIGUSR1` arrived since the last call,
   `MPIDI_check_for_failed_procs`.
2. **Thread communicators** (`ENABLE_THREADCOMM`): `MPIR_Threadcomm_progress`.
   If it made progress, return.
3. **Progress hooks** (`MPIDI_PROGRESS_HOOKS`): first the global hooks
   (`MPIR_Progress_hook_exec_all(-1, ...)`), then the hooks of each VCI in
   `state`. Return as soon as one made progress.
4. **SHM and netmod, per VCI.**
   - With `MPIDI_CH4_MAX_VCIS == 1` (a build without multiple VCIs), poll VCI 0.
   - Otherwise either do global progress (every implicit VCI) or poll only
     the VCIs in `state`; see [Per-VCI and global progress](#per-vci-and-global-progress).
   - For each VCI, `MPIDI_PROGRESS(vci, is_global)` polls `MPIDI_SHM_progress`
     under the VCI lock, then `MPIDI_NM_progress` (under the VCI lock unless
     the VCI is explicit or the MT model is lockless). Outside global
     progress, a part is skipped once something made progress in this
     call, so the netmod is not polled when shm made progress.
5. **Throttle** (`MPIR_CVAR_CH4_PROGRESS_THROTTLE`): after
   `MPIR_CVAR_CH4_PROGRESS_THROTTLE_NO_PROGRESS_COUNT` consecutive calls
   without progress, yield the thread (`MPID_Thread_yield`) on each further
   call until one makes progress. This keeps a high-PPN node from
   saturating the shared memory queues with polling.

Hooks come first and return early: while a nonblocking collective or an
async thing makes progress, a call does not reach shm or the netmod. The
communication those hooks wait for is still polled, by the next call.

`made_progress` drives the early returns and the throttle, so it matters
which parts report it. The POSIX shm module reports a received message or
a drained send. The netmods do not report progress at present: OFI never
sets it, and UCX only through UCC (`MPIDI_common_ucc_progress`).

## Per-VCI and global progress

### VCIs

A VCI (virtual communication interface) is an independent set of
communication resources (netmod endpoints and completion queues, shm queues,
a request pool and a lock, `MPIDI_VCI_LOCK(vci)`), so threads using
different VCIs do not contend. `MPIDI_global` (`ch4_types.h`) counts them:

| Range | Kind | Chosen by |
|---|---|---|
| `0 .. n_vcis - 1` | implicit | hashing (`MPIDI_get_comm_vci`, the pt2pt hashing in `ch4_vci.h`), sized by `MPIR_CVAR_CH4_NUM_VCIS` |
| `n_vcis .. n_total_vcis - 1` | reserved (explicit) | `MPID_Allocate_vci`, for MPIX streams; sized by `MPIR_CVAR_CH4_RESERVE_VCIS` |

The netmod may create fewer VCIs than requested (`MPIDI_Comm_set_vcis`).
A request's VCI is its pool index (`MPIDI_Request_get_vci`). A request
created for a communicator, e.g. for a nonblocking collective, comes from
the communicator's VCI (`MPID_Request_create_from_comm`).

`MPIDI_VCI_IS_EXPLICIT(vci)` is true for a reserved VCI unless reserved
VCIs are shared (`share_reserved_vcis`, set when a GPU stream allocates a
shared VCI). An explicit VCI is used by a single serial context (its
stream), so its netmod progress takes no lock, and it never triggers
global progress.

### Per-VCI progress

`MPI_Test` and `MPI_Wait` (through `MPID_Test*` / `MPID_Wait*`) poll only the
VCIs of their requests. With many threads each on its own VCI, a wait does
not touch, or lock, the other VCIs.

### Global progress

Polling only the request's VCI is not always enough: the messages that
complete a request may arrive on another VCI. For example, MPIDIG active
messages for RMA use VCI 0 (`MPIDIG_wait_am_acc`, `ch4_impl.h`). So, every
so often, a call does *global progress*: it polls every implicit VCI
(`0 .. n_vcis - 1`) instead of those in `state`.

`MPIDI_do_global_progress` decides, with a thread-local counter
(`global_vci_poll_count`) and a mask chosen by `MPIR_CVAR_CH4_GLOBAL_PROGRESS`:

| Value | Mask | Global progress |
|---|---|---|
| `normal` (default) | `0xff` | every 256 calls |
| `low` | `0xfffffff` | every 2^28 calls (minutes) |
| `high` | `0x1` | every other call |

There is no global progress with a single implicit VCI (`n_vcis == 1`), or
when `state` holds a single explicit VCI. In global progress every VCI is
polled even after another one made progress (`is_global`).

### Progress inside a VCI lock

Code that waits while holding a VCI lock uses `MPIDI_progress_test_vci(vci)`:
MPIDIG RMA synchronization (`mpidig_win.h`) through
`MPIDIU_PROGRESS_WHILE(cond, vci)` / `MPIDIU_PROGRESS_DO_WHILE`, which also
yield the global and VCI locks between iterations, and MPIDIG partitioned
communication (`mpidig_part.h`) directly. `MPIDI_progress_test_vci`
polls the netmod and shm of that VCI directly, without hooks. When it is
time for global progress (and the VCI is not explicit), it releases the VCI
lock, calls `MPID_Progress_test(NULL)` and takes the lock again, since
global progress takes every VCI lock in turn.

### Locks and thread granularity

- With `--enable-thread-cs=global`, one global lock covers every MPI call,
  and `MPID_Progress_wait` yields it (`MPIDI_PROGRESS_YIELD`) so other
  threads can enter.
- With `--enable-thread-cs=per-vci` (`MPICH_THREAD_GRANULARITY__VCI`), each
  part is polled under its VCI lock as above, and `MPID_Progress_wait`
  yields the thread.
- The MT model (`MPIDI_CH4_MT_MODEL`: `direct` or `lockless`, see
  `mpidpre.h`) decides whether netmod progress takes the VCI lock.
- Progress hooks and async things have their own locks (below).

## Progress hooks

Progress hooks (`src/util/mpir_progress_hook.c`) let any component be polled
from the device's progress without the device knowing about it. Each slot
(one per VCI, plus a global one, `vci == -1`) holds up to 10 hooks:

```c
int MPIR_Progress_hook_register(int vci, int (*progress_fn) (int vci, int *made_progress), int *id);
int MPIR_Progress_hook_deregister(int id);
int MPIR_Progress_hook_activate(int id);
int MPIR_Progress_hook_deactivate(int id);
int MPIR_Progress_hook_exec_all(int vci, int *made_progress);
```

A component registers its hook once (at init), activates it when it has
pending work, and deactivates it when the work is done, so an idle hook
costs one atomic load per call. Activation is a relaxed flag: a hook
deactivated by another thread may be polled once more, which is harmless.
The hook itself protects its own state.

Hooks registered in CH4 builds:

| Hook | Slot | Registered in | Active while |
|---|---|---|---|
| `MPIR_Async_things_progress` | each VCI and global | `src/util/mpir_async_things.c` | the slot's list of async things is not empty |
| `MPIDU_Sched_progress` | global | `src/mpi/coll/src/coll_impl.c` | MPIDU schedules are pending |
| `MPII_Genutil_progress_hook` | global | `src/mpi/coll/transports/gentran/gentran_impl.c` | gentran schedules are queued |
| `hcoll_do_progress` | global | `src/mpid/common/hcoll/hcoll_init.c` | hcoll is enabled |
| `mpidi_ucc_progress` | global | `src/mpid/common/ucc/mpid_ucc.c` | UCC is enabled |

## Async things

Async things (`src/util/mpir_async_things.c`) are the general mechanism
for work that needs polling: each *thing* is a poll function and its state.
Users add them with `MPIX_Async_start(poll_fn, extra_state, stream)`, and
MPICH itself uses them, e.g. greq collective algorithms such as `ibcast`
`circ_graph` ([Collective Schedules](Collective_Schedules.md)).

```c
int MPIR_Async_things_add(int (*poll_fn) (struct MPIR_Async_thing * entry), void *state,
                          MPIR_Stream * stream_ptr);
```

- Things live in one list per VCI plus a global list. A thing added with a
  stream goes to the stream's VCI, so only progress on that VCI (e.g. the
  stream's progress thread) polls it; without a stream it goes to the
  global list, polled by every progress call that runs hooks.
- Adding the first thing to a list activates that list's hook; removing the
  last one deactivates it.
- `MPIR_Async_things_progress` calls every thing's `poll_fn` in its list.
  `poll_fn` returns `MPIX_ASYNC_DONE` to be removed or
  `MPIX_ASYNC_NOPROGRESS` to stay, and may add new things with
  `MPIX_Async_spawn` (`MPIR_Async_thing_spawn`), which join the list after
  the call. The order is not defined; every thing is polled eventually.
- Each list has a mutex. A thread-local flag skips async progress when a
  `poll_fn` calls into MPI progress again (e.g. a blocking MPI call).
- `MPIR_Async_things_add_in_vci` adds a thing from inside a VCI lock: it
  releases the VCI lock around the add, so the async lock is always taken
  before a VCI lock.
- `MPIR_Async_things_finalize` polls until every list is empty.

## Nonblocking collective schedules

Both schedule engines ([Collective Schedules](Collective_Schedules.md)) are
global progress hooks, polled by every progress call that runs hooks,
whatever the VCIs in `state`.

- **MPIDU schedules** (`sched_` algorithms, `src/mpid/common/sched/mpidu_sched.c`):
  `MPIR_Sched_start` appends the schedule to `all_schedules` and activates
  `MPIDU_Sched_progress`. The hook takes `MPIDIU_THREAD_SCHED_LIST_MUTEX`,
  polls every pending schedule (`MPIDU_Sched_progress_state`), and
  deactivates itself when the list is empty. A thread-local flag skips the
  hook when a schedule callback calls into progress again (e.g.
  `MPI_Comm_idup` runs `MPIR_Allreduce` from a callback).
- **Gentran schedules** (`tsp_` algorithms, `src/mpi/coll/transports/gentran`):
  `MPIR_TSP_sched_start` queues the schedule on `MPII_coll_queue` and
  activates `MPII_Genutil_progress_hook`. The hook takes
  `MPIDIU_THREAD_TSP_QUEUE_MUTEX`, pokes every queued schedule
  (`MPII_Genutil_sched_poke`), and deactivates itself when the queue is
  empty. `MPIR_CVAR_PROGRESS_MAX_COLLS` (0: no limit) stops a call after
  that many schedules made progress. A static flag skips the hook when
  it is entered again, e.g. through netmod progress inside a send.
- **Greq algorithms** have no hook of their own; they use async things.

The point-to-point operations a schedule issues complete through shm and
netmod progress on their own VCIs, and the hook sees their requests
complete.

## SHM progress

`MPIDI_SHM_progress(vci)` (`shm/src/shm_progress.h`) calls
`MPIDI_POSIX_progress(vci)` (`shm/posix/posix_progress.h`), which handles at
most one message each way per call:

- **Receive** (`MPIDI_POSIX_progress_recv`): take one message from the
  VCI's eager queue (`MPIDI_POSIX_eager_recv_begin`, e.g. the iqueue
  module). A first segment runs the MPIDIG active message handler
  (`MPIDIG_global.target_msg_cbs`); a pipelined message keeps the receive
  request in `active_rreq[src_local_rank]` and copies later fragments into
  it, completing it after the last one. Sets `made_progress`.
- **Send** (`MPIDI_POSIX_progress_send`): retry the first message in the
  VCI's postponed queue, which holds sends that found the queue full. Sets
  `made_progress` if it went out.

The shm module is polled before the netmod, under the VCI lock. Other
shared memory paths (IPC, the release-gather collectives) have no polling
of their own here.

## Netmod progress

- **OFI** (`netmod/ofi/ofi_progress.h`): `MPIDI_NM_progress(vci)` returns at
  once for a VCI the netmod does not have (global progress polls every VCI).
  It first handles completion entries buffered earlier, if any; otherwise
  it reads up to `MPIDI_OFI_NUM_CQ_ENTRIES` entries from the completion
  queue of each NIC of the VCI (`fi_cq_read`) and dispatches each to its
  request (`MPIDI_OFI_dispatch_optimized`), then retries the first deferred
  active message send of the VCI.
- **UCX** (`netmod/ucx/ucx_progress.h`): `ucp_worker_progress` on the VCI's
  worker, which runs the completion callbacks, then UCC progress if UCC is
  built in.

Neither sets `made_progress` from its own completions (see
[`MPIDI_progress_test`](#mpidi_progress_test)).

## Progress threads and streams

- `MPIR_CVAR_ASYNC_PROGRESS=1` (with `MPI_THREAD_MULTIPLE`) starts one
  progress thread at init (`MPID_Init_async_thread`, `src/mpi/init/init_async.c`).
  `MPIX_Start_progress_thread(stream)` starts one for a stream (one per VCI);
  `MPIR_CVAR_PROGRESS_THREAD_AFFINITY` pins them.
- A progress thread loops on `MPID_Stream_progress(stream)` under the global
  lock, yielding it between iterations, until it is stopped.
- `MPID_Stream_progress(NULL)` is `MPID_Progress_test(NULL)`. For a stream,
  it polls only the stream's VCI with every part; for a GPU stream it also
  issues the queued stream operations before
  (`MPIDU_stream_workq_progress_ops`) and checks the waiting ones after
  (`MPIDU_stream_workq_progress_wait_list`).

## Related

- [CH3 Progress Design](CH3_Progress_Design.md)
- [CH4 Overall Design](CH4_Overall_Design.md)
- [Schedules of Nonblocking and Persistent Collectives](Collective_Schedules.md)
- [Making MPICH Thread Safe](Making_MPICH_Thread_Safe.md)
