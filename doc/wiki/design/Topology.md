# Topology and Communicator Split Types

This page describes how MPICH models process locality and hardware topology,
and how `MPI_Comm_split_type` uses them. It also describes the `split_type`
hook that lets a device override the split, and the contract a device must
follow when it does.

## Overview

Topology information comes from three sources:

| Source | What it describes | Code |
|---|---|---|
| Node map | Which processes share a *locality domain* (a "node") | `src/util/mpir_nodemap.c` |
| hwtopo | Hardware objects within a node (package, NUMA, core, cache, devices), via hwloc | `src/util/mpir_hwtopo.c`, `src/include/mpir_hwtopo.h` |
| nettopo | Network topology across nodes (tree switches, torus) | `src/util/mpir_nettopo.c`, `src/include/mpir_nettopo.h` |

The split types form a nested hierarchy. Each level splits within the level
above it:

```
input comm
  └─ network / neighborhood splits   (MPIX_COMM_TYPE_NEIGHBORHOOD)
      └─ node == MPI_COMM_TYPE_SHARED  (node map)
          └─ hardware splits         (MPI_COMM_TYPE_HW_GUIDED / HW_UNGUIDED,
                                      MPI_COMM_TYPE_RESOURCE_GUIDED)
```

The key invariant: **a hardware split never crosses the
`MPI_COMM_TYPE_SHARED` domain.** Users expect `HW_GUIDED`, `HW_UNGUIDED`, and
`RESOURCE_GUIDED` results to be subsets of the shared-memory communicator.

## The node map

`MPIR_Process.node_map[rank]` maps each process in `MPI_COMM_WORLD` to a node
id in `0..num_nodes-1`. `MPID_Get_node_id()` returns it, and everything that
asks "are these processes local to each other" uses it.

A node is not necessarily a physical host. It is a **locality domain**: the
set of processes that can communicate through shared memory. The node map is
built once by `MPIR_build_nodemap()`, called from `MPIR_pmi_init()` before any
device initialization:

1. **nolocal mode.** If `get_option_no_local()` is true, every process is its
   own node (`node_map[i] = i`). It is true when:
    - MPICH is configured with `ENABLE_NO_LOCAL`, either through
      `--enable-nolocal` or set by a device subconfigure because the device has
      no shared memory (ch3:sock; ch4 with `--with-ch4-shmmods=none`); or
    - `MPIR_CVAR_NOLOCAL=1`; or
    - `MPL_proc_mutex_enabled()` is false. Without an interprocess mutex, shared
      memory communication cannot be made race free.
      `get_option_no_local()` sets `MPIR_CVAR_NOLOCAL` in this case, so the
      cvar stays consistent with the node map.
2. **Process manager.** Otherwise, node ids come from PMI
   (`MPIR_pmi_build_nodemap()`) and are normalized to a contiguous range.
3. **Cliques (debugging).** On a single host, `MPIR_CVAR_NUM_CLIQUES` or
   `MPIR_CVAR_ODD_EVEN_CLIQUES` split the processes into several fake nodes,
   by block (`MPIR_CVAR_CLIQUES_BY_BLOCK`) or round robin.

`MPIR_build_locality()` then derives `local_size`, `local_rank`,
`node_local_map`, and `node_root_map`.

Because a node is a locality domain, a device without shared memory must make
the node map say so by using nolocal mode. It should not leave the map
grouping processes by host and special-case `MPI_COMM_TYPE_SHARED` instead.
Otherwise, the hardware splits and the communicator hierarchy (below)
disagree with `MPI_COMM_TYPE_SHARED`.

### Communicator hierarchy

Every intracommunicator gets locality information from the node map
(`check_hierarchy()` in `src/mpi/comm/commutil.c`): `num_local`, `local_rank`,
`num_external`, `external_rank`, and `hierarchy_flags`. These include
`MPIR_COMM_HIERARCHY__SINGLE_NODE` and `MPIR_COMM_HIERARCHY__NO_LOCAL`, the
latter set when every process is on its own node. `node_comm` and
`node_roots_comm` are built from the same information and are used by the
node-aware (SMP) collective algorithms and by collective selection. In nolocal
mode, communicators are flagged `NO_LOCAL` and the SMP algorithms are not
selected.

## hwtopo

`MPII_hwtopo_init()` loads the hwloc topology (`MPIR_pmi_load_hwloc_topology()`)
and queries the process binding. `MPIR_hwtopo_is_initialized()` is false if
hwloc is not available or the binding cannot be queried (e.g. on macOS).

