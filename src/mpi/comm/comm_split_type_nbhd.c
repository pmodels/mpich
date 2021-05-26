/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpicomm.h"

static const char *NETWORK_INFO_KEY = "network_topo";

static int network_split_switch_level(MPIR_Comm * comm_ptr, int key,
                                      int switch_level, MPIR_Comm ** newcomm_ptr);
static int network_split_by_minsize(MPIR_Comm * comm_ptr, int key, int subcomm_min_size,
                                    MPIR_Comm ** newcomm_ptr);
static int get_color_from_subset_bitmap(int node_index, int *bitmap, int bitmap_size, int min_size);
static int network_split_by_min_memsize(MPIR_Comm * comm_ptr, int key, long min_mem_size,
                                        MPIR_Comm ** newcomm_ptr);
static int network_split_by_torus_dimension(MPIR_Comm * comm_ptr, int key,
                                            int dimension, MPIR_Comm ** newcomm_ptr);

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
    /* check whether all processes are using the same dirname */
    mpi_errno = MPII_compare_info_hint(hintval, comm_ptr, &info_args_are_equal);

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

        /* check whether all processes are using the same hint */
        mpi_errno = MPII_compare_info_hint(hintval, comm_ptr, &info_args_are_equal);

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

int MPIR_Comm_split_type_nbhd_common_dir(MPIR_Comm * user_comm_ptr, int key, const char *hintval,
                                         MPIR_Comm ** newcomm_ptr)
{
#ifndef HAVE_ROMIO
    *newcomm_ptr = NULL;
    return MPI_SUCCESS;
#else
    int mpi_errno = MPI_SUCCESS;
    MPI_Comm dummycomm;
    MPIR_Comm *dummycomm_ptr;

    /* ROMIO will call MPI-functions. Take off the lock taken by MPI_Comm_split_type */
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno = MPIR_Comm_split_filesystem(user_comm_ptr->handle, key, hintval, &dummycomm);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Comm_get_ptr(dummycomm, dummycomm_ptr);
    *newcomm_ptr = dummycomm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
#endif
}

