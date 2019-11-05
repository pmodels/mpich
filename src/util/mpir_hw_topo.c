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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_HW_TOPO_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_HW_TOPO_INIT);

    hw_topo.bindset_is_valid = 0;

#ifdef HAVE_HWLOC
    hw_topo.bindset = hwloc_bitmap_alloc();
    hwloc_topology_init(&hw_topo.hwloc_topology);
    hwloc_topology_set_io_types_filter(hw_topo.hwloc_topology, HWLOC_TYPE_FILTER_KEEP_ALL);
    if (!hwloc_topology_load(hw_topo.hwloc_topology))
        hw_topo.bindset_is_valid =
            !hwloc_get_proc_cpubind(hw_topo.hwloc_topology, getpid(), hw_topo.bindset,
                                    HWLOC_CPUBIND_PROCESS);
#endif

#ifdef HAVE_NETLOC
    hw_topo.network_attr.u.tree.node_levels = NULL;
    hw_topo.network_attr.network_endpoint = NULL;
    hw_topo.netloc_topology = NULL;
    hw_topo.network_attr.type = MPIR_NETWORK_TOPOLOGY_TYPE__INVALID;
    if (strlen(MPIR_CVAR_NETLOC_NODE_FILE)) {
        mpi_errno = netloc_parse_topology(&hw_topo.netloc_topology, MPIR_CVAR_NETLOC_NODE_FILE);
        if (mpi_errno == NETLOC_SUCCESS)
            MPIR_Netloc_parse_topology(hw_topo.hwloc_topology, hw_topo.netloc_topology,
                                       &hw_topo.network_attr);
    }
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_HW_TOPO_INIT);
    return mpi_errno;
}

int MPII_hw_topo_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_HW_TOPO_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_HW_TOPO_FINALIZE);

#ifdef HAVE_HWLOC
    hwloc_topology_destroy(hw_topo.hwloc_topology);
    hwloc_bitmap_free(hw_topo.bindset);
#endif

    hw_topo.bindset_is_valid = 0;

#ifdef HAVE_NETLOC
    switch (hw_topo.network_attr.type) {
        case MPIR_NETWORK_TOPOLOGY_TYPE__TORUS:
            if (hw_topo.network_attr.u.torus.geometry != NULL)
                MPL_free(hw_topo.network_attr.u.torus.geometry);
            break;
        case MPIR_NETWORK_TOPOLOGY_TYPE__FAT_TREE:
        case MPIR_NETWORK_TOPOLOGY_TYPE__CLOS_NETWORK:
        default:
            if (hw_topo.network_attr.u.tree.node_levels != NULL)
                MPL_free(hw_topo.network_attr.u.tree.node_levels);
            break;
    }
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_HW_TOPO_FINALIZE);
    return mpi_errno;
}

bool MPIR_hw_topo_is_initialized(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_HW_TOPO_IS_INITIALIZED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_HW_TOPO_IS_INITIALIZED);

    ret = (hw_topo.bindset_is_valid) ? true : false;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_HW_TOPO_IS_INITIALIZED);
    return ret;
}

MPIR_Node_obj MPIR_Node_get_covering_obj(void)
{
    MPIR_Node_obj ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ);

    if (!hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    ret = hwloc_get_obj_covering_cpuset(hw_topo.hwloc_topology, hw_topo.bindset);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ);
    return ret;
}

MPIR_Node_obj MPIR_Node_get_covering_obj_by_type(MPIR_Node_obj_type obj_type)
{
    MPIR_Node_obj ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ_BY_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ_BY_TYPE);

    if (!hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    hwloc_obj_type_t hw_obj_type = convert_mpir_node_obj_type_to_hw(obj_type);

    hwloc_obj_t tmp = NULL;
    hwloc_obj_t covering_obj =
        hwloc_get_obj_covering_cpuset(hw_topo.hwloc_topology, hw_topo.bindset);
    if (!covering_obj)
        goto fn_exit;

    while ((tmp = hwloc_get_next_obj_by_type(hw_topo.hwloc_topology, hw_obj_type, tmp)) != NULL) {
        if (hwloc_bitmap_isincluded(covering_obj->cpuset, tmp->cpuset) ||
            hwloc_bitmap_isequal(tmp->cpuset, covering_obj->cpuset)) {
            ret = tmp;
            break;
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ_BY_TYPE);
    return ret;
}

MPIR_Node_obj MPIR_Node_get_covering_obj_by_depth(int depth)
{
    MPIR_Node_obj ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ_BY_DEPTH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ_BY_DEPTH);

    if (!hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    hwloc_obj_t tmp = NULL;
    hwloc_obj_t covering_obj =
        hwloc_get_obj_covering_cpuset(hw_topo.hwloc_topology, hw_topo.bindset);
    if (!covering_obj)
        goto fn_exit;

    while ((tmp = hwloc_get_next_obj_by_depth(hw_topo.hwloc_topology, depth, tmp)) != NULL) {
        if (hwloc_bitmap_isincluded(covering_obj->cpuset, tmp->cpuset) ||
            hwloc_bitmap_isequal(tmp->cpuset, covering_obj->cpuset)) {
            ret = tmp;
            break;
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_COVERING_OBJ_BY_DEPTH);
    return ret;
}

MPIR_Node_obj MPIR_Node_get_parent_obj(MPIR_Node_obj obj)
{
    MPIR_Node_obj ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_PARENT_OBJ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_PARENT_OBJ);

    if (obj == NULL)
        goto fn_exit;

#ifdef HAVE_HWLOC
    ret = ((hwloc_obj_t) obj)->parent;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_PARENT_OBJ);
    return ret;
}

