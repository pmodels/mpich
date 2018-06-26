/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidu_shm_impl.h"
#include "build_nodemap.h"
#include "mpir_mem.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : MEMBIND
      description : cvars controlling shared memory objects binding

cvars:
    - name        : MPIR_CVAR_MEMBIND_NUMA_ENABLE
      category    : MEMBIND
      type        : string
      default     : "NO"
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable NUMA architecture support. NUMA support is needed to
        use heterogeneous memory.

    - name        : MPIR_CVAR_MEMBIND_TYPE_LIST
      category    : MEMBIND
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The memory types used for allocating MPICH objects in a
        heterogeneous memory system.

    - name        : MPIR_CVAR_MEMBIND_POLICY_LIST
      category    : MEMBIND
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL
      description : >-
        The memory policy used for allocating MPICH objects in a
        heterogeneous memory system (e.g., BIND, INTERLEAVE).

    - name        : MPIR_CVAR_MEMBIND_FLAGS_LIST
      category    : MEMBIND
      type        : string
      default     : ""
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL
      description : >-
        The memory flags used for allocating MPICH objects in a
        heterogeneous memory system (e.g., STRICT).

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define MEMBIND_INFO_SET(cvar, key, val)                                   \
    do {                                                                   \
        char *token, *tok, *brk_o, *brk_p, *info_str, *val_str;            \
        for (token = strtok_r(cvar, ",", &brk_o); token;                   \
             token = strtok_r(NULL, ",", &brk_o)) {                        \
            tok = MPL_strdup(token);                                       \
            MPIR_Memtype mbind_type;                                       \
            hwloc_membind_policy_t mbind_policy;                           \
            int mbind_flags;                                               \
            MPIDU_shm_obj_t mbind_object;                                  \
                                                                           \
            info_str = strtok_r(tok, ":", &brk_p);                         \
            val_str = strtok_r(NULL, ":", &brk_p);                         \
                                                                           \
            if (!strcmp(info_str, "FASTBOXES")) {                          \
                mbind_object = MPIDU_SHM_OBJ__FASTBOXES;                   \
            } else if (!strcmp(info_str, "CELLS")) {                       \
                mbind_object = MPIDU_SHM_OBJ__CELLS;                       \
            } else if (!strcmp(info_str, "COPYBUFS")) {                    \
                mbind_object = MPIDU_SHM_OBJ__COPYBUFS;                    \
            } else if (!strcmp(info_str, "WIN")) {                         \
                mbind_object = MPIDU_SHM_OBJ__WIN;                         \
            } else {                                                       \
                continue;                                                  \
            }                                                              \
                                                                           \
            if (!strcmp(key, "type")) {                                    \
                if (!strcmp(val_str, "AUTO")) {                            \
                    mbind_type = MPIR_MEMTYPE__DEFAULT;                    \
                } else if (!strcmp(val_str, "DDR")) {                      \
                    mbind_type = MPIR_MEMTYPE__DDR;                        \
                } else if (!strcmp(val_str, "MCDRAM")) {                   \
                    mbind_type = MPIR_MEMTYPE__MCDRAM;                     \
                } else {                                                   \
                    continue;                                              \
                }                                                          \
            } else if (!strcmp(key, "policy")) {                           \
                if (!strcmp(val_str, "BIND")) {                            \
                    mbind_policy = HWLOC_MEMBIND_BIND;                     \
                } else if (!strcmp(val_str, "INTERLEAVE")) {               \
                    mbind_policy = HWLOC_MEMBIND_INTERLEAVE;               \
                } else {                                                   \
                    continue;                                              \
                }                                                          \
            } else {                                                       \
                if (!strcmp(val_str, "STRICT")) {                          \
                    mbind_flags = HWLOC_MEMBIND_STRICT;                    \
                }                                                          \
            }                                                              \
                                                                           \
            MPIDU_shm_obj_info.object_##val[mbind_object] = mbind_##val;   \
                                                                           \
            MPL_free(tok);                                                 \
        }                                                                  \
    } while (0)

#define ENABLE_NUMA ((!strcmp(MPIR_CVAR_MEMBIND_NUMA_ENABLE, "YES")) ? 1 : 0)

MPIDU_shm_obj_info_t MPIDU_shm_obj_info = { 0 };
MPIDU_shm_numa_info_t MPIDU_shm_numa_info = { 0 };

