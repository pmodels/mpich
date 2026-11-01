# Schedules of Nonblocking and Persistent Collectives

A nonblocking or persistent collective builds an operation object when it is
called, and runs it later: once for a nonblocking call, at every `MPI_Start`
for a persistent one. This page describes the three kinds of operation
objects, what they own, and how long they live.

## Operation kinds

After algorithm selection, `coll_sig->sched` holds the operation object and
`coll_sig->sched_type` (`enum MPIR_sched_type`) tells its kind:

| `sched_type` | Object | Built by | Run by |
|---|---|---|---|
| `MPIR_SCHED_NORMAL` | `MPIR_Sched_t` (`MPIDU_Sched`, `src/mpid/common/sched`) | `sched_` algorithms, through `MPIR_Sched_*` | the `MPIDU` schedule progress |
| `MPIR_SCHED_GENTRAN` | `MPIR_TSP_sched_t` (`MPII_Genutil_sched_t`, `src/mpi/coll/transports/gentran`) | `tsp_` algorithms, through `MPIR_TSP_sched_*` | the gentran progress |
| `MPIR_SCHED_GREQ` | `MPIR_Coll_greq_t` (`src/mpi/coll/src/coll_greq.c`) | greq algorithms, through their `_init` | the algorithm itself |

The first two are generic: the algorithm describes its steps, and a shared
engine runs them. A *greq algorithm* (grequest collective) has its own
state machine instead. It keeps its own state, makes its own progress (e.g.
with an Async thing, `MPIR_Async_things_add`), and reports completion through
a generalized request. `ibcast` `circ_graph` is a greq algorithm.

The generator (`maint/gen_coll.py`) tells the kinds apart by the algorithm
name in `coll_algorithms.txt`: in a nonblocking collective, `tsp_*` is a
gentran algorithm, `sched_*` an `MPIDU` one, and any other name a greq
algorithm.

All three kinds go through the same steps:

| Step | Where | NORMAL | GENTRAN | GREQ |
|---|---|---|---|---|
| create | `_cnt` wrapper (`dump_algo_prep`) | `MPII_CSEL_CREATE_SCHED` | `MPII_CSEL_CREATE_TSP_SCHED` | `MPII_CSEL_CREATE_GREQ` |
| build | the algorithm | adds steps | adds vertices | `_init` sets up `state` |
| hold handles | generated `_impl`, `MPIR_Coll_hold_handles` | into its `owned` | into its `owned` | into its `owned` |
| start (nonblocking) | `MPII_SCHED_START` | `MPIR_Sched_start` | `MPIR_TSP_sched_start` | `MPIR_Coll_greq_start` |
| start (persistent) | `MPIR_Persist_coll_start` | `MPIR_Sched_reset` + start | `MPIR_TSP_sched_reset` + start | `greq->start_fn` |
| free (nonblocking) | | by the engine, at completion | by the engine, at completion | when the request is freed |
| free (persistent) | `MPIR_Persist_coll_free_cb` | `MPIR_Sched_free` | `MPIR_TSP_sched_free` | `MPIR_Coll_greq_free` |

A blocking collective that runs a nonblocking algorithm (`MPIR_Coll_nb`)
builds and starts the operation the same way and waits for it.

## What an operation owns: `MPIR_Coll_owned_t`

The user may free the communicator, datatypes and op of a nonblocking or
persistent collective as soon as the call returns, and algorithms build
internal datatypes and buffers that must live as long as the operation. Every
operation object therefore embeds an `MPIR_Coll_owned_t`
(`src/mpi/coll/include/coll_types.h`, declared in `src/include/mpir_nbc.h`, implemented in
`src/mpi/coll/src/coll_owned.c`):

```c
typedef struct {
    UT_array buffers;   /* MPL_malloc'd memory, MPL_free'd */
    UT_array handles;   /* comm/datatype/op references, released */
} MPIR_Coll_owned_t;

void MPIR_Coll_owned_init(MPIR_Coll_owned_t * o);
void MPIR_Coll_owned_add_buffer(MPIR_Coll_owned_t * o, void *buf);
void MPIR_Coll_owned_add_handle(MPIR_Coll_owned_t * o, int handle);
void MPIR_Coll_owned_free(MPIR_Coll_owned_t * o);
```

- `add_buffer` takes ownership of a buffer allocated with `MPL_malloc`.
- `add_handle` takes a reference of its own on a communicator, datatype or op.
  A caller that owns a reference (e.g. a datatype it created) releases its
  own afterwards. Predefined handles are skipped. Windows are rejected,
  since freeing one is collective.