int MPIR_Node_get_obj_index(MPIR_Node_obj obj)
{
    int ret = -1;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_OBJ_INDEX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_OBJ_INDEX);

    if (obj == NULL)
        goto fn_exit;

#ifdef HAVE_HWLOC
    ret = ((hwloc_obj_t) obj)->logical_index;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_OBJ_INDEX);
    return ret;
}

int MPIR_Node_get_obj_depth(MPIR_Node_obj obj)
{
    int ret = -1;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_OBJ_DEPTH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_OBJ_DEPTH);

    if (obj == NULL)
        goto fn_exit;

#ifdef HAVE_HWLOC
    ret = ((hwloc_obj_t) obj)->depth;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_OBJ_DEPTH);
    return ret;
}

MPIR_Node_obj_type MPIR_Node_get_obj_type(MPIR_Node_obj obj)
{
    MPIR_Node_obj_type ret = MPIR_NODE_OBJ_TYPE__NONE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_OBJ_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_OBJ_TYPE);

    if (obj == NULL)
        goto fn_exit;

#ifdef HAVE_HWLOC
    hwloc_obj_t tmp = (hwloc_obj_t) obj;
    ret = convert_hw_node_obj_type_to_mpir(tmp->type);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_OBJ_TYPE);
    return ret;
}

const char *MPIR_Node_get_obj_name(MPIR_Node_obj obj)
{
    const char *ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_OBJ_NAME);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_OBJ_NAME);

    if (obj == NULL)
        goto fn_exit;

#ifdef HAVE_HWLOC
    ret = ((hwloc_obj_t) obj)->name;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_OBJ_NAME);
    return ret;
}

uint64_t MPIR_Node_get_total_mem(void)
{
    uint64_t ret = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_TOTAL_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_TOTAL_MEM);

    if (!hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    hwloc_obj_t tmp = NULL;
    while ((tmp = hwloc_get_next_obj_by_type(hw_topo.hwloc_topology, HWLOC_OBJ_NUMANODE, tmp)))
        ret += tmp->total_memory;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_TOTAL_MEM);
    return ret;
}

int MPIR_Node_get_obj_type_affinity(MPIR_Node_obj_type obj_type)
{
    int ret = -1;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_OBJ_TYPE_AFFINITY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_OBJ_TYPE_AFFINITY);

    if (!hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    hwloc_obj_type_t hw_obj_type = convert_mpir_node_obj_type_to_hw(obj_type);
    int nobjs = hwloc_get_nbobjs_by_type(hw_topo.hwloc_topology, hw_obj_type);

    if (nobjs > 0) {
        int count = 0;
        hwloc_obj_t tmp = NULL;

        for (int i = 0; i < nobjs && count < 2; i++) {
            tmp = hwloc_get_obj_by_type(hw_topo.hwloc_topology, hw_obj_type, i);
            if (hwloc_bitmap_intersects(tmp->cpuset, hw_topo.bindset)) {
                ret = tmp->logical_index;
                count++;
            }
        }

        if (count > 1)
            ret = -1;
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_OBJ_TYPE_AFFINITY);
    return ret;
}

MPIR_Node_obj MPIR_Node_get_non_io_ancestor_obj(MPIR_Node_obj dev_obj)
{
    MPIR_Node_obj ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_NON_IO_ANCESTOR_OBJ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_NON_IO_ANCESTOR_OBJ);

    if (dev_obj == NULL || !hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    ret = hwloc_get_non_io_ancestor_obj(hw_topo.hwloc_topology, (hwloc_obj_t) dev_obj);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_NON_IO_ANCESTOR_OBJ);
    return ret;
}