static MPIDU_shm_seg_t memory;
static MPIDU_shm_barrier_t *barrier;

#undef FUNCNAME
#define FUNCNAME MPIDU_shm_numa_info_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_shm_numa_info_init(int rank, int size, int *nodemap)
{
    int i, mpi_errno = MPI_SUCCESS;
    int num_local;
    int local_rank;
    int local_rank_0;
    int nnodes = 1;
    int num_bound_nodes = 0;
    int *nodeid_p = NULL;

#ifdef HAVE_HWLOC
    if (ENABLE_NUMA && MPIR_Process.bindset_is_valid) {
        /* Detect number of NUMA nodes in the system */
        nnodes = hwloc_get_nbobjs_by_type(MPIR_Process.hwloc_topology, HWLOC_OBJ_NUMANODE);

        /* Detect whether processes are bound to only one numa node or not */
        hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
        hwloc_cpuset_to_nodeset(MPIR_Process.hwloc_topology, MPIR_Process.bindset, nodeset);
        for (i = 0; i < nnodes; i++) {
            hwloc_obj_t obj =
                hwloc_get_obj_by_type(MPIR_Process.hwloc_topology, HWLOC_OBJ_NUMANODE, i);
            /* NOTE: need 'hwloc_bitmap_intersects' instead of 'hwloc_bitmap_isequal'
             *       because in hwloc numa nodes in the same Group (Cluster) share the
             *       cpuset. Thus, converting cpusets to nodesets on systems like KNL
             *       will result on more than one node to be set in the bitmap (one for
             *       DDR and one for MCDRAM). However, in such systems, we only need to
             *       detect binding to DDR nodes (which have no 'subtype'). For more
             *       information see: B. Goglin "Exposing the Locality of Heterogeneous
             *       Memory Architectures to HPC Applications" */
            if (hwloc_bitmap_intersects(obj->nodeset, nodeset) && !obj->subtype) {
                num_bound_nodes++;
            }
        }
        hwloc_bitmap_free(nodeset);
    }
#endif

    /* Initialize numa info */
    MPIDU_shm_numa_info.nodeset_is_valid = (num_bound_nodes == 1);
    MPIDU_shm_numa_info.nnodes = nnodes;
    MPIDU_shm_numa_info.nodeid = NULL;
    MPIDU_shm_numa_info.siblid = NULL;
    MPIDU_shm_numa_info.type = NULL;
    MPIDU_shm_numa_info.bitmap = 0;

    /* Get nodemap info to initialize shm segment */
    MPIR_NODEMAP_get_local_info(rank, size, nodemap, &num_local, &local_rank, &local_rank_0);

    MPIR_CHKPMEM_DECL(3);
    MPIR_CHKPMEM_MALLOC(MPIDU_shm_numa_info.nodeid, int *, num_local * sizeof(int), mpi_errno,
                        "node ids", MPL_MEM_OTHER);
    MPIR_CHKPMEM_MALLOC(MPIDU_shm_numa_info.siblid, int *, nnodes * sizeof(int), mpi_errno,
                        "sibling ids", MPL_MEM_OTHER);
    MPIR_CHKPMEM_MALLOC(MPIDU_shm_numa_info.type, MPIR_Memtype *, nnodes * sizeof(MPIR_Memtype),
                        mpi_errno, "node memory type", MPL_MEM_OTHER);

    /* Initialize node ids */
    for (i = 0; i < num_local; i++)
        MPIDU_shm_numa_info.nodeid[i] = 0;

    /* Initialize sibling ids */
    for (i = 0; i < nnodes; i++)
        MPIDU_shm_numa_info.siblid[i] = -1;

    /* Request node id per local rank region */
    mpi_errno =
        MPIDU_shm_seg_alloc(num_local * sizeof(int), (void **) &nodeid_p, MPIR_MEMTYPE__DEFAULT,
                            MPL_MEM_SHM);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Actually allocate segment and assign regions to pointer */
    mpi_errno =
        MPIDU_shm_seg_commit(&memory, &barrier, num_local, local_rank, local_rank_0, rank, 0,
                             MPIDU_SHM_OBJ__NONE, MPL_MEM_SHM);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Synchronize all processes */
    MPIDU_shm_barrier(barrier, num_local);

#ifdef HAVE_HWLOC
    /* Initialize default binding values */
    for (i = 0; i < MPIDU_SHM_OBJ__NUM; i++) {
        MPIDU_shm_obj_info.object_type[i] = MPIR_MEMTYPE__NONE;
        MPIDU_shm_obj_info.object_policy[i] = HWLOC_MEMBIND_BIND;
        MPIDU_shm_obj_info.object_flags[i] = HWLOC_MEMBIND_STRICT;
    }

    /* Detect NUMAs only if node binding is valid */
    if (MPIDU_shm_numa_info.nodeset_is_valid) {
        hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
        hwloc_cpuset_to_nodeset(MPIR_Process.hwloc_topology, MPIR_Process.bindset, nodeset);

        for (i = 0; i < nnodes; i++) {
            hwloc_obj_t obj =
                hwloc_get_obj_by_type(MPIR_Process.hwloc_topology, HWLOC_OBJ_NUMANODE, i);

            /* Every rank sets its node id */
            if (hwloc_bitmap_intersects(obj->nodeset, nodeset) && !obj->subtype) {
                nodeid_p[local_rank] = i;
            }

            /* Set memory type for node to default */
            MPIDU_shm_numa_info.type[i] = MPIR_MEMTYPE__DEFAULT;

            /* Detect actual memory type for the node */
            if (obj->subtype && !strcmp(obj->subtype, "MCDRAM")) {
                MPIDU_shm_numa_info.type[i] = MPIR_MEMTYPE__MCDRAM;
            }

            /* Detect the sibling NUMA memory for the node */
            if (obj->next_sibling && obj->next_sibling->subtype &&
                !strcmp(obj->next_sibling->subtype, "MCDRAM")) {
                MPIDU_shm_numa_info.siblid[i] = obj->next_sibling->logical_index;
            }
        }

        /* Wait for all ranks to update their shm segment information */
        MPIDU_shm_barrier(barrier, num_local);

        /* Each rank updates its private bitmap and nodeid array */
        for (i = 0; i < num_local; i++) {
            if (!(MPIDU_shm_numa_info.bitmap & (1 << nodeid_p[i]))) {
                MPIDU_shm_numa_info.bitmap |= (1 << nodeid_p[i]);
            }
            MPIDU_shm_numa_info.nodeid[i] = nodeid_p[i];
        }

        /* Set user defined binding values, if any */
        MEMBIND_INFO_SET(MPIR_CVAR_MEMBIND_TYPE_LIST, "type", type);
        MEMBIND_INFO_SET(MPIR_CVAR_MEMBIND_POLICY_LIST, "policy", policy);
        MEMBIND_INFO_SET(MPIR_CVAR_MEMBIND_FLAGS_LIST, "flags", flags);

        hwloc_bitmap_free(nodeset);
    } else {
        /* If no binding we need at least one node for later
         * shared memory segment allocation, however we won't
         * need to bind memory to the node in this case. */
        MPIDU_shm_numa_info.bitmap = 1;
    }
#else
    /* Initialize default binding values */
    for (i = 0; i < MPIDU_SHM_OBJ__NUM; i++) {
        MPIDU_shm_obj_info.object_type[i] = MPIR_MEMTYPE__NONE;
    }

    /* No hwloc, init numa_info to have at least one node */
    MPIDU_shm_numa_info.bitmap = 1;
    MPIR_ERR_CHKANDJUMP(1, mpi_errno, MPI_ERR_OTHER, "**hwloc");
#endif

    MPIDU_shm_barrier(barrier, num_local);

  fn_exit:
    /* Destroy shared memory */
    MPIDU_shm_seg_destroy(&memory, num_local);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_shm_numa_info_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_shm_numa_info_finalize()
{
    MPL_free(MPIDU_shm_numa_info.nodeid);
    MPL_free(MPIDU_shm_numa_info.siblid);
    MPL_free(MPIDU_shm_numa_info.type);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_shm_numa_bind_set
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_shm_numa_bind_set(void *addr, size_t len, int node_id, MPIR_Info * info,
                            MPIDU_shm_obj_t object)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_HWLOC
    int i;
    int info_flag;
    int flags = 0;
    hwloc_membind_policy_t policy = HWLOC_MEMBIND_BIND;
    hwloc_obj_t obj;
    hwloc_nodeset_t nodeset;
    MPIR_Memtype type = MPIR_MEMTYPE__NONE;
    char info_value[MPI_MAX_INFO_VAL + 1];

    /* If cannot bind fail */
    if (!MPIDU_shm_numa_info.nodeset_is_valid) {
        MPIR_ERR_CHKANDJUMP(1, mpi_errno, MPI_ERR_OTHER, "**nobind");
    }

    /* If object is none skip binding */
    if (object == MPIDU_SHM_OBJ__NONE) {
        goto fn_exit;
    }

    if (object == MPIDU_SHM_OBJ__WIN) {
        if (info) {
            MPIR_Info_get_impl(info, "mpich_win_membind_type", MPI_MAX_INFO_VAL, info_value,
                               &info_flag);
            if (info_flag) {
                if (!strcmp(info_value, "dram")) {
                    type = MPIR_MEMTYPE__DDR;
                } else if (!strcmp(info_value, "mcdram")) {
                    type = MPIR_MEMTYPE__MCDRAM;
                } else {
                    type = MPIR_MEMTYPE__DEFAULT;
                }
            }

            MPIR_Info_get_impl(info, "mpich_win_membind_policy", MPI_MAX_INFO_VAL, info_value,
                               &info_flag);
            if (info_flag) {
                if (!strcmp(info_value, "bind")) {
                    policy = HWLOC_MEMBIND_BIND;
                } else if (!strcmp(info_value, "interleave")) {
                    policy = HWLOC_MEMBIND_INTERLEAVE;
                }
            }

            MPIR_Info_get_impl(info, "mpich_win_membind_flags", MPI_MAX_INFO_VAL, info_value,
                               &info_flag);
            if (info_flag) {
                if (!strcmp(info_value, "strict")) {
                    flags = HWLOC_MEMBIND_STRICT;
                }
            }
        }
    }

    /* Overwrite user hints with environment */
    if (MPIDU_shm_obj_info.object_type[object] != MPIR_MEMTYPE__NONE) {
        type = MPIDU_shm_obj_info.object_type[object];
        policy = MPIDU_shm_obj_info.object_policy[object];
        flags = MPIDU_shm_obj_info.object_flags[object];
    }

    /* No binding set by user, terminate */
    if (type == MPIR_MEMTYPE__NONE) {
        goto fn_exit;
    }

    int target_id = node_id;
    int node_bitmap = MPIDU_shm_numa_info.bitmap;
    int *nodeid_p = MPIDU_shm_numa_info.nodeid;
    int *siblid_p = MPIDU_shm_numa_info.siblid;

    if (type != MPIR_MEMTYPE__DEFAULT) {
        target_id = siblid_p[node_id];
        if (target_id >= 0) {
            /* If non-default memory is not available, fail */
            if (MPIDU_shm_obj_info.object_type[object] != MPIDU_shm_numa_info.type[target_id]) {
                MPIR_ERR_CHKANDJUMP(1, mpi_errno, MPI_ERR_OTHER, "**memtype");
            }
        } else {
            MPIR_ERR_CHKANDJUMP(1, mpi_errno, MPI_ERR_OTHER, "**memtype");
        }
    }

    /* Get hwloc nodeset for memory binding */
    if (policy == HWLOC_MEMBIND_BIND) {
        obj = hwloc_get_obj_by_type(MPIR_Process.hwloc_topology, HWLOC_OBJ_NUMANODE, target_id);
        nodeset = hwloc_bitmap_dup(obj->nodeset);
    } else if (policy == HWLOC_MEMBIND_INTERLEAVE) {
        nodeset = hwloc_bitmap_alloc();
        for (i = 0; i < MPIDU_shm_numa_info.nnodes; i++) {
            if (node_bitmap & (1 << i)) {
                if (type == MPIR_MEMTYPE__DEFAULT)
                    obj = hwloc_get_obj_by_type(MPIR_Process.hwloc_topology, HWLOC_OBJ_NUMANODE, i);
                else
                    obj =
                        hwloc_get_obj_by_type(MPIR_Process.hwloc_topology, HWLOC_OBJ_NUMANODE,
                                              siblid_p[i]);
                hwloc_bitmap_or(nodeset, obj->nodeset, nodeset);
            }
        }
    }

    /* Do memory binding */
    hwloc_set_area_membind(MPIR_Process.hwloc_topology, addr, len, nodeset,
                           policy, HWLOC_MEMBIND_BYNODESET | flags);

    hwloc_bitmap_free(nodeset);
#else
    MPIR_ERR_CHKANDJUMP(1, mpi_errno, MPI_ERR_OTHER, "**hwloc");
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