- `free` frees the buffers and releases the handles. It runs exactly once,
  when the operation object is freed, so the objects stay valid across every
  start of a persistent operation.

Each operation kind exposes its holder:

| Kind | Forwarders | Accessor |
|---|---|---|
| NORMAL | `MPIR_Sched_add_buffer`, `MPIR_Sched_alloc_state`, `MPIR_Sched_add_handle` | `MPIR_Sched_get_owned` |
| GENTRAN | `MPIR_TSP_sched_add_buffer`, `MPIR_TSP_sched_malloc`, `MPIR_TSP_sched_add_handle` | `MPIR_TSP_sched_get_owned` |
| GREQ | n/a | `&greq->owned` |

Schedule steps and gentran vertices do not take references on their
communicator, datatypes or op. They use objects that the operation owns.

### Who adds what

- **The collective's own objects.** The generated nonblocking and persistent
  `MPIR_<Coll>_impl` calls the generated `MPIR_Coll_hold_handles(&coll_sig)`
  after selection (`MPIR_Coll_json`), when the operation is built but not
  yet started. It finds the holder by `sched_type` and adds the handles of
  the coll_sig to it.
- **Datatypes an algorithm creates.** The algorithm adds them right after
  creating them and releases its own reference, e.g.

  ```c
  mpi_errno = MPIR_Type_vector_impl(..., &newtype);
  MPIR_ERR_CHECK(mpi_errno);
  mpi_errno = MPIR_Type_commit_impl(&newtype);
  MPIR_ERR_CHECK(mpi_errno);
  MPIR_Sched_add_handle(s, newtype);
  MPIR_Datatype_release_if_not_builtin(newtype);
  /* newtype stays valid for the steps: the schedule holds it */
  ```

  `MPIR_Type_free_impl` would also set `newtype` to `MPI_DATATYPE_NULL`.
  Releasing right away leaves the algorithm nothing to free if the build
  fails later.
- **Schedules built outside the collective entry points**, e.g.
  `MPIR_Isendrecv_impl`, add the objects their steps use.
- **The communicator a schedule is started with** is also held by the
  request (`req->comm`). Internal communicators of a communicator
  (`local_comm`, `node_comm`, ...) live as long as it does.

### `MPIR_Coll_hold_handles`

Generated from the MPI function parameters. It adds the communicator, then
every datatype and op the collective uses at this process. It has cases only
for the nonblocking `coll_type`s, which persistent collectives use too; a
blocking collective never holds its handles, and any other type asserts. A
handle that the collective ignores at this process is not validated and may be
garbage, so it is skipped:

| Skipped | When |
|---|---|
| send types | `sendbuf == MPI_IN_PLACE` |
| receive types | `recvbuf == MPI_IN_PLACE` |
| root-only parameters (`root_only=True`) | intracommunicator: `rank != root`; intercommunicator: `root != MPI_ROOT` |
| the non-root side of a rooted collective | intercommunicator: `root == MPI_ROOT` or `MPI_PROC_NULL` |
| everything but the communicator | intercommunicator: `root == MPI_PROC_NULL` |
| entries of a type array (`alltoallw`, `neighbor_alltoallw`) | the entry's count is 0 |

An algorithm must not use the type of an empty entry in a step, since the schedule does not hold
it and the user may have freed it. The algorithms of `alltoallw` and `neighbor_alltoallw` use
`MPIR_BYTE_INTERNAL` for an entry with count 0.

Type arrays have `comm_size` entries (the remote size on an
intercommunicator), or `indegree`/`outdegree` entries for neighborhood
collectives.

## `sched_` algorithms (MPIDU schedules)

### Interface

A `sched_` algorithm takes the collective's arguments and an `MPIR_Sched_t`,
and appends entries to it:

```c
int MPIR_Ibarrier_intra_sched_recursive_doubling(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    ...
    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;

        mpi_errno = MPIR_Sched_send(NULL, 0, MPIR_BYTE_INTERNAL, dst, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Sched_recv(NULL, 0, MPIR_BYTE_INTERNAL, src, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        MPIR_ERR_CHECK(mpi_errno);

        mask <<= 1;
    }
    ...
}
```