MPIR_Node_obj MPIR_Node_get_osdev_obj_by_busidstring(const char *bus_id_string)
{
    MPIR_Node_obj ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_OSDEV_OBJ_BY_BUSIDSTRING);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_OSDEV_OBJ_BY_BUSIDSTRING);

    if (bus_id_string == NULL || !hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    ret = hwloc_get_pcidev_by_busidstring(hw_topo.hwloc_topology, bus_id_string);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_OSDEV_OBJ_BY_BUSIDSTRING);
    return ret;
}

#ifdef HAVE_HWLOC
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
#endif

MPIR_Node_obj MPIR_Node_get_common_non_io_ancestor_obj(const char *dev_name)
{
    MPIR_Node_obj ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_COMMON_NON_IO_ANCESTOR_OBJ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_COMMON_NON_IO_ANCESTOR_OBJ);

    if (dev_name == NULL || !hw_topo.bindset_is_valid)
        goto fn_exit;

#ifdef HAVE_HWLOC
    hwloc_obj_t obj_containing_cpuset =
        hwloc_get_obj_covering_cpuset(hw_topo.hwloc_topology, hw_topo.bindset);
    if (!obj_containing_cpuset)
        goto fn_exit;

    hwloc_obj_t io_device = NULL;
    hwloc_obj_t non_io_ancestor = NULL;

    while ((io_device = hwloc_get_next_osdev(hw_topo.hwloc_topology, io_device))) {
        if (!io_device_found(dev_name, "hfi", io_device, HWLOC_OBJ_OSDEV_OPENFABRICS))
            continue;
        if (!io_device_found(dev_name, "ib", io_device, HWLOC_OBJ_OSDEV_NETWORK))
            continue;
        if (!io_device_found(dev_name, "eth", io_device, HWLOC_OBJ_OSDEV_NETWORK) &&
            !io_device_found(dev_name, "en", io_device, HWLOC_OBJ_OSDEV_NETWORK))
            continue;

        if (!strncmp(dev_name, "gpu", strlen("gpu"))) {
            if (io_device->attr->osdev.type == HWLOC_OBJ_OSDEV_GPU) {
                if ((*(dev_name + strlen("gpu")) != '\0') &&
                    atoi(dev_name + strlen("gpu")) != io_device->logical_index) {
                    continue;
                }
            } else {
                continue;
            }
        }

        non_io_ancestor = hwloc_get_non_io_ancestor_obj(hw_topo.hwloc_topology, io_device);
        while (!hwloc_obj_type_is_normal(non_io_ancestor->type))
            non_io_ancestor = non_io_ancestor->parent;
        MPIR_Assert(non_io_ancestor && non_io_ancestor->depth >= 0);

        if (!hwloc_obj_is_in_subtree
            (hw_topo.hwloc_topology, obj_containing_cpuset, non_io_ancestor))
            continue;

        break;
    }

    ret = non_io_ancestor;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_COMMON_NON_IO_ANCESTOR_OBJ);
    return ret;
}

MPIR_Node_obj_osdev_type MPIR_Node_get_osdev_obj_type(MPIR_Node_obj dev_obj)
{
    MPIR_Node_obj_osdev_type ret = MPIR_NODE_OBJ_OSDEV_TYPE__NONE;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NODE_GET_OSDEV_OBJ_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NODE_GET_OSDEV_OBJ_TYPE);

    if (dev_obj == NULL)
        goto fn_exit;

#ifdef HAVE_HWLOC
    hwloc_obj_t tmp = (hwloc_obj_t) dev_obj;
    ret = convert_hw_node_osdev_obj_type_to_mpir(tmp->attr->osdev.type);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NODE_GET_OSDEV_OBJ_TYPE);
    return ret;
}

MPIR_Network_topology_type MPIR_Net_get_topo_type(void)
{
    MPIR_Network_topology_type ret = MPIR_NETWORK_TOPOLOGY_TYPE__INVALID;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_GET_TOPO_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_GET_TOPO_TYPE);

#ifdef HAVE_NETLOC
    ret = hw_topo.network_attr.type;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_GET_TOPO_TYPE);
    return ret;
}

MPIR_Network_node_type MPIR_Net_get_node_type(MPIR_Network_node node)
{
    MPIR_Network_node_type ret = MPIR_NETWORK_NODE_TYPE__INVALID;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_GET_NODE_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_GET_NODE_TYPE);

    if (node == NULL)
        goto fn_exit;

#ifdef HAVE_NETLOC
    ret = convert_hw_net_obj_type_to_mpir(((netloc_node_t *) node)->node_type);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_GET_NODE_TYPE);
    return ret;
}