Hardware objects are identified by a gid, `MPIR_hwtopo_gid_t`, which encodes
the object class, depth, and logical index. `MPIR_HWTOPO_GID_ROOT` means
"no object" or "the whole machine".

- `MPIR_hwtopo_get_type_id(name)` maps a resource name to a
  `MPIR_hwtopo_type_e`. For example, `"node"` and `"machine"` map to
  `MPIR_HWTOPO_TYPE__NODE`, and `"pu"` and `"hwthread"` both map to
  `MPIR_HWTOPO_TYPE__HWTHREAD`.
- `MPIR_hwtopo_get_obj_by_name(name)` returns the gid of the object of that
  type that contains the whole process binding. It returns
  `MPIR_HWTOPO_GID_ROOT` if the binding spans more than one such object or
  hwtopo is not initialized. Names starting with `pci:`, `ib`, `hfi`, `eth`,
  `en`, and `gpu` resolve to the non-I/O ancestor of the matching device.

A gid is only meaningful within a node. Logical indices restart on each host,
so the same gid on two nodes refers to different objects. This is why the
hardware splits always split within the node first.

## Split types

`MPI_Comm_split_type` calls `MPIR_Comm_split_type_impl()`
(`src/mpi/comm/comm_split_type.c`). It dispatches to the device hook if one is
installed (see below), otherwise to the generic `MPIR_Comm_split_type()`, and
then attaches the info to the new communicator.

The generic `MPIR_Comm_split_type()` first splits out the processes that pass
`MPI_UNDEFINED`, then branches on the split type:

| Split type | Implementation | Result |
|---|---|---|
| `MPI_COMM_TYPE_SHARED` | `MPIR_Comm_split_type_node_topo()` | Split by node id |
| `MPI_COMM_TYPE_HW_GUIDED` | `split_type_hw_guided()` | See below |
| `MPI_COMM_TYPE_RESOURCE_GUIDED` | `split_type_hw_guided()` for `mpi_hw_resource_type`; `split_type_pset_name()` for `mpi_pset_name` (not yet implemented, returns `MPI_COMM_NULL`) | |
| `MPI_COMM_TYPE_HW_UNGUIDED` | `split_type_hw_unguided()` | See below |
| `MPIX_COMM_TYPE_NEIGHBORHOOD` | `MPIR_Comm_split_type_neighborhood()` | See below |

Info keys that control a split are read with `MPII_collect_info_key()`, which
checks that all processes pass the same value. If the values differ, the key is
ignored, as if it had not been passed.

### MPI_COMM_TYPE_HW_GUIDED

`split_type_hw_guided()` handles the `mpi_hw_resource_type` value:

1. `"mpi_shared_memory"`: equivalent to `MPI_COMM_TYPE_SHARED`, as the MPI
   standard requires. It calls `MPIR_Comm_split_type_impl(..., MPI_COMM_TYPE_SHARED, ...)`,
   so it goes through the device hook if there is one.
2. Otherwise, it first splits by node (`split_type_by_node()`). Since the node
   map is the shared-memory domain, this keeps the result within
   `MPI_COMM_TYPE_SHARED`.
3. `"node"` or `"machine"` (`MPIR_HWTOPO_TYPE__NODE`): the node communicator is
   the result. This does not depend on hwtopo.
4. If hwtopo is not initialized, the result is `MPI_COMM_NULL`.
5. Otherwise, each process looks up the gid of the resource that contains its
   binding and splits the node communicator by gid. A process whose binding is
   not within a single instance of the resource (gid is
   `MPIR_HWTOPO_GID_ROOT`) uses `MPI_UNDEFINED` and gets `MPI_COMM_NULL`. It
   still participates in the split. The result is not required to be a proper
   subset.

### MPI_COMM_TYPE_HW_UNGUIDED

`split_type_hw_unguided()` returns the largest hardware split that is a proper
subset of the input communicator, or `MPI_COMM_NULL` if there is none. It tries
levels from the top down:

1. Node. If the node split is smaller than the input, it returns it with
   resource type `"node"`.
2. `"package"`, `"numanode"`, `"cpu"`, `"core"`, `"hwthread"`, `"bindset"`,
   splitting by gid. This step only runs when all processes are on one node,
   so gids do not collide.

On success, it sets `mpi_hw_resource_type` on the info object to the level
that was chosen.

### MPIX_COMM_TYPE_NEIGHBORHOOD

`MPIR_Comm_split_type_neighborhood()` (`src/mpi/comm/comm_split_type_nbhd.c`)
handles splits above the node level:

- `nbhd_common_dirname`: processes that share a file system directory (through
  ROMIO).
