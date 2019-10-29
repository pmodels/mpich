/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"
#include "mpir_hw_topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_split_type */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_split_type = PMPI_Comm_split_type
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_split_type  MPI_Comm_split_type
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_split_type as PMPI_Comm_split_type
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm * newcomm)
    __attribute__ ((weak, alias("PMPI_Comm_split_type")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_split_type
#define MPI_Comm_split_type PMPI_Comm_split_type

int MPIR_Comm_split_type_self(MPIR_Comm * user_comm_ptr, int split_type, int key,
                              MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *comm_self_ptr;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, newcomm_ptr);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_node(MPIR_Comm * user_comm_ptr, int split_type, int key,
                              MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

struct shmem_processor_info_table {
    const char *val;
    MPIR_Node_obj_type obj_type;
};

/* hardware topology node object table */
static struct shmem_processor_info_table shmem_processor_info[] = {
    {"machine", MPIR_NODE_OBJ_TYPE__MACHINE},
    {"socket", MPIR_NODE_OBJ_TYPE__PACKAGE},
    {"package", MPIR_NODE_OBJ_TYPE__PACKAGE},
    {"numa", MPIR_NODE_OBJ_TYPE__NUMANODE},
    {"core", MPIR_NODE_OBJ_TYPE__CORE},
    {"hwthread", MPIR_NODE_OBJ_TYPE__PU},
    {"pu", MPIR_NODE_OBJ_TYPE__PU},
    {"l1dcache", MPIR_NODE_OBJ_TYPE__L1CACHE},
    {"l1ucache", MPIR_NODE_OBJ_TYPE__L1CACHE},
    {"l1icache", MPIR_NODE_OBJ_TYPE__L1ICACHE},
    {"l1cache", MPIR_NODE_OBJ_TYPE__L1CACHE},
    {"l2dcache", MPIR_NODE_OBJ_TYPE__L2CACHE},
    {"l2ucache", MPIR_NODE_OBJ_TYPE__L2CACHE},
    {"l2icache", MPIR_NODE_OBJ_TYPE__L2ICACHE},
    {"l2cache", MPIR_NODE_OBJ_TYPE__L2CACHE},
    {"l3dcache", MPIR_NODE_OBJ_TYPE__L3CACHE},
    {"l3ucache", MPIR_NODE_OBJ_TYPE__L3CACHE},
    {"l3icache", MPIR_NODE_OBJ_TYPE__L3ICACHE},
    {"l3cache", MPIR_NODE_OBJ_TYPE__L3CACHE},
    {"l4dcache", MPIR_NODE_OBJ_TYPE__L4CACHE},
    {"l4ucache", MPIR_NODE_OBJ_TYPE__L4CACHE},
    {"l4cache", MPIR_NODE_OBJ_TYPE__L4CACHE},
    {"l5dcache", MPIR_NODE_OBJ_TYPE__L5CACHE},
    {"l5ucache", MPIR_NODE_OBJ_TYPE__L5CACHE},
    {"l5cache", MPIR_NODE_OBJ_TYPE__L5CACHE},
    {NULL, MPIR_NODE_OBJ_TYPE__MAX}
};

static int node_split_processor(MPIR_Comm * comm_ptr, int key, const char *hintval,
                                MPIR_Comm ** newcomm_ptr)
{
    int color;
    MPIR_Node_obj obj_containing_cpuset;
    MPIR_Node_obj_type query_obj_type = MPIR_NODE_OBJ_TYPE__MAX;
    int i, mpi_errno = MPI_SUCCESS;

    /* assign the node id as the color, initially */
    MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);

    /* try to find the info value in the processor object
     * table */
    for (i = 0; shmem_processor_info[i].val; i++) {
        if (!strcmp(shmem_processor_info[i].val, hintval)) {
            query_obj_type = shmem_processor_info[i].obj_type;
            break;
        }
    }

    if (query_obj_type == MPIR_NODE_OBJ_TYPE__MAX)
        goto split_id;

    obj_containing_cpuset = MPIR_Node_get_covering_obj_by_type(query_obj_type);
    if (obj_containing_cpuset)
        color = MPIR_Node_get_obj_index(obj_containing_cpuset);
    else
        color = MPI_UNDEFINED;

  split_id:
    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split_pci_device(MPIR_Comm * comm_ptr, int key,
                                 const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Node_obj non_io_ancestor = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    non_io_ancestor = MPIR_Node_get_non_io_ancestor_obj(hintval);
    if (non_io_ancestor)
        color = MPIR_Node_get_obj_index(non_io_ancestor);
    else
        color = MPI_UNDEFINED;

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split_network_device(MPIR_Comm * comm_ptr, int key,
                                     const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Node_obj non_io_ancestor;
    int mpi_errno = MPI_SUCCESS;
    int color;

    /* assign the node id as the color, initially */
    MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);

    non_io_ancestor = MPIR_Node_get_non_io_ancestor_obj(hintval);
    if (non_io_ancestor) {
        uint32_t depth = (uint32_t) MPIR_Node_get_obj_depth(non_io_ancestor);
        int idx = MPIR_Node_get_obj_index(non_io_ancestor);
        color = (int) ((depth << 16) + idx);
    } else {
        color = MPI_UNDEFINED;
    }

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split_gpu_device(MPIR_Comm * comm_ptr, int key,
                                 const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Node_obj non_io_ancestor;
    int mpi_errno = MPI_SUCCESS;
    int color;

    non_io_ancestor = MPIR_Node_get_non_io_ancestor_obj(hintval);
    if (non_io_ancestor) {
        MPIR_Node_obj_type type = MPIR_Node_get_obj_type(non_io_ancestor);
        int idx = MPIR_Node_get_obj_index(non_io_ancestor);
        color = ((type << (sizeof(int) * 4)) + idx);
    } else {
        color = MPI_UNDEFINED;
    }

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int network_split_switch_level(MPIR_Comm * comm_ptr, int key,
                                      int switch_level, MPIR_Comm ** newcomm_ptr)
{
    int i, color;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Network_node network_node;
    MPIR_Network_node *traversal_stack;
    MPIR_Network_topology_type topo_type;
    int num_nodes;
    int traversal_begin, traversal_end;

    topo_type = MPIR_Net_get_topo_type();
    num_nodes = MPIR_Net_get_num_nodes();

    if (topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__FAT_TREE ||
        topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__CLOS_NETWORK) {
        MPIR_Network_node *switches_at_level;
        int switch_count;
        traversal_stack =
            (MPIR_Network_node *) MPL_malloc(sizeof(MPIR_Network_node) * num_nodes, MPL_MEM_OTHER);

        network_node = MPIR_Net_get_endpoint();

        traversal_begin = 0;
        traversal_end = 0;
        MPIR_Net_tree_topo_get_switches_at_level(switch_level, &switches_at_level, &switch_count);

        /* Find the switch `switch_level` steps away */
        MPIR_Assert(traversal_end < num_nodes);
        traversal_stack[traversal_end++] = network_node;
        color = 0;
        while (traversal_end > traversal_begin) {
            MPIR_Network_node current_node = traversal_stack[traversal_begin++];
            int num_edges;
            int node_uid = MPIR_Net_get_node_uid(current_node);
            int *node_levels = MPIR_Net_tree_topo_get_node_levels();
            MPIR_Network_node_type node_type = MPIR_Net_get_node_type(current_node);
            MPIR_Network_edge *edges;
            if (node_type == MPIR_NETWORK_NODE_TYPE__SWITCH && node_levels[node_uid]
                == switch_level) {
                for (i = 0; i < switch_count; i++) {
                    if (switches_at_level[i] == current_node) {
                        color = color & (1 < i);
                        break;
                    }
                }
            } else {
                continue;
            }
            /*find all nodes not visited with an edge from the current node */
            MPIR_Net_get_all_edges(network_node, &num_edges, &edges);
            for (i = 0; i < num_edges; i++) {
                MPIR_Assert(traversal_end < num_nodes);
                traversal_stack[traversal_end++] = MPIR_Net_get_edge_dest_node(edges[i]);
            }
        }

        if (color == 0) {
            color = MPI_UNDEFINED;
        }

        MPL_free(traversal_stack);
        MPL_free(switches_at_level);
    } else {
        color = MPI_UNDEFINED;
    }

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int get_color_from_subset_bitmap(int node_index, int *bitmap, int bitmap_size, int min_size)
{
    int color;
    int subset_size;
    int current_comm_color;
    int prev_comm_color;
    int i;

    subset_size = 0;
    current_comm_color = 0;
    prev_comm_color = -1;
    color = prev_comm_color;

    for (i = 0; i < bitmap_size; i++) {
        if (subset_size >= min_size) {
            subset_size = 0;
            prev_comm_color = current_comm_color;
            current_comm_color = i;
        }
        subset_size += bitmap[i];
        if (i == node_index) {
            color = current_comm_color;
        }
    }
    if (subset_size < min_size && i == bitmap_size)
        color = prev_comm_color;

  fn_exit:
    return color;

  fn_fail:
    goto fn_exit;

}

static int network_split_by_minsize(MPIR_Comm * comm_ptr, int key, int subcomm_min_size,
                                    MPIR_Comm ** newcomm_ptr)
{

    int mpi_errno = MPI_SUCCESS;
    int i, j, color;
    int comm_size = MPIR_Comm_size(comm_ptr);
    int node_index;
    int num_nodes;
    MPIR_Network_topology_type topo_type;

    num_nodes = MPIR_Net_get_num_nodes();
    topo_type = MPIR_Net_get_topo_type();

    if (subcomm_min_size == 0 || comm_size < subcomm_min_size ||
        topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__INVALID) {
        *newcomm_ptr = NULL;
    } else {
        int *num_processes_at_node = NULL;
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;

        if (topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__FAT_TREE ||
            topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__CLOS_NETWORK) {
            mpi_errno = MPIR_Net_tree_topo_get_hostnode_index(&node_index, &num_nodes);

            MPIR_ERR_CHECK(mpi_errno);

            num_processes_at_node = (int *) MPL_calloc(1, sizeof(int) * num_nodes, MPL_MEM_OTHER);
            num_processes_at_node[node_index] = 1;

        } else if (topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__TORUS) {
            num_processes_at_node = (int *) MPL_calloc(1, sizeof(int) * num_nodes, MPL_MEM_OTHER);
            node_index = MPIR_Net_torus_topo_get_node_index();
            num_processes_at_node[node_index] = 1;
        }
        MPIR_Assert(num_processes_at_node != NULL);
        /* Send the count to processes */
        mpi_errno =
            MPID_Allreduce(MPI_IN_PLACE, num_processes_at_node, num_nodes, MPI_INT,
                           MPI_SUM, comm_ptr, &errflag);

        if (topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__FAT_TREE ||
            topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__CLOS_NETWORK) {
            color =
                get_color_from_subset_bitmap(node_index, num_processes_at_node, num_nodes,
                                             subcomm_min_size);
        } else {
            int torus_dim = MPIR_Net_torus_topo_get_dimension();
            int *torus_geometry = MPIR_Net_torus_topo_get_geometry();
            int *offset_along_dimension = (int *) MPL_calloc(torus_dim, sizeof(int),
                                                             MPL_MEM_OTHER);
            int *partition = (int *) MPL_calloc(torus_dim, sizeof(int),
                                                MPL_MEM_OTHER);
            int num_processes = 0, total_num_processes = 0;

            for (i = 0; i < torus_dim; i++) {
                partition[i] = 1;
            }

            while (1) {
                int node_covered = 0;
                color = total_num_processes;
                for (i = 0; i < torus_dim; i = (i + 1) % torus_dim) {
                    int cube_size;
                    if (partition[i] - 1 + offset_along_dimension[i] == torus_geometry[i]) {
                        if (i == torus_dim - 1) {
                            break;
                        }
                        continue;
                    }
                    partition[i]++;
                    cube_size = 0;
                    for (j = 0; j < torus_dim; j++) {
                        if (partition[j] != 0) {
                            cube_size = cube_size * partition[j];
                        }
                    }
                    num_processes = 0;
                    for (j = 0; j < cube_size; j++) {
                        int *coordinate = (int *) MPL_calloc(torus_dim,
                                                             sizeof(int), MPL_MEM_OTHER);
                        int idx = j;
                        int k;
                        int current_dim = 0;
                        while (current_dim < torus_dim) {
                            coordinate[current_dim++] = idx % partition[j];
                            idx = idx / partition[j];
                        }
                        idx = 0;
                        for (k = 0; k < torus_dim; k++) {
                            idx = idx * (partition[j] + offset_along_dimension[i]) + coordinate[k];
                        }
                        if (idx == node_index) {
                            node_covered = 1;
                            break;
                        }
                        num_processes += num_processes_at_node[idx];
                        MPL_free(coordinate);
                    }
                    if (num_processes >= subcomm_min_size) {
                        total_num_processes += num_processes;
                        num_processes = 0;
                        for (j = 0; j < torus_dim; j++) {
                            offset_along_dimension[i] += partition[j] + 1;
                        }
                        break;
                    }
                }
                if (total_num_processes == num_nodes || node_covered) {
                    break;
                }
            }
            MPL_free(offset_along_dimension);
            MPL_free(partition);
        }

        mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        /* There are more processes in the subset than requested within the node.
         * Split further inside each node */
        if (num_processes_at_node[node_index] > subcomm_min_size && node_index == color &&
            num_processes_at_node[node_index] < subcomm_min_size) {
            MPIR_Comm *node_comm;
            int subcomm_rank;
            int tree_depth;
            int min_tree_depth = -1;
            int num_procs;

            num_procs = num_processes_at_node[node_index];
            node_comm = *newcomm_ptr;
            subcomm_rank = MPIR_Comm_rank(node_comm);
            MPIR_Node_obj obj_containing_cpuset = MPIR_Node_get_covering_obj();

            /* get depth in topology tree */
            tree_depth = MPIR_Node_get_obj_depth(obj_containing_cpuset);

            /* get min tree depth to all processes */
            MPID_Allreduce(&tree_depth, &min_tree_depth, 1, MPI_INT, MPI_MIN, node_comm, &errflag);

            if (min_tree_depth) {
                int num_hwloc_objs_at_depth;
                int *parent_idx = MPL_calloc(num_procs, sizeof(int), MPL_MEM_OTHER);

                while (tree_depth != min_tree_depth) {
                    obj_containing_cpuset = MPIR_Node_get_parent_obj(obj_containing_cpuset);
                    tree_depth = MPIR_Node_get_obj_depth(obj_containing_cpuset);
                }
                parent_idx[subcomm_rank] = MPIR_Node_get_obj_index(obj_containing_cpuset);

                /* get parent_idx to all processes */
                MPID_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, parent_idx, 1, MPI_INT,
                               node_comm, &errflag);

                /* reorder parent indices */
                for (i = 0; i < num_procs - 1; i++) {
                    for (j = 0; j < num_procs - i - 1; j++) {
                        if (parent_idx[j] > parent_idx[j + 1]) {
                            int tmp = parent_idx[j];
                            parent_idx[j] = parent_idx[j + 1];
                            parent_idx[j + 1] = tmp;
                        }
                    }
                }

                /* get number of parents and unique indices */
                int *obj_idx_at_depth = MPL_malloc(sizeof(int) * num_procs, MPL_MEM_OTHER);
                obj_idx_at_depth[0] = parent_idx[0];
                int current_idx = parent_idx[0];
                num_hwloc_objs_at_depth = 1;
                for (i = 1; i < num_procs; i++) {
                    if (parent_idx[i] != current_idx) {
                        obj_idx_at_depth[num_hwloc_objs_at_depth++] = parent_idx[i];
                        current_idx = parent_idx[i];
                    }
                }

                int *processes_cpuset =
                    (int *) MPL_calloc(num_hwloc_objs_at_depth, sizeof(int), MPL_MEM_OTHER);

                int hw_obj_index;
                int current_proc_index = -1;

                for (hw_obj_index = 0; hw_obj_index < num_hwloc_objs_at_depth; hw_obj_index++) {
                    for (i = 0; i < num_procs; i++) {
                        if (parent_idx[i] == obj_idx_at_depth[hw_obj_index]) {
                            processes_cpuset[hw_obj_index]++;
                            if (i == subcomm_rank) {
                                current_proc_index = hw_obj_index;
                            }
                            break;
                        }
                    }
                }

                color =
                    get_color_from_subset_bitmap(current_proc_index, processes_cpuset,
                                                 num_hwloc_objs_at_depth, subcomm_min_size);

                mpi_errno = MPIR_Comm_split_impl(node_comm, color, key, newcomm_ptr);
                MPIR_ERR_CHECK(mpi_errno);
                MPL_free(processes_cpuset);
                MPL_free(parent_idx);
                MPL_free(obj_idx_at_depth);
                MPIR_Comm_free_impl(node_comm);
            }
        }
        MPL_free(num_processes_at_node);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int network_split_by_min_memsize(MPIR_Comm * comm_ptr, int key, long min_mem_size,
                                        MPIR_Comm ** newcomm_ptr)
{

    int mpi_errno = MPI_SUCCESS;
    MPIR_Network_topology_type topo_type;

    /* Get available memory in the node */
    long total_memory_size = 0;
    int memory_per_process;

    total_memory_size = MPIR_Node_get_total_mem();

    topo_type = MPIR_Net_get_topo_type();

    if (min_mem_size == 0 || topo_type == MPIR_NETWORK_TOPOLOGY_TYPE__INVALID) {
        *newcomm_ptr = NULL;
    } else {
        int num_ranks_node;
        if (MPIR_Process.comm_world->node_comm != NULL) {
            num_ranks_node = MPIR_Comm_size(MPIR_Process.comm_world->node_comm);
        } else {
            num_ranks_node = 1;
        }
        memory_per_process = total_memory_size / num_ranks_node;
        mpi_errno = network_split_by_minsize(comm_ptr, key, min_mem_size / memory_per_process,
                                             newcomm_ptr);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int network_split_by_torus_dimension(MPIR_Comm * comm_ptr, int key,
                                            int dimension, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, color;
    int torus_dim;
    MPIR_Network_topology_type topo_type;

    topo_type = MPIR_Net_get_topo_type();
    torus_dim = MPIR_Net_torus_topo_get_dimension();

    /* Dimension is assumed to be indexed from 0 */
    if (topo_type != MPIR_NETWORK_TOPOLOGY_TYPE__TORUS || dimension >= torus_dim) {
        *newcomm_ptr = NULL;
    } else {
        int node_coordinates = MPIR_Net_torus_topo_get_node_index();
        int *node_dimensions = MPIR_Net_torus_topo_get_geometry();
        color = 0;
        for (i = 0; i < torus_dim; i++) {
            int coordinate_along_dim;
            if (i == dimension) {
                coordinate_along_dim = 0;
            } else {
                coordinate_along_dim = node_coordinates % node_dimensions[i];
            }
            if (i == 0) {
                color = coordinate_along_dim;
            } else {
                color = color + coordinate_along_dim * node_dimensions[i - 1];
            }
            node_coordinates = node_coordinates / node_dimensions[i];
        }
        mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static const char *SHMEM_INFO_KEY = "shmem_topo";
static const char *NETWORK_INFO_KEY = "network_topo";

static int compare_info_hint(const char *hintval, MPIR_Comm * comm_ptr, int *info_args_are_equal)
{
    int hintval_size = strlen(hintval);
    int hintval_size_max;
    int hintval_equal;
    int hintval_equal_global = 0;
    char *hintval_global = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    /* Find the maximum hintval size.  Each process locally compares
     * its hintval size to the global max, and makes sure that this
     * comparison is successful on all processes. */
    mpi_errno =
        MPID_Allreduce(&hintval_size, &hintval_size_max, 1, MPI_INT, MPI_MAX, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    hintval_equal = (hintval_size == hintval_size_max);

    mpi_errno =
        MPID_Allreduce(&hintval_equal, &hintval_equal_global, 1, MPI_INT, MPI_LAND,
                       comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    if (!hintval_equal_global)
        goto fn_exit;


    /* Now that the sizes of the hintvals match, check to make sure
     * the actual hintvals themselves are the equal */
    hintval_global = (char *) MPL_malloc(strlen(hintval), MPL_MEM_OTHER);

    mpi_errno =
        MPID_Allreduce(hintval, hintval_global, strlen(hintval), MPI_CHAR,
                       MPI_MAX, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    hintval_equal = !memcmp(hintval, hintval_global, strlen(hintval));

    mpi_errno =
        MPID_Allreduce(&hintval_equal, &hintval_equal_global, 1, MPI_INT, MPI_LAND,
                       comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPL_free(hintval_global);

    *info_args_are_equal = hintval_equal_global;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_node_topo(MPIR_Comm * user_comm_ptr, int split_type, int key,
                                   MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr;
    int mpi_errno = MPI_SUCCESS;
    int flag = 0;
    char hintval[MPI_MAX_INFO_VAL + 1];
    int info_args_are_equal;
    *newcomm_ptr = NULL;

    mpi_errno = MPIR_Comm_split_type_node(user_comm_ptr, split_type, key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (comm_ptr == NULL) {
        MPIR_Assert(split_type == MPI_UNDEFINED);
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (info_ptr) {
        MPIR_Info_get_impl(info_ptr, SHMEM_INFO_KEY, MPI_MAX_INFO_VAL, hintval, &flag);
    }

    if (!flag) {
        hintval[0] = '\0';
    }

    mpi_errno = compare_info_hint(hintval, comm_ptr, &info_args_are_equal);

    MPIR_ERR_CHECK(mpi_errno);

    /* if all processes do not have the same hintval, skip
     * topology-aware comm split */
    if (!info_args_are_equal)
        goto use_node_comm;

    /* if no info key is given, skip topology-aware comm split */
    if (!info_ptr)
        goto use_node_comm;

    /* if hw topology is not initialized, skip topology-aware comm split */
    if (!MPIR_hwtopo_is_initialized())
        goto use_node_comm;

    if (flag) {
        if (!strncmp(hintval, "pci:", strlen("pci:")))
            mpi_errno = node_split_pci_device(comm_ptr, key, hintval, newcomm_ptr);
        else if (!strncmp(hintval, "ib", strlen("ib")) ||
                 !strncmp(hintval, "en", strlen("en")) ||
                 !strncmp(hintval, "eth", strlen("eth")) || !strncmp(hintval, "hfi", strlen("hfi")))
            mpi_errno = node_split_network_device(comm_ptr, key, hintval, newcomm_ptr);
        else if (!strncmp(hintval, "gpu", strlen("gpu")))
            mpi_errno = node_split_gpu_device(comm_ptr, key, hintval, newcomm_ptr);
        else
            mpi_errno = node_split_processor(comm_ptr, key, hintval, newcomm_ptr);

        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Comm_free_impl(comm_ptr);

        goto fn_exit;
    }

  use_node_comm:
    *newcomm_ptr = comm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_network_topo(MPIR_Comm * comm_ptr, int key, const char *hintval,
                                      MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    if (!strncmp(hintval, ("switch_level:"), strlen("switch_level:"))
        && *(hintval + strlen("switch_level:")) != '\0') {
        int switch_level = atoi(hintval + strlen("switch_level:"));
        mpi_errno = network_split_switch_level(comm_ptr, key, switch_level, newcomm_ptr);
    } else if (!strncmp(hintval, ("subcomm_min_size:"), strlen("subcomm_min_size:"))
               && *(hintval + strlen("subcomm_min_size:")) != '\0') {
        int subcomm_min_size = atoi(hintval + strlen("subcomm_min_size:"));
        mpi_errno = network_split_by_minsize(comm_ptr, key, subcomm_min_size, newcomm_ptr);
    } else if (!strncmp(hintval, ("min_mem_size:"), strlen("min_mem_size:"))
               && *(hintval + strlen("min_mem_size:")) != '\0') {
        long min_mem_size = atol(hintval + strlen("min_mem_size:"));
        /* Split by minimum memory size per subcommunicator in bytes */
        mpi_errno = network_split_by_min_memsize(comm_ptr, key, min_mem_size, newcomm_ptr);
    } else if (!strncmp(hintval, ("torus_dimension:"), strlen("torus_dimension:"))
               && *(hintval + strlen("torus_dimension:")) != '\0') {
        int dimension = atol(hintval + strlen("torus_dimension:"));
        mpi_errno = network_split_by_torus_dimension(comm_ptr, key, dimension, newcomm_ptr);
    }
    return mpi_errno;
}

int MPIR_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key,
                         MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (split_type == MPI_COMM_TYPE_SHARED) {
        mpi_errno = MPIR_Comm_split_type_self(comm_ptr, split_type, key, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (split_type == MPIX_COMM_TYPE_NEIGHBORHOOD) {
        mpi_errno =
            MPIR_Comm_split_type_neighborhood(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_ARG, "**arg");
    }

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_nbhd_common_dir(MPIR_Comm * user_comm_ptr, int key, const char *hintval,
                                         MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ROMIO
    MPI_Comm dummycomm;
    MPIR_Comm *dummycomm_ptr;

    mpi_errno = MPIR_Comm_split_filesystem(user_comm_ptr->handle, key, hintval, &dummycomm);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm_get_ptr(dummycomm, dummycomm_ptr);
    *newcomm_ptr = dummycomm_ptr;
#endif

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_neighborhood(MPIR_Comm * comm_ptr, int split_type, int key,
                                      MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{

    int flag = 0;
    char hintval[MPI_MAX_INFO_VAL + 1];
    int mpi_errno = MPI_SUCCESS;
    int info_args_are_equal;

    *newcomm_ptr = NULL;

    if (info_ptr) {
        MPIR_Info_get_impl(info_ptr, "nbhd_common_dirname", MPI_MAX_INFO_VAL, hintval, &flag);
    }
    if (!flag) {
        hintval[0] = '\0';
    }

    *newcomm_ptr = NULL;
    mpi_errno = compare_info_hint(hintval, comm_ptr, &info_args_are_equal);

    MPIR_ERR_CHECK(mpi_errno);

    if (info_args_are_equal && flag) {
        MPIR_Comm_split_type_nbhd_common_dir(comm_ptr, key, hintval, newcomm_ptr);
    } else {
        /* Check if the info hint is a network topology hint */
        if (info_ptr) {
            MPIR_Info_get_impl(info_ptr, NETWORK_INFO_KEY, MPI_MAX_INFO_VAL, hintval, &flag);
        }

        if (!flag) {
            hintval[0] = '\0';
        }

        mpi_errno = compare_info_hint(hintval, comm_ptr, &info_args_are_equal);

        MPIR_ERR_CHECK(mpi_errno);

        /* if all processes have the same hintval, perform
         * topology-aware comm split */
        if (info_args_are_equal) {
            MPIR_Comm_split_type_network_topo(comm_ptr, key, hintval, newcomm_ptr);
        }
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Comm_split_type_impl(MPIR_Comm * comm_ptr, int split_type, int key,
                              MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Only MPI_COMM_TYPE_SHARED, MPI_UNDEFINED, and
     * NEIGHBORHOOD are supported */
    MPIR_Assert(split_type == MPI_COMM_TYPE_SHARED ||
                split_type == MPI_UNDEFINED || split_type == MPIX_COMM_TYPE_NEIGHBORHOOD);

    if (MPIR_Comm_fns != NULL && MPIR_Comm_fns->split_type != NULL) {
        mpi_errno = MPIR_Comm_fns->split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    } else {
        mpi_errno = MPIR_Comm_split_type(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@

MPI_Comm_split_type - Creates new communicators based on split types and keys

Input Parameters:
+ comm - communicator (handle)
. split_type - type of processes to be grouped together (nonnegative integer).
. key - control of rank assignment (integer)
- info - hints to improve communicator creation (handle)

Output Parameters:
. newcomm - new communicator (handle)

Notes:
  The 'split_type' must be non-negative or 'MPI_UNDEFINED'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Comm_free
@*/
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPIR_Info *info_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_SPLIT_TYPE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_SPLIT_TYPE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }

#endif /* HAVE_ERROR_CHECKING */

    /* Get handles to MPI objects. */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(newcomm, "newcomm", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_split_type_impl(comm_ptr, split_type, key, info_ptr, &newcomm_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    if (newcomm_ptr)
        MPIR_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_SPLIT_TYPE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        /* FIXME this error code is wrong, it's the error code for
         * regular MPI_Comm_split */
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_split",
                                 "**mpi_comm_split %C %d %d %p", comm, split_type, key, newcomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