MPIR_Network_node MPIR_Net_get_endpoint(void)
{
    MPIR_Network_node ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_GET_ENDPOINT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_GET_ENDPOINT);

#ifdef HAVE_NETLOC
    ret = hw_topo.network_attr.network_endpoint;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_GET_ENDPOINT);
    return ret;
}

MPIR_Network_node MPIR_Net_get_edge_dest_node(MPIR_Network_edge edge)
{
    MPIR_Network_node ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_GET_EDGE_DEST_NODE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_GET_EDGE_DEST_NODE);

    if (edge == NULL)
        goto fn_exit;

#ifdef HAVE_NETLOC
    ret = ((netloc_edge_t *) edge)->dest_node;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_GET_EDGE_DEST_NODE);
    return ret;
}

int MPIR_Net_get_node_uid(MPIR_Network_node node)
{
    int ret = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_GET_NODE_UID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_GET_NODE_UID);

    if (node == NULL)
        goto fn_exit;

#ifdef HAVE_NETLOC
    ret = ((netloc_node_t *) node)->__uid__;
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_GET_NODE_UID);
    return ret;
}

int MPIR_Net_get_num_nodes(void)
{
    int ret = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_GET_NUM_NODES);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_GET_NUM_NODES);

#ifdef HAVE_NETLOC
    ret = hw_topo.netloc_topology->num_nodes;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_GET_NUM_NODES);
    return ret;
}

int MPIR_Net_get_all_edges(MPIR_Network_node node, int *num_edges, MPIR_Network_edge ** edges)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_GET_ALL_EDGES);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_GET_ALL_EDGES);

    *num_edges = 0;
    *edges = NULL;

    if (node == NULL)
        goto fn_exit;

#ifdef HAVE_NETLOC
    mpi_errno =
        netloc_get_all_edges(hw_topo.netloc_topology, node, num_edges, (netloc_edge_t ***) edges);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_GET_ALL_EDGES);
    return mpi_errno;
}

int *MPIR_Net_tree_topo_get_node_levels(void)
{
    int *ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_TREE_TOPO_GET_NODE_LEVELS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_TREE_TOPO_GET_NODE_LEVELS);

#ifdef HAVE_NETLOC
    ret = hw_topo.network_attr.u.tree.node_levels;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_TREE_TOPO_GET_NODE_LEVELS);
    return ret;
}

int MPIR_Net_tree_topo_get_hostnode_index(int *node_index, int *num_nodes)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_TREE_TOPO_GET_HOSTNODE_INDEX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_TREE_TOPO_GET_HOSTNODE_INDEX);

    *node_index = 0;
    *num_nodes = 0;

#ifdef HAVE_NETLOC
    mpi_errno =
        MPIR_Netloc_get_hostnode_index_in_tree(hw_topo.network_attr, hw_topo.netloc_topology,
                                               hw_topo.network_attr.network_endpoint, node_index,
                                               num_nodes);
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_TREE_TOPO_GET_HOSTNODE_INDEX);
    return mpi_errno;
}

int MPIR_Net_tree_topo_get_switches_at_level(int switch_level,
                                             MPIR_Network_node ** switches_at_level,
                                             int *switch_count)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_TREE_TOPO_GET_SWITCHES_AT_LEVEL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_TREE_TOPO_GET_SWITCHES_AT_LEVEL);

    *switches_at_level = NULL;
    *switch_count = 0;

#ifdef HAVE_NETLOC
    mpi_errno =
        MPIR_Netloc_get_switches_at_level(hw_topo.netloc_topology, hw_topo.network_attr,
                                          switch_level, (netloc_node_t ***) switches_at_level,
                                          switch_count);
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_TREE_TOPO_GET_SWITCHES_AT_LEVEL);
    return mpi_errno;
}

int MPIR_Net_torus_topo_get_dimension(void)
{
    int ret = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_DIMENSION);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_DIMENSION);

#ifdef HAVE_NETLOC
    ret = hw_topo.network_attr.u.torus.dimension;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_DIMENSION);
    return ret;
}

int *MPIR_Net_torus_topo_get_geometry(void)
{
    int *ret = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_GEOMETRY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_GEOMETRY);

#ifdef HAVE_NETLOC
    ret = hw_topo.network_attr.u.torus.geometry;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_GEOMETRY);
    return ret;
}

int MPIR_Net_torus_topo_get_node_index(void)
{
    int ret = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_NODE_INDEX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_NODE_INDEX);

#ifdef HAVE_NETLOC
    ret = hw_topo.network_attr.u.torus.node_index;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_NET_TORUS_TOPO_GET_NODE_INDEX);
    return ret;
}
