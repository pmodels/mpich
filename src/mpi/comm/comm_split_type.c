/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

#ifdef HAVE_HWLOC
#include "hwloc.h"
#endif

#ifdef HAVE_NETLOC
#include "netloc_util.h"
#endif

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

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_self
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_self(MPIR_Comm * user_comm_ptr, int split_type, int key,
                              MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *comm_self_ptr;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
    mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, newcomm_ptr);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_node
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_node(MPIR_Comm * user_comm_ptr, int split_type, int key,
                              MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (comm_ptr)
        MPIR_Comm_free_impl(comm_ptr);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#ifdef HAVE_HWLOC

struct shmem_processor_info_table {
    const char *val;
    hwloc_obj_type_t obj_type;
};

/* hwloc processor object table */
static struct shmem_processor_info_table shmem_processor_info[] = {
    {"machine", HWLOC_OBJ_MACHINE},
    {"socket", HWLOC_OBJ_PACKAGE},
    {"package", HWLOC_OBJ_PACKAGE},
    {"numa", HWLOC_OBJ_NUMANODE},
    {"core", HWLOC_OBJ_CORE},
    {"hwthread", HWLOC_OBJ_PU},
    {"pu", HWLOC_OBJ_PU},
    {"l1dcache", HWLOC_OBJ_L1CACHE},
    {"l1ucache", HWLOC_OBJ_L1CACHE},
    {"l1icache", HWLOC_OBJ_L1ICACHE},
    {"l1cache", HWLOC_OBJ_L1CACHE},
    {"l2dcache", HWLOC_OBJ_L2CACHE},
    {"l2ucache", HWLOC_OBJ_L2CACHE},
    {"l2icache", HWLOC_OBJ_L2ICACHE},
    {"l2cache", HWLOC_OBJ_L2CACHE},
    {"l3dcache", HWLOC_OBJ_L3CACHE},
    {"l3ucache", HWLOC_OBJ_L3CACHE},
    {"l3icache", HWLOC_OBJ_L3ICACHE},
    {"l3cache", HWLOC_OBJ_L3CACHE},
    {"l4dcache", HWLOC_OBJ_L4CACHE},
    {"l4ucache", HWLOC_OBJ_L4CACHE},
    {"l4cache", HWLOC_OBJ_L4CACHE},
    {"l5dcache", HWLOC_OBJ_L5CACHE},
    {"l5ucache", HWLOC_OBJ_L5CACHE},
    {"l5cache", HWLOC_OBJ_L5CACHE},
    {NULL, HWLOC_OBJ_TYPE_MAX}
};

static int node_split_processor(MPIR_Comm * comm_ptr, int key, const char *hintval,
                                MPIR_Comm ** newcomm_ptr)
{
    int color;
    hwloc_obj_t obj_containing_cpuset;
    hwloc_obj_type_t query_obj_type = HWLOC_OBJ_TYPE_MAX;
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

    if (query_obj_type == HWLOC_OBJ_TYPE_MAX)
        goto split_id;

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.hwloc_topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);
    if (obj_containing_cpuset->type == query_obj_type) {
        color = obj_containing_cpuset->logical_index;
    } else {
        hwloc_obj_t hobj = NULL;
        hwloc_obj_t tmp = NULL;
        /* hwloc_get_ancestor_of_type call cannot be used here because HWLOC version 2.0 and above do not
         * treat memory objects (NUMA) as objects in topology tree (Details can be found in
         * https://www.open-mpi.org/projects/hwloc/doc/v2.0.1/a00327.php#upgrade_to_api_2x_memory_find)
         */
        while ((tmp =
                hwloc_get_next_obj_by_type(MPIR_Process.hwloc_topology, query_obj_type,
                                           tmp)) != NULL) {
            if (hwloc_bitmap_isincluded(obj_containing_cpuset->cpuset, tmp->cpuset) ||
                hwloc_bitmap_isequal(tmp->cpuset, obj_containing_cpuset->cpuset)) {
                hobj = tmp;
                break;
            }
        }
        if (hobj)
            color = hobj->logical_index;
        else
            color = MPI_UNDEFINED;
    }

  split_id:
    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split_pci_device(MPIR_Comm * comm_ptr, int key,
                                 const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    hwloc_obj_t obj_containing_cpuset, io_device = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.hwloc_topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);

    io_device =
        hwloc_get_pcidev_by_busidstring(MPIR_Process.hwloc_topology, hintval + strlen("pci:"));

    if (io_device != NULL) {
        hwloc_obj_t non_io_ancestor =
            hwloc_get_non_io_ancestor_obj(MPIR_Process.hwloc_topology, io_device);

        /* An io object will never be the root of the topology and is
         * hence guaranteed to have a non io ancestor */
        MPIR_Assert(non_io_ancestor);

        if (hwloc_obj_is_in_subtree
            (MPIR_Process.hwloc_topology, obj_containing_cpuset, non_io_ancestor)) {
            color = non_io_ancestor->logical_index;
        } else
            color = MPI_UNDEFINED;
    } else
        color = MPI_UNDEFINED;

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int io_device_found(const char *resource, const char *devname, hwloc_obj_t io_device,
                           hwloc_obj_osdev_type_t obj_type)
{
    if (!strncmp(resource, devname, strlen(devname))) {
        /* device type does not match */
        if (io_device->attr->osdev.type != obj_type)
            return 0;

        /* device prefix does not match */
        if (strncmp(io_device->name, devname, strlen(devname)))
            return 0;

        /* specific device is supplied, but does not match */
        if (strlen(resource) != strlen(devname) && strcmp(io_device->name, resource))
            return 0;
    }

    return 1;
}

static int node_split_network_device(MPIR_Comm * comm_ptr, int key,
                                     const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    hwloc_obj_t obj_containing_cpuset, io_device = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    /* assign the node id as the color, initially */
    MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.hwloc_topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);

    color = MPI_UNDEFINED;
    while ((io_device = hwloc_get_next_osdev(MPIR_Process.hwloc_topology, io_device))) {
        hwloc_obj_t non_io_ancestor;
        uint32_t depth;

        if (!io_device_found(hintval, "hfi", io_device, HWLOC_OBJ_OSDEV_OPENFABRICS))
            continue;
        if (!io_device_found(hintval, "ib", io_device, HWLOC_OBJ_OSDEV_NETWORK))
            continue;
        if (!io_device_found(hintval, "eth", io_device, HWLOC_OBJ_OSDEV_NETWORK) &&
            !io_device_found(hintval, "en", io_device, HWLOC_OBJ_OSDEV_NETWORK))
            continue;

        non_io_ancestor = hwloc_get_non_io_ancestor_obj(MPIR_Process.hwloc_topology, io_device);
        while (!hwloc_obj_type_is_normal(non_io_ancestor->type))
            non_io_ancestor = non_io_ancestor->parent;
        MPIR_Assert(non_io_ancestor && non_io_ancestor->depth >= 0);

        if (!hwloc_obj_is_in_subtree
            (MPIR_Process.hwloc_topology, obj_containing_cpuset, non_io_ancestor))
            continue;

        /* Get a unique ID for the non-IO object.  Use fixed width
         * unsigned integers, so bit shift operations are well
         * defined */
        depth = (uint32_t) non_io_ancestor->depth;
        color = (int) ((depth << 16) + non_io_ancestor->logical_index);
        break;
    }

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int node_split_gpu_device(MPIR_Comm * comm_ptr, int key,
                                 const char *hintval, MPIR_Comm ** newcomm_ptr)
{
    hwloc_obj_t obj_containing_cpuset, io_device = NULL;
    int mpi_errno = MPI_SUCCESS;
    int color;

    obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(MPIR_Process.hwloc_topology, MPIR_Process.bindset);
    MPIR_Assert(obj_containing_cpuset != NULL);

    color = MPI_UNDEFINED;
    while ((io_device = hwloc_get_next_osdev(MPIR_Process.hwloc_topology, io_device))
           != NULL) {
        if (io_device->attr->osdev.type == HWLOC_OBJ_OSDEV_GPU) {
            if ((*(hintval + strlen("gpu")) != '\0') &&
                atoi(hintval + strlen("gpu")) != io_device->logical_index)
                continue;
            hwloc_obj_t non_io_ancestor =
                hwloc_get_non_io_ancestor_obj(MPIR_Process.hwloc_topology, io_device);
            MPIR_Assert(non_io_ancestor);
            if (hwloc_obj_is_in_subtree
                (MPIR_Process.hwloc_topology, obj_containing_cpuset, non_io_ancestor)) {
                color =
                    (non_io_ancestor->type << (sizeof(int) * 4)) + non_io_ancestor->logical_index;
                break;
            }
        }
    }

    mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
#endif /* HAVE_HWLOC */

#ifdef HAVE_NETLOC
static int network_split_switch_level(MPIR_Comm * comm_ptr, int key,
                                      int switch_level, MPIR_Comm ** newcomm_ptr)
{
    int i, color;
    int mpi_errno = MPI_SUCCESS;
    netloc_node_t *network_node;
    netloc_node_t **traversal_stack;
    int traversal_begin, traversal_end;

    if (MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__FAT_TREE ||
        MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__CLOS_NETWORK) {
        netloc_node_t **switches_at_level;
        int switch_count;
        traversal_stack =
            (netloc_node_t **) MPL_malloc(sizeof(netloc_node_t *) *
                                          MPIR_Process.netloc_topology->num_nodes, MPL_MEM_OTHER);

        network_node = MPIR_Process.network_attr.network_endpoint;

        traversal_begin = 0;
        traversal_end = 0;
        MPIR_Netloc_get_switches_at_level(MPIR_Process.netloc_topology, MPIR_Process.network_attr,
                                          switch_level, &switches_at_level, &switch_count);
        /* Find the switch `switch_level` steps away */
        MPIR_Assert(traversal_end < MPIR_Process.netloc_topology->num_nodes);
        traversal_stack[traversal_end++] = network_node;
        color = 0;
        while (traversal_end > traversal_begin) {
            netloc_node_t *current_node = traversal_stack[traversal_begin++];
            int num_edges;
            netloc_edge_t **edges;
            if (current_node->node_type == NETLOC_NODE_TYPE_SWITCH
                && MPIR_Process.network_attr.u.tree.node_levels[current_node->__uid__]
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
            netloc_get_all_edges(MPIR_Process.netloc_topology, network_node, &num_edges, &edges);
            for (i = 0; i < num_edges; i++) {
                MPIR_Assert(traversal_end < MPIR_Process.netloc_topology->num_nodes);
                traversal_stack[traversal_end++] = edges[i]->dest_node;
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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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
    int i, color;
    int comm_size = MPIR_Comm_size(comm_ptr);
    netloc_node_t *network_node;

    if (subcomm_min_size == 0 || comm_size < subcomm_min_size ||
        MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__INVALID) {
        *newcomm_ptr = NULL;
    } else {
        int node_index, num_nodes, i;
        int *num_processes_at_node = NULL;
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        int subset_size;
        int current_comm_color;
        int prev_comm_color;

        network_node = MPIR_Process.network_attr.network_endpoint;

        if (MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__FAT_TREE ||
            MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__CLOS_NETWORK) {
            mpi_errno =
                MPIR_Netloc_get_hostnode_index_in_tree(MPIR_Process.network_attr,
                                                       MPIR_Process.netloc_topology, network_node,
                                                       &node_index, &num_nodes);

            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            num_processes_at_node = (int *) MPL_calloc(1, sizeof(int) * num_nodes, MPL_MEM_OTHER);
            num_processes_at_node[node_index] = 1;

        } else if (MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__TORUS) {
            num_processes_at_node =
                (int *) MPL_calloc(1, sizeof(int) * MPIR_Process.netloc_topology->num_nodes,
                                   MPL_MEM_OTHER);
            num_processes_at_node[MPIR_Process.network_attr.u.torus.node_idx] = 1;
        }
        MPIR_Assert(num_processes_at_node != NULL);
        /* Send the count to processes */
        mpi_errno =
            MPID_Allreduce(MPI_IN_PLACE, num_processes_at_node, num_nodes, MPI_INT,
                           MPI_SUM, comm_ptr, &errflag);

        if (MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__FAT_TREE ||
            MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__CLOS_NETWORK) {
            color =
                get_color_from_subset_bitmap(node_index, num_processes_at_node, num_nodes,
                                             subcomm_min_size);
        } else {
            int *offset_along_dimension =
                (int *) MPL_calloc(MPIR_Process.network_attr.u.torus.dimension, sizeof(int),
                                   MPL_MEM_OTHER);
            int *partition =
                (int *) MPL_calloc(MPIR_Process.network_attr.u.torus.dimension, sizeof(int),
                                   MPL_MEM_OTHER);
            int start_index = offset_along_dimension[0];
            int num_processes = 0, total_num_processes = 0;
            int j, size;

            for (i = 0; i < MPIR_Process.network_attr.u.torus.dimension; i++) {
                partition[i] = 1;
            }

            while (1) {
                int node_covered = 0;
                color = total_num_processes;
                for (i = 0; i < MPIR_Process.network_attr.u.torus.dimension;
                     i = (i + 1) % MPIR_Process.network_attr.u.torus.dimension) {
                    int cube_size;
                    if (partition[i] - 1 + offset_along_dimension[i] ==
                        MPIR_Process.network_attr.u.torus.geometry[i]) {
                        if (i == MPIR_Process.network_attr.u.torus.dimension - 1) {
                            break;
                        }
                        continue;
                    }
                    partition[i]++;
                    cube_size = 0;
                    for (j = 0; j < MPIR_Process.network_attr.u.torus.dimension; j++) {
                        if (partition[j] != 0) {
                            cube_size = cube_size * partition[j];
                        }
                    }
                    num_processes = 0;
                    for (j = 0; j < cube_size; j++) {
                        int *coordinate =
                            (int *) MPL_calloc(MPIR_Process.network_attr.u.torus.dimension,
                                               sizeof(int), MPL_MEM_OTHER);
                        int index = j;
                        int k;
                        int current_dim = 0;
                        while (current_dim < MPIR_Process.network_attr.u.torus.dimension) {
                            coordinate[current_dim++] = index % partition[j];
                            index = index / partition[j];
                        }
                        index = 0;
                        for (k = 0; k < MPIR_Process.network_attr.u.torus.dimension; k++) {
                            index =
                                index * (partition[j] + offset_along_dimension[i]) + coordinate[k];
                        }
                        if (index == MPIR_Process.network_attr.u.torus.node_idx) {
                            node_covered = 1;
                            break;
                        }
                        num_processes += num_processes_at_node[index];
                        MPL_free(coordinate);
                    }
                    if (num_processes >= subcomm_min_size) {
                        total_num_processes += num_processes;
                        num_processes = 0;
                        for (j = 0; j < MPIR_Process.network_attr.u.torus.dimension; j++) {
                            offset_along_dimension[i] += partition[j] + 1;
                        }
                        break;
                    }
                }
                if (total_num_processes == MPIR_Process.netloc_topology->num_nodes || node_covered) {
                    break;
                }
            }
            MPL_free(offset_along_dimension);
            MPL_free(partition);
        }

        mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, newcomm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* There are more processes in the subset than requested within the node.
         * Split further inside each node */
        if (num_processes_at_node[node_index] > subcomm_min_size && node_index == color &&
            ((node_index < (node_index - 1) ||
              num_processes_at_node[node_index] < subcomm_min_size))) {
            MPIR_Comm *node_comm;
            int subcomm_rank;
            int min_tree_depth;
            hwloc_cpuset_t *node_comm_bindset;
            int num_procs;

            num_procs = num_processes_at_node[node_index];
            node_comm = *newcomm_ptr;
            subcomm_rank = MPIR_Comm_rank(node_comm);
            node_comm_bindset =
                (hwloc_cpuset_t *) MPL_calloc(num_procs, sizeof(hwloc_cpuset_t), MPL_MEM_OTHER);
            node_comm_bindset[subcomm_rank] = MPIR_Process.bindset;

            /* Send the bindset to processes in node communicator */
            mpi_errno =
                MPID_Allreduce(MPI_IN_PLACE, node_comm_bindset,
                               num_procs * sizeof(hwloc_cpuset_t), MPI_BYTE,
                               MPI_NO_OP, node_comm, &errflag);


            min_tree_depth = -1;
            for (i = 0; i < num_procs; i++) {
                hwloc_obj_t obj_containing_cpuset =
                    hwloc_get_obj_covering_cpuset(MPIR_Process.hwloc_topology,
                                                  node_comm_bindset[i]);
                if (obj_containing_cpuset->depth < min_tree_depth || min_tree_depth == -1) {
                    min_tree_depth = obj_containing_cpuset->depth;
                }
            }

            if (min_tree_depth) {
                int num_hwloc_objs_at_depth =
                    hwloc_get_nbobjs_by_depth(MPIR_Process.hwloc_topology, min_tree_depth);
                int *processes_cpuset =
                    (int *) MPL_calloc(num_hwloc_objs_at_depth, sizeof(int), MPL_MEM_OTHER);
                hwloc_obj_t parent_obj;
                int hw_obj_index;
                int current_proc_index = -1;

                parent_obj = NULL;
                hw_obj_index = 0;
                while ((parent_obj =
                        hwloc_get_next_obj_by_depth(MPIR_Process.hwloc_topology, min_tree_depth,
                                                    parent_obj)) != NULL) {
                    for (i = 0; i < num_procs; i++) {
                        if (hwloc_bitmap_isincluded(parent_obj->cpuset, node_comm_bindset[i]) ||
                            hwloc_bitmap_isequal(parent_obj->cpuset, node_comm_bindset[i])) {
                            processes_cpuset[hw_obj_index] = 1;
                            if (i == subcomm_rank) {
                                current_proc_index = hw_obj_index;
                            }
                            break;
                        }
                    }
                    hw_obj_index++;
                }

                color =
                    get_color_from_subset_bitmap(current_proc_index, processes_cpuset,
                                                 num_hwloc_objs_at_depth, subcomm_min_size);

                mpi_errno = MPIR_Comm_split_impl(node_comm, color, key, newcomm_ptr);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPL_free(processes_cpuset);
                MPIR_Comm_free_impl(node_comm);
            }
            MPL_free(node_comm_bindset);
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
    int i, color;
    netloc_node_t *network_node;

    /* Get available memory in the node */
    hwloc_obj_t memory_obj = NULL;
    long total_memory_size = 0;
    int memory_per_process;

    while ((memory_obj =
            hwloc_get_next_obj_by_type(MPIR_Process.hwloc_topology, HWLOC_OBJ_NUMANODE,
                                       memory_obj)) != NULL) {
        /* Memory size is in bytes here */
        total_memory_size += memory_obj->total_memory;
    }

    if (min_mem_size == 0 || MPIR_Process.network_attr.type == MPIR_NETLOC_NETWORK_TYPE__INVALID) {
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
    int comm_size = MPIR_Comm_size(comm_ptr);

    /* Dimension is assumed to be indexed from 0 */
    if (MPIR_Process.network_attr.type != MPIR_NETLOC_NETWORK_TYPE__TORUS ||
        dimension >= MPIR_Process.network_attr.u.torus.dimension) {
        *newcomm_ptr = NULL;
    } else {
        int node_coordinates = MPIR_Process.network_attr.u.torus.node_idx;
        int *node_dimensions = MPIR_Process.network_attr.u.torus.geometry;
        color = 0;
        for (i = 0; i < MPIR_Process.network_attr.u.torus.dimension; i++) {
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

#endif

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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    hintval_equal = (hintval_size == hintval_size_max);

    mpi_errno =
        MPID_Allreduce(&hintval_equal, &hintval_equal_global, 1, MPI_INT, MPI_LAND,
                       comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!hintval_equal_global)
        goto fn_exit;


    /* Now that the sizes of the hintvals match, check to make sure
     * the actual hintvals themselves are the equal */
    hintval_global = (char *) MPL_malloc(strlen(hintval), MPL_MEM_OTHER);

    mpi_errno =
        MPID_Allreduce(hintval, hintval_global, strlen(hintval), MPI_CHAR,
                       MPI_MAX, comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    hintval_equal = !memcmp(hintval, hintval_global, strlen(hintval));

    mpi_errno =
        MPID_Allreduce(&hintval_equal, &hintval_equal_global, 1, MPI_INT, MPI_LAND,
                       comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (hintval_global != NULL)
        MPL_free(hintval_global);

    *info_args_are_equal = hintval_equal_global;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_node_topo
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* if all processes do not have the same hintval, skip
     * topology-aware comm split */
    if (!info_args_are_equal)
        goto use_node_comm;

    /* if no info key is given, skip topology-aware comm split */
    if (!info_ptr)
        goto use_node_comm;

#ifdef HAVE_HWLOC
    /* if our bindset is not valid, skip topology-aware comm split */
    if (!MPIR_Process.bindset_is_valid)
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

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPIR_Comm_free_impl(comm_ptr);

        goto fn_exit;
    }
#endif /* HAVE_HWLOC */

  use_node_comm:
    *newcomm_ptr = comm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_network_topo
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_network_topo(MPIR_Comm * comm_ptr, int key, const char *hintval,
                                      MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_NETLOC
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
#endif
  fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key,
                         MPIR_Info * info_ptr, MPIR_Comm ** newcomm_ptr)
{
    MPIR_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;

    /* split out the undefined processes */
    mpi_errno = MPIR_Comm_split_impl(user_comm_ptr, split_type == MPI_UNDEFINED ? MPI_UNDEFINED : 0,
                                     key, &comm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (split_type == MPI_UNDEFINED) {
        *newcomm_ptr = NULL;
        goto fn_exit;
    }

    if (split_type == MPI_COMM_TYPE_SHARED) {
        mpi_errno = MPIR_Comm_split_type_self(comm_ptr, split_type, key, newcomm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else if (split_type == MPIX_COMM_TYPE_NEIGHBORHOOD) {
        mpi_errno =
            MPIR_Comm_split_type_neighborhood(comm_ptr, split_type, key, info_ptr, newcomm_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
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

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_nbhd_common_dir
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_split_type_nbhd_common_dir(MPIR_Comm * user_comm_ptr, int key, const char *hintval,
                                         MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ROMIO
    MPI_Comm dummycomm;
    MPIR_Comm *dummycomm_ptr;

    mpi_errno = MPIR_Comm_split_filesystem(user_comm_ptr->handle, key, hintval, &dummycomm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_Comm_get_ptr(dummycomm, dummycomm_ptr);
    *newcomm_ptr = dummycomm_ptr;
#endif

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_neighborhood
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

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

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_split_type_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_split_type
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (newcomm_ptr)
        MPIR_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_SPLIT_TYPE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        /* FIXME this error code is wrong, it's the error code for
         * regular MPI_Comm_split */
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_split",
                                 "**mpi_comm_split %C %d %d %p", comm, split_type, key, newcomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