static int network_split_switch_level(MPIR_Comm * comm_ptr, int key,
                                      int switch_level, MPIR_Comm ** newcomm_ptr)
{
    int i, color;
    int mpi_errno = MPI_SUCCESS;
    MPIR_nettopo_node_t network_node;
    MPIR_nettopo_node_t *traversal_stack;
    MPIR_nettopo_type_e topo_type;
    int num_nodes;
    int traversal_begin, traversal_end;

    topo_type = MPIR_nettopo_get_type();
    num_nodes = MPIR_nettopo_get_num_nodes();

    if (topo_type == MPIR_NETTOPO_TYPE__FAT_TREE || topo_type == MPIR_NETTOPO_TYPE__CLOS_NETWORK) {
        MPIR_nettopo_node_t *switches_at_level;
        int switch_count;
        traversal_stack =
            (MPIR_nettopo_node_t *) MPL_malloc(sizeof(MPIR_nettopo_node_t) * num_nodes,
                                               MPL_MEM_OTHER);

        network_node = MPIR_nettopo_get_endpoint();

        traversal_begin = 0;
        traversal_end = 0;
        MPIR_nettopo_tree_get_switches_at_level(switch_level, &switches_at_level, &switch_count);

        /* Find the switch `switch_level` steps away */
        MPIR_Assert(traversal_end < num_nodes);
        traversal_stack[traversal_end++] = network_node;
        color = 0;
        while (traversal_end > traversal_begin) {
            MPIR_nettopo_node_t current_node = traversal_stack[traversal_begin++];
            int num_edges;
            int node_uid = MPIR_nettopo_get_node_uid(current_node);
            int *node_levels = MPIR_nettopo_tree_get_node_levels();
            MPIR_nettopo_node_type_e node_type = MPIR_nettopo_get_node_type(current_node);
            MPIR_nettopo_edge_t *edges;
            if (node_type == MPIR_NETTOPO_NODE_TYPE__SWITCH && node_levels[node_uid]
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
            MPIR_nettopo_get_all_edges(network_node, &num_edges, &edges);
            for (i = 0; i < num_edges; i++) {
                MPIR_Assert(traversal_end < num_nodes);
                traversal_stack[traversal_end++] = MPIR_nettopo_get_edge_dest_node(edges[i]);
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

    return color;
}

static int network_split_by_minsize(MPIR_Comm * comm_ptr, int key, int subcomm_min_size,
                                    MPIR_Comm ** newcomm_ptr)
{

    int mpi_errno = MPI_SUCCESS;
    int i, j, color;
    int comm_size = MPIR_Comm_size(comm_ptr);
    int node_index;
    int num_nodes;
    MPIR_nettopo_type_e topo_type;

    num_nodes = MPIR_nettopo_get_num_nodes();
    topo_type = MPIR_nettopo_get_type();

    if (subcomm_min_size == 0 || comm_size < subcomm_min_size ||
        topo_type == MPIR_NETTOPO_TYPE__INVALID) {
        *newcomm_ptr = NULL;
    } else {
        int *num_processes_at_node = NULL;
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;

        if (topo_type == MPIR_NETTOPO_TYPE__FAT_TREE ||
            topo_type == MPIR_NETTOPO_TYPE__CLOS_NETWORK) {
            mpi_errno = MPIR_nettopo_tree_get_hostnode_index(&node_index, &num_nodes);

            MPIR_ERR_CHECK(mpi_errno);

            num_processes_at_node = (int *) MPL_calloc(1, sizeof(int) * num_nodes, MPL_MEM_OTHER);
            num_processes_at_node[node_index] = 1;

        } else if (topo_type == MPIR_NETTOPO_TYPE__TORUS) {
            num_processes_at_node = (int *) MPL_calloc(1, sizeof(int) * num_nodes, MPL_MEM_OTHER);
            node_index = MPIR_nettopo_torus_get_node_index();
            num_processes_at_node[node_index] = 1;
        }
        MPIR_Assert(num_processes_at_node != NULL);
        /* Send the count to processes */
        mpi_errno =
            MPID_Allreduce(MPI_IN_PLACE, num_processes_at_node, num_nodes, MPI_INT,
                           MPI_SUM, comm_ptr, &errflag);

        if (topo_type == MPIR_NETTOPO_TYPE__FAT_TREE ||
            topo_type == MPIR_NETTOPO_TYPE__CLOS_NETWORK) {
            color =
                get_color_from_subset_bitmap(node_index, num_processes_at_node, num_nodes,
                                             subcomm_min_size);
        } else {
            int torus_dim = MPIR_nettopo_torus_get_dimension();
            int *torus_geometry = MPIR_nettopo_torus_get_geometry();
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
            MPIR_hwtopo_gid_t obj_containing_cpuset = MPIR_hwtopo_get_leaf();

            /* get depth in topology tree */
            tree_depth = MPIR_hwtopo_get_depth(obj_containing_cpuset);

            /* get min tree depth to all processes */
            MPID_Allreduce(&tree_depth, &min_tree_depth, 1, MPI_INT, MPI_MIN, node_comm, &errflag);

            if (min_tree_depth) {
                int num_hwloc_objs_at_depth;
                int *parent_idx = MPL_calloc(num_procs, sizeof(int), MPL_MEM_OTHER);

                while (tree_depth != min_tree_depth) {
                    obj_containing_cpuset =
                        MPIR_hwtopo_get_ancestor(obj_containing_cpuset, --tree_depth);
                }
                parent_idx[subcomm_rank] = obj_containing_cpuset;

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
    MPIR_nettopo_type_e topo_type;

    /* Get available memory in the node */
    long total_memory_size = 0;
    int memory_per_process;

    total_memory_size = MPIR_hwtopo_get_node_mem();

    topo_type = MPIR_nettopo_get_type();

    if (min_mem_size == 0 || topo_type == MPIR_NETTOPO_TYPE__INVALID) {
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

    return mpi_errno;
}

static int network_split_by_torus_dimension(MPIR_Comm * comm_ptr, int key,
                                            int dimension, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, color;
    int torus_dim;
    MPIR_nettopo_type_e topo_type;

    topo_type = MPIR_nettopo_get_type();
    torus_dim = MPIR_nettopo_torus_get_dimension();

    /* Dimension is assumed to be indexed from 0 */
    if (topo_type != MPIR_NETTOPO_TYPE__TORUS || dimension >= torus_dim) {
        *newcomm_ptr = NULL;
    } else {
        int node_coordinates = MPIR_nettopo_torus_get_node_index();
        int *node_dimensions = MPIR_nettopo_torus_get_geometry();
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

    return mpi_errno;
}
