/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"
#include "mpir_hw_topo.h"

#ifdef HAVE_HWLOC
#include "hwloc.h"
#endif

#ifdef HAVE_NETLOC
#include "mpir_netloc.h"
#endif

/*
 * Hardwre topology
 */
static struct {
#ifdef HAVE_HWLOC
    hwloc_topology_t hwloc_topology;
    hwloc_cpuset_t bindset;
#endif
    int bindset_is_valid;
#ifdef HAVE_NETLOC
    netloc_topology_t netloc_topology;
    MPIR_Netloc_network_attributes network_attr;
#endif
} hw_topo = {
0};

#ifdef HAVE_HWLOC
static hwloc_obj_type_t convert_mpir_node_obj_type_to_hw(MPIR_Node_obj_type type)
{
    hwloc_obj_type_t ret;

    switch (type) {
        case MPIR_NODE_OBJ_TYPE__MACHINE:
            ret = HWLOC_OBJ_MACHINE;
            break;
        case MPIR_NODE_OBJ_TYPE__PACKAGE:
            ret = HWLOC_OBJ_PACKAGE;
            break;
        case MPIR_NODE_OBJ_TYPE__CORE:
            ret = HWLOC_OBJ_CORE;
            break;
        case MPIR_NODE_OBJ_TYPE__PU:
            ret = HWLOC_OBJ_PU;
            break;
        case MPIR_NODE_OBJ_TYPE__L1CACHE:
            ret = HWLOC_OBJ_L1CACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__L2CACHE:
            ret = HWLOC_OBJ_L2CACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__L3CACHE:
            ret = HWLOC_OBJ_L3CACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__L4CACHE:
            ret = HWLOC_OBJ_L4CACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__L5CACHE:
            ret = HWLOC_OBJ_L5CACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__L1ICACHE:
            ret = HWLOC_OBJ_L1ICACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__L2ICACHE:
            ret = HWLOC_OBJ_L2ICACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__L3ICACHE:
            ret = HWLOC_OBJ_L3ICACHE;
            break;
        case MPIR_NODE_OBJ_TYPE__GROUP:
            ret = HWLOC_OBJ_GROUP;
            break;
        case MPIR_NODE_OBJ_TYPE__NUMANODE:
            ret = HWLOC_OBJ_NUMANODE;
            break;
        case MPIR_NODE_OBJ_TYPE__BRIDGE:
            ret = HWLOC_OBJ_BRIDGE;
            break;
        case MPIR_NODE_OBJ_TYPE__PCI_DEVICE:
            ret = HWLOC_OBJ_PCI_DEVICE;
            break;
        case MPIR_NODE_OBJ_TYPE__OS_DEVICE:
            ret = HWLOC_OBJ_OS_DEVICE;
            break;
        case MPIR_NODE_OBJ_TYPE__MISC:
            ret = HWLOC_OBJ_MISC;
            break;
        default:
            ret = -1;
    }

    return ret;
}