- `network_topo`: network topology from nettopo, with values
  `switch_level:<n>`, `subcomm_min_size:<n>`, `min_mem_size:<n>`, and
  `torus_dimension:<n>`.

## The split_type hook

### Mechanism

```c
/* src/include/mpir_comm.h */
typedef struct MPIR_Commops {
    int (*split_type) (MPIR_Comm *, int, int, MPIR_Info *, MPIR_Comm **);
} MPIR_Commops;
extern struct MPIR_Commops *MPIR_Comm_fns;
```

`MPIR_Comm_fns` is `NULL` by default (`src/mpi/comm/commutil.c`). A device
installs an override by pointing it to a table with a non-`NULL` `split_type`,
typically in `MPID_Init`. `MPIR_Comm_split_type_impl()` calls the hook if it is
installed and otherwise calls the generic `MPIR_Comm_split_type()`:

```c
if (MPIR_Comm_fns != NULL && MPIR_Comm_fns->split_type != NULL) {
    mpi_errno = MPIR_Comm_fns->split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
} else {
    mpi_errno = MPIR_Comm_split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
}
```

Currently no device installs the hook. ch3:sock, ch3:nemesis, and ch4 all use
the generic path, and express their locality through the node map. The hook is
kept as an extension point for a future device.

### When to use it

First, consider expressing the device's locality through the node map. A
device without shared memory should use nolocal mode. In that case, the generic
code gives a consistent `MPI_COMM_TYPE_SHARED`, hardware splits, and
communicator hierarchy, and no hook is needed.

Use the hook only when the device has split semantics that the node map cannot
express, for example:

- a shared-memory domain that depends on information only the device has; or
- device-specific split types (an `MPIX_COMM_TYPE_*` extension).

### Contract for a split_type override

An override replaces `MPIR_Comm_split_type()` for all split types, so it must
follow the same rules:

1. **Collective and consistent.** It is called collectively on `comm_ptr`.
   Every process must take the same path and call the same sequence of
   collective operations. Branch only on information that is the same on all
   processes, such as the split type, or on keys read with
   `MPII_collect_info_key()`.
2. **MPI_UNDEFINED.** Processes may pass `MPI_UNDEFINED` as the split type
   while others pass a real type. Those processes must get `*newcomm_ptr = NULL`
   and still take part in the collective split. The usual pattern, from the
   generic code, is to split them out first:
   ```c
   mpi_errno = MPIR_Comm_split_impl(user_comm_ptr,
                                    split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                    key, &comm_ptr);
   ```
3. **Hand back what you don't handle.** For split types the device does not
   specialize, call the generic `MPIR_Comm_split_type()` with the original
   communicator. Do not call `MPIR_Comm_split_type_impl()` on the same
   arguments, because that dispatches back to the hook and recurses.
4. **Keep the hierarchy invariant.** If the device overrides
   `MPI_COMM_TYPE_SHARED`, the hardware splits, which run in the generic code,
   must still nest within its result. They split within the node map, so the
   device's `MPI_COMM_TYPE_SHARED` must match the node map (each result is one
   node), or the device must override the hardware split types too.
   `"mpi_shared_memory"` must give the same result as `MPI_COMM_TYPE_SHARED`.
   The generic code ensures this by calling `MPIR_Comm_split_type_impl()`, so
   it reaches the hook.
5. **Results.** Return a new communicator or `NULL` (`MPI_COMM_NULL`). Do not
   return the input communicator itself. `MPIR_Comm_split_type_impl()` attaches
   the info to the result after the hook returns.

Building blocks the hook can use:

| Function | Result |
|---|---|
| `MPIR_Comm_split_type_node_topo()` | Split by node id |
| `MPIR_Comm_split_type_self()` | A dup of `MPI_COMM_SELF` for each process |
| `MPIR_Comm_split_type_neighborhood()` | The `MPIX_COMM_TYPE_NEIGHBORHOOD` split |
| `MPIR_Comm_split_type()` | The generic implementation of all split types |

### History

ch3:nemesis and ch4 used to install overrides. Both only specialized
`MPI_COMM_TYPE_SHARED` (a split by node, or `MPIR_Comm_split_type_self()` for
nemesis without an interprocess mutex). ch3:sock used a special case in the
generic code that returned `MPIR_Comm_split_type_self()`. In both
`MPIR_Comm_split_type_self()` cases, the node map still grouped processes by
host, so the hardware splits could return communicators larger than
`MPI_COMM_TYPE_SHARED`. These cases now use nolocal mode, and the overrides were
removed because they matched the generic path.