- The schedule is an ordered list of entries: `MPIR_Sched_send`,
  `MPIR_Sched_recv` (`_recv_status` also keeps the status), `MPIR_Sched_send_defer`
  (the count is read when the entry starts), `MPIR_Sched_reduce`,
  `MPIR_Sched_copy`, and `MPIR_Sched_cb` / `MPIR_Sched_cb2` (a callback, e.g. to
  compute the next step from received data).
- `MPIR_Sched_barrier(s)` marks the last entry as a barrier: no entry after it
  starts until every entry up to it has completed. The entries between two
  barriers start together, so the schedule runs as a sequence of rounds.
- Every send and receive uses the schedule's tag. `MPII_CSEL_CREATE_SCHED`
  takes it (`MPIR_Sched_next_tag`) when the schedule is created.
- Scratch memory comes from `MPIR_Sched_alloc_state`, or is handed over with
  `MPIR_Sched_add_buffer`. Both go into `owned`.
- `MPIR_Sched_*` are macros for `MPIDU_Sched_*` (each device's `mpid_sched.h`).

### Engine (`src/mpid/common/sched/mpidu_sched.c`)

- `MPIR_Sched_start` creates the request (which holds the communicator),
  starts the entries up to the first barrier (`MPIDU_Sched_continue`), appends
  the schedule to the list of pending schedules and activates the NBC progress
  hook.
- `MPIDU_Sched_progress`, the progress hook, polls every pending schedule. An
  entry completes when its request does; `s->idx` moves past the completed
  entries in order, and passing a completed barrier starts the next round.
  When `s->idx` reaches the end, the request is completed, and a regular
  schedule is freed (`MPIDU_Sched_free`).
- A persistent schedule (`MPIR_SCHED_KIND_PERSISTENT`) stays. At every
  `MPI_Start`, `MPIR_Sched_reset` marks every entry not started and rewinds
  `s->idx`; the tag is kept. `MPIR_Persist_coll_free_cb` frees it with
  `MPIR_Sched_free`.

## `tsp_` algorithms (gentran)

### Interface

A `tsp_` algorithm takes the collective's arguments and an
`MPIR_TSP_sched_t`, and adds the vertices of a dependency graph. Each
`MPIR_TSP_sched_*` call takes the vertices it depends on (`n_in_vtcs`,
`in_vtcs`) and returns the id of its own vertex:

```c
int MPIR_TSP_Ibarrier_sched_intra_k_dissemination(MPIR_Comm * comm, int k, MPIR_TSP_sched_t sched)
{
    ...
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);
    ...
    for (i = 0; i < nphases; i++) {
        for (j = 1; j < k; j++) {
            to = (rank + j * shift) % nranks;
            from = ...;
            /* no dependency: all receives are posted at the start */
            mpi_errno = MPIR_TSP_sched_irecv(NULL, 0, MPIR_BYTE_INTERNAL, from, tag, comm, sched,
                                             0, NULL, &recv_ids[i * (k - 1) + j - 1]);
            MPIR_ERR_CHECK(mpi_errno);
            /* the sends of phase i wait for the receives of the earlier phases */
            mpi_errno = MPIR_TSP_sched_isend(NULL, 0, MPIR_BYTE_INTERNAL, to, tag, comm, sched,
                                             i * (k - 1), recv_ids, &vtx_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
        shift *= k;
    }
    ...
}
```

- Vertex kinds: `MPIR_TSP_sched_isend`, `_irecv` (`_irecv_status`), `_imcast`
  (one buffer to several ranks), `_reduce_local`, `_localcopy`, `_cb`, a
  generic kind registered with `MPIR_TSP_sched_new_type`, and a nested
  schedule (`MPIR_TSP_sched_sub_sched`).
- `MPIR_TSP_sched_sink` adds a vertex that depends on every earlier vertex
  that nothing depends on yet, and `_selective_sink` one that depends on a
  given set; both join branches of the graph. `MPIR_TSP_sched_fence` is a
  sink that every later vertex without explicit dependencies also waits for.
- The algorithm takes its own tag (`MPIR_Sched_next_tag`) while it builds the
  graph, and passes it to every send and receive.
- Scratch memory comes from `MPIR_TSP_sched_malloc`, or is handed over with
  `MPIR_TSP_sched_add_buffer`. Both go into `owned`.

### Engine (`src/mpi/coll/transports/gentran`)

- `MPIR_TSP_sched_start` returns a request created already complete for an
  empty schedule. Otherwise it creates the request, pokes the schedule once
  (`MPII_Genutil_sched_poke` issues every vertex with no pending
  dependencies), and, if the schedule is not done yet, queues it for the
  gentran progress hook.
- `MPII_Genutil_progress_hook` pokes every queued schedule. When an issued
  vertex completes, each vertex that depends on it has its
  `pending_dependencies` decremented, and is issued when that reaches 0. So a
  vertex starts as soon as its own dependencies are done, not at a round
  boundary. When every vertex has completed, the request is completed and a
  non-persistent schedule is freed.
- A persistent schedule stays. At every `MPI_Start`, `MPIR_TSP_sched_reset`
  puts every vertex back in its initial state with
  `pending_dependencies = num_dependencies` (nested schedules too), and the
  tag the algorithm took is reused. `MPIR_Persist_coll_free_cb` frees it with
  `MPIR_TSP_sched_free`.

## Greq algorithms

### Interface

A greq algorithm provides an `_init` with the collective's arguments, which
fills in an `MPIR_Coll_greq_t`:

```c
typedef struct MPIR_Coll_greq {
    /* set by the algorithm's _init */
    void *state;
    int (*start_fn) (void *state, MPIR_Request ** req);
    void (*free_fn) (void *state);
    /* set up before _init */
    MPIR_Coll_owned_t owned;
} MPIR_Coll_greq_t;

int MPIR_Ibcast_intra_circ_graph_init(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                      int root, MPIR_Comm * comm, MPIR_Coll_greq_t * greq);
```

- `_init` does everything that does not change between runs: computing the
  communication pattern, allocating queues, querying buffer attributes,
  taking a tag. A persistent collective pays this cost once.
- `start_fn` runs the operation once. It resets the per-run part of `state`,
  creates a generalized request without callbacks
  (`MPIR_Grequest_start_impl(NULL, NULL, NULL, NULL, req)`), arranges its own
  progress, and returns the request. It sets `*req = NULL` when there is
  nothing to do (e.g. a single process).
- When the run is done, the algorithm completes the request with
  `MPIR_Grequest_complete_impl`. That is the last time the run touches
  `state`: another thread may free the operation right after it.
- `free_fn` releases `state`. It may be NULL when the algorithm put everything
  in `owned` (`MPIR_Coll_owned_add_buffer`).
- At most one run is active at a time. A persistent request cannot be started
  again before it completes, and a nonblocking call runs once.

### Framework (`src/mpi/coll/src/coll_greq.c`)

- `MPII_CSEL_CREATE_GREQ(coll_sig)` allocates the `MPIR_Coll_greq_t`, initializes
  `owned`, and sets `sched_type = MPIR_SCHED_GREQ`. The `_cnt` wrapper then
  calls the algorithm's `_init` with the arguments from the `coll_sig`.
- `MPIR_Coll_greq_start(greq, &request)` (nonblocking, from
  `MPII_SCHED_START`): calls `greq->start_fn` and makes the request's `free_fn`
  free the operation, so the operation lives until the user frees the request.
  If `start_fn` returns no request, it frees the operation and returns a request
  created already complete.
- Persistent: `MPIR_Persist_coll_start` calls `greq->start_fn` and uses the
  returned request as `real_request`, or one created already complete.
  `MPIR_Persist_coll_free_cb` calls `MPIR_Coll_greq_free`.
- `MPIR_Coll_greq_free(greq)`: `greq->free_fn(greq->state)` if set,
  `MPIR_Coll_owned_free`, `MPL_free`.

A greq algorithm may not set `gpu_swap`, since there is no schedule to add the
swap steps to. It handles device buffers itself.

### Example: `ibcast` `circ_graph`

| Stage | Work |
|---|---|
| `_init` | `MPII_circ_graph_create` (Skip/R/S), request queue setup (chunking, contiguity and GPU attributes, tag, arrays), rank, root, round count |
| `start_fn` | `MPII_cga_queue_reset`, reset the round counters, create the grequest, `MPIR_Async_things_add` |
| poll, when done | clear the pending blocks, complete the grequest |
| `free_fn` | `MPII_circ_graph_free`, `MPII_cga_queue_free` |

A persistent `MPIDU` schedule also keeps its tag for every start. The blocking
`circ_graph` algorithms (`bcast`, `allgather`, `reduce`) call
`MPII_cga_queue_free` after `MPII_cga_waitall`.

## Related

- [Collective Algorithm Selection (CSEL)](Collective_Selection.md)