static MPIR_Node_obj_type convert_hw_node_obj_type_to_mpir(hwloc_obj_type_t type)
{
    MPIR_Node_obj_type ret;

    switch (type) {
        case HWLOC_OBJ_MACHINE:
            ret = MPIR_NODE_OBJ_TYPE__MACHINE;
            break;
        case HWLOC_OBJ_PACKAGE:
            ret = MPIR_NODE_OBJ_TYPE__PACKAGE;
            break;
        case HWLOC_OBJ_CORE:
            ret = MPIR_NODE_OBJ_TYPE__CORE;
            break;
        case HWLOC_OBJ_PU:
            ret = MPIR_NODE_OBJ_TYPE__PU;
            break;
        case HWLOC_OBJ_L1CACHE:
            ret = MPIR_NODE_OBJ_TYPE__L1CACHE;
            break;
        case HWLOC_OBJ_L2CACHE:
            ret = MPIR_NODE_OBJ_TYPE__L2CACHE;
            break;
        case HWLOC_OBJ_L3CACHE:
            ret = MPIR_NODE_OBJ_TYPE__L3CACHE;
            break;
        case HWLOC_OBJ_L4CACHE:
            ret = MPIR_NODE_OBJ_TYPE__L4CACHE;
            break;
        case HWLOC_OBJ_L5CACHE:
            ret = MPIR_NODE_OBJ_TYPE__L5CACHE;
            break;
        case HWLOC_OBJ_L1ICACHE:
            ret = MPIR_NODE_OBJ_TYPE__L1ICACHE;
            break;
        case HWLOC_OBJ_L2ICACHE:
            ret = MPIR_NODE_OBJ_TYPE__L2ICACHE;
            break;
        case HWLOC_OBJ_L3ICACHE:
            ret = MPIR_NODE_OBJ_TYPE__L3ICACHE;
            break;
        case HWLOC_OBJ_GROUP:
            ret = MPIR_NODE_OBJ_TYPE__GROUP;
            break;
        case HWLOC_OBJ_NUMANODE:
            ret = MPIR_NODE_OBJ_TYPE__NUMANODE;
            break;
        case HWLOC_OBJ_BRIDGE:
            ret = MPIR_NODE_OBJ_TYPE__BRIDGE;
            break;
        case HWLOC_OBJ_PCI_DEVICE:
            ret = MPIR_NODE_OBJ_TYPE__PCI_DEVICE;
            break;
        case HWLOC_OBJ_OS_DEVICE:
            ret = MPIR_NODE_OBJ_TYPE__OS_DEVICE;
            break;
        case HWLOC_OBJ_MISC:
            ret = MPIR_NODE_OBJ_TYPE__MISC;
            break;
        default:
            ret = MPIR_NODE_OBJ_TYPE__NONE;
    }

    return ret;
}

static MPIR_Node_obj_osdev_type convert_hw_node_osdev_obj_type_to_mpir(hwloc_obj_osdev_type_t type)
{
    MPIR_Node_obj_osdev_type ret;

    switch (type) {
        case HWLOC_OBJ_OSDEV_BLOCK:
            ret = MPIR_NODE_OBJ_OSDEV_TYPE__BLOCK;
            break;
        case HWLOC_OBJ_OSDEV_GPU:
            ret = MPIR_NODE_OBJ_OSDEV_TYPE__GPU;
            break;
        case HWLOC_OBJ_OSDEV_NETWORK:
            ret = MPIR_NODE_OBJ_OSDEV_TYPE__NETWORK;
            break;
        case HWLOC_OBJ_OSDEV_OPENFABRICS:
            ret = MPIR_NODE_OBJ_OSDEV_TYPE__OPENFABRICS;
            break;
        case HWLOC_OBJ_OSDEV_DMA:
            ret = MPIR_NODE_OBJ_OSDEV_TYPE__DMA;
            break;
        case HWLOC_OBJ_OSDEV_COPROC:
            ret = MPIR_NODE_OBJ_OSDEV_TYPE__COPROC;
            break;
        default:
            ret = MPIR_NODE_OBJ_OSDEV_TYPE__NONE;
    }

    return ret;
}
#endif

#ifdef HAVE_NETLOC
static netloc_node_type_t convert_mpir_net_obj_type_to_hw(MPIR_Network_node_type type)
{
    netloc_node_type_t ret;

    switch (type) {
        case MPIR_NETWORK_NODE_TYPE__HOST:
            ret = NETLOC_NODE_TYPE_HOST;
            break;
        case MPIR_NETWORK_NODE_TYPE__SWITCH:
            ret = NETLOC_NODE_TYPE_SWITCH;
            break;
        case MPIR_NETWORK_NODE_TYPE__INVALID:
            ret = NETLOC_NODE_TYPE_INVALID;
            break;
        default:
            ret = -1;
    }

    return ret;
}

static MPIR_Network_node_type convert hw_net_obj_type_to_mpir(netloc_node_type_t type)
{
    MPIR_Network_node_type ret;

    switch (type) {
        case NETLOC_NODE_TYPE_HOST:
            ret = MPIR_NETWORK_NODE_TYPE__HOST;
            break;
        case NETLOC_NODE_TYPE_SWITCH:
            ret = MPIR_NETWORK_NODE_TYPE__SWITCH;
            break;
        case NETLOC_NODE_TYPE_INVALID:
            ret = MPIR_NETWORK_NODE_TYPE__INVALID;
            break;
        default:
            ret = -1;
    }

    return ret;
}
#endif

int MPII_hw_topo_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

int MPII_hw_topo_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

bool MPIR_hw_topo_is_initialized(void)
{

}

MPIR_Node_obj MPIR_Node_get_covering_obj(void)
{
    MPIR_Node_obj ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj MPIR_Node_get_covering_obj_by_type(MPIR_Node_obj_type obj_type)
{
    MPIR_Node_obj ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj MPIR_Node_get_covering_obj_by_depth(int depth)
{
    MPIR_Node_obj ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj MPIR_Node_get_parent_obj(MPIR_Node_obj obj)
{
    MPIR_Node_obj ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

int MPIR_Node_get_obj_index(MPIR_Node_obj obj)
{
    int ret = -1;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

int MPIR_Node_get_obj_depth(MPIR_Node_obj obj)
{
    int ret = -1;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj_type MPIR_Node_get_obj_type(MPIR_Node_obj obj)
{
    MPIR_Node_obj_type ret = MPIR_NODE_OBJ_TYPE__NONE;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

const char *MPIR_Node_get_obj_name(MPIR_Node_obj obj)
{
    const char *ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

uint64_t MPIR_Node_get_total_mem(void)
{
    uint64_t ret = 0;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

int MPIR_Node_get_obj_type_affinity(MPIR_Node_obj_type obj_type)
{
    int ret = -1;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj MPIR_Node_get_non_io_ancestor_obj(MPIR_Node_obj dev_obj)
{
    MPIR_Node_obj ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj MPIR_Node_get_osdev_obj_by_busidstring(const char *bus_id_string)
{
    MPIR_Node_obj ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj MPIR_Node_get_common_non_io_ancestor_obj(const char *dev_name)
{
    MPIR_Node_obj ret = NULL;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Node_obj_osdev_type MPIR_Node_get_osdev_obj_type(MPIR_Node_obj dev_obj)
{
    MPIR_Node_obj_osdev_type ret = MPIR_NODE_OBJ_OSDEV_TYPE__NONE;

#ifdef HAVE_HWLOC
    ;
#endif

    return ret;
}

MPIR_Network_topology_type MPIR_Net_get_topo_type(void)
{
    MPIR_Network_topology_type ret = MPIR_NETWORK_TOPOLOGY_TYPE__INVALID;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

MPIR_Network_node_type MPIR_Net_get_node_type(MPIR_Network_node node)
{
    MPIR_Network_node_type ret = MPIR_NETWORK_NODE_TYPE__INVALID;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

MPIR_Network_node MPIR_Net_get_endpoint(void)
{
    MPIR_Network_node ret = NULL;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

MPIR_Network_node MPIR_Net_get_edge_dest_node(MPIR_Network_edge edge)
{
    MPIR_Network_node ret = NULL;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

int MPIR_Net_get_node_uid(MPIR_Network_node node)
{
    int ret = 0;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

int MPIR_Net_get_num_nodes(void)
{
    int ret = 0;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

int MPIR_Net_get_all_edges(MPIR_Network_node node, int *num_edges, MPIR_Network_edge ** edges)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_NETLOC
    ;
#endif

    return mpi_errno;
}

int *MPIR_Net_tree_topo_get_node_levels(void)
{
    int *ret = NULL;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

int MPIR_Net_tree_topo_get_hostnode_index(int *node_index, int *num_nodes)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_NETLOC
    ;
#endif

    return mpi_errno;
}

int MPIR_Net_tree_topo_get_switches_at_level(int switch_level,
                                             MPIR_Network_node ** switches_at_level,
                                             int *switch_count)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_NETLOC
    ;
#endif

    return mpi_errno;
}

int MPIR_Net_torus_topo_get_dimension(void)
{
    int ret = 0;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

int *MPIR_Net_torus_topo_get_geometry(void)
{
    int *ret = NULL;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}

int MPIR_Net_torus_topo_get_node_index(void)
{
    int ret = 0;

#ifdef HAVE_NETLOC
    ;
#endif

    return ret;
}
