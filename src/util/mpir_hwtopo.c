/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include <strings.h>    /* for strcasecmp */

#ifdef HAVE_HWLOC
#include "hwloc.h"
#endif

/*********************************************************************************************
 * hwtopo unique global id
 *********************************************************************************************
 | 1 sign bit | unused (13) | class (2 bits) | tree depth (6 bits) | logical index (10 bits) |
 |            |             | obj class      | [-63 -- 63]         | [0 -- 1023]             |
 *********************************************************************************************/
#define HWTOPO_GID_BITS        (31)
#define HWTOPO_GID_CLASS_BITS  (2)
#define HWTOPO_GID_DEPTH_BITS  (6)
#define HWTOPO_GID_INDEX_BITS  (10)
#define HWTOPO_GID_CLASS_SHIFT (HWTOPO_GID_DEPTH_BITS + HWTOPO_GID_INDEX_BITS)
#define HWTOPO_GID_DEPTH_SHIFT (HWTOPO_GID_INDEX_BITS)
#define HWTOPO_GID_INDEX_SHIFT (0)
#define HWTOPO_GID_CLASS_MASK  (0x00030000)
#define HWTOPO_GID_DEPTH_MASK  (0x0000FC00)
#define HWTOPO_GID_INDEX_MASK  (0x000003FF)
#define HWTOPO_GID_CLASS_MAX   ((1 << HWTOPO_GID_CLASS_BITS) - 1)
#define HWTOPO_GID_DEPTH_MAX   ((1 << HWTOPO_GID_DEPTH_BITS) - 1)
#define HWTOPO_GID_INDEX_MAX   ((1 << HWTOPO_GID_INDEX_BITS) - 1)

/* hwloc has 4 classes of objects: Memory, I/O, Misc and Normal. Numa nodes
 * (i.e., HWLOC_OBJ_NUMANODE) are a memory class object and are attached as
 * child to the object in the topology tree grouping cores (e.g., package).
 * This means that Numa nodes are not part of the main topology tree (a walk
 * from a leaf to the root will not traverse any Numa node) and thus, like
 * other non-Normal objects, have a special negative depth value (-3 for Numa
 * nodes). For this reason, as we need to be able to perform different actions
 * for different classes of objects, we also encode the object class in the
 * gid. */
typedef enum {
    HWTOPO_CLASS__INVALID = -1,
    HWTOPO_CLASS__MEMORY,
    HWTOPO_CLASS__IO,
    HWTOPO_CLASS__MISC,
    HWTOPO_CLASS__NORMAL
} hwtopo_class_e;

/* When calculating the gid for non-Normal objects we remove the sign of
 * the depth field. This is done as valid gids are positive and the depth
 * is not directly available outside the topology layer anyway (it has to
 * be queried using get_depth). In this case, when we calculate the depth
 * from the gid we do the opposite operation, restoring the sign. */
static MPIR_hwtopo_gid_t HWTOPO_GET_GID(hwtopo_class_e class, int depth, int idx)
{
    MPIR_Assert(class != HWTOPO_CLASS__INVALID);
    MPIR_hwtopo_gid_t gid;
    int depth_ = (class != HWTOPO_CLASS__NORMAL) ? -depth : depth;
    MPIR_Assert(depth <= HWTOPO_GID_DEPTH_MAX);
    MPIR_Assert(idx <= HWTOPO_GID_INDEX_MAX);
    gid = (class << HWTOPO_GID_CLASS_SHIFT);
    gid |= (depth_ << HWTOPO_GID_DEPTH_SHIFT);
    gid |= (idx << HWTOPO_GID_INDEX_SHIFT);
    return gid;
}

static hwtopo_class_e HWTOPO_GET_CLASS(MPIR_hwtopo_gid_t gid)
{
    int class;
    class = (gid & HWTOPO_GID_CLASS_MASK);
    class = (class >> HWTOPO_GID_CLASS_SHIFT);
    return (hwtopo_class_e) class;
}

static int HWTOPO_GET_DEPTH(MPIR_hwtopo_gid_t gid)
{
    int depth;
    depth = (gid & HWTOPO_GID_DEPTH_MASK);
    depth = (depth >> HWTOPO_GID_DEPTH_SHIFT);
    if (HWTOPO_GET_CLASS(gid) != HWTOPO_CLASS__NORMAL) {
        depth = -depth;
    }
    return depth;
}

static int HWTOPO_GET_INDEX(MPIR_hwtopo_gid_t gid)
{
    int idx;
    idx = (gid & HWTOPO_GID_INDEX_MASK);
    idx = (idx >> HWTOPO_GID_INDEX_SHIFT);
    return idx;
}

#ifdef HAVE_HWLOC
static hwloc_obj_type_t get_hwloc_obj_type(MPIR_hwtopo_type_e type)
{
    hwloc_obj_type_t hwloc_obj_type;

    switch (type) {
        case MPIR_HWTOPO_TYPE__NODE:
            hwloc_obj_type = HWLOC_OBJ_MACHINE;
            break;
        case MPIR_HWTOPO_TYPE__PACKAGE:
        case MPIR_HWTOPO_TYPE__SOCKET:
        case MPIR_HWTOPO_TYPE__CPU:
            hwloc_obj_type = HWLOC_OBJ_PACKAGE;
            break;
        case MPIR_HWTOPO_TYPE__CORE:
            hwloc_obj_type = HWLOC_OBJ_CORE;
            break;
        case MPIR_HWTOPO_TYPE__HWTHREAD:
            hwloc_obj_type = HWLOC_OBJ_PU;
            break;
        case MPIR_HWTOPO_TYPE__L1CACHE:
            hwloc_obj_type = HWLOC_OBJ_L1CACHE;
            break;
        case MPIR_HWTOPO_TYPE__L2CACHE:
            hwloc_obj_type = HWLOC_OBJ_L2CACHE;
            break;
        case MPIR_HWTOPO_TYPE__L3CACHE:
            hwloc_obj_type = HWLOC_OBJ_L3CACHE;
            break;
        case MPIR_HWTOPO_TYPE__L4CACHE:
            hwloc_obj_type = HWLOC_OBJ_L4CACHE;
            break;
        case MPIR_HWTOPO_TYPE__L5CACHE:
            hwloc_obj_type = HWLOC_OBJ_L5CACHE;
            break;
        case MPIR_HWTOPO_TYPE__DDR:
        case MPIR_HWTOPO_TYPE__HBM:
            hwloc_obj_type = HWLOC_OBJ_NUMANODE;
            break;
        case MPIR_HWTOPO_TYPE__PCI_DEVICE:
            hwloc_obj_type = HWLOC_OBJ_PCI_DEVICE;
            break;
        default:
            hwloc_obj_type = (hwloc_obj_type_t) (-1);
    }

    return hwloc_obj_type;
}

/* Get object type class in hwloc: Memory, I/O, Misc or Normal */
static hwtopo_class_e get_type_class(hwloc_obj_type_t type)
{
    hwtopo_class_e class;

    switch (type) {
        case HWLOC_OBJ_NUMANODE:
            class = HWTOPO_CLASS__MEMORY;
            break;
        case HWLOC_OBJ_BRIDGE:
        case HWLOC_OBJ_PCI_DEVICE:
        case HWLOC_OBJ_OS_DEVICE:
            class = HWTOPO_CLASS__IO;
            break;
        case HWLOC_OBJ_MISC:
            class = HWTOPO_CLASS__MISC;
            break;
        case HWLOC_OBJ_MACHINE:
        case HWLOC_OBJ_PACKAGE:
        case HWLOC_OBJ_CORE:
        case HWLOC_OBJ_PU:
        case HWLOC_OBJ_L1CACHE:
        case HWLOC_OBJ_L2CACHE:
        case HWLOC_OBJ_L3CACHE:
        case HWLOC_OBJ_L4CACHE:
        case HWLOC_OBJ_L5CACHE:
        case HWLOC_OBJ_L1ICACHE:
        case HWLOC_OBJ_L2ICACHE:
        case HWLOC_OBJ_L3ICACHE:
        case HWLOC_OBJ_GROUP:
            class = HWTOPO_CLASS__NORMAL;
            break;
        default:
            class = HWTOPO_CLASS__INVALID;
    }

    return class;
}
#endif

/*
 * Hardware topology
 */
#ifdef HAVE_HWLOC
static hwloc_topology_t hwloc_topology;
static hwloc_cpuset_t bindset;
#endif
static int bindset_is_valid;


int MPII_hwtopo_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    bindset_is_valid = 0;

#ifdef HAVE_HWLOC
    bindset = hwloc_bitmap_alloc();
    hwloc_topology_init(&hwloc_topology);
    char *xmlfile = MPIR_pmi_get_hwloc_xmlfile();
    if (xmlfile != NULL) {
        int rc;
        rc = hwloc_topology_set_xml(hwloc_topology, xmlfile);
        if (rc == 0) {
            /* To have hwloc still actually call OS-specific hooks, the
             * HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM has to be set to assert that the loaded
             * file is really the underlying system. */
            hwloc_topology_set_flags(hwloc_topology, HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM);
        }
    }

    hwloc_topology_set_io_types_filter(hwloc_topology, HWLOC_TYPE_FILTER_KEEP_ALL);
    if (!hwloc_topology_load(hwloc_topology))
        bindset_is_valid =
            !hwloc_get_proc_cpubind(hwloc_topology, getpid(), bindset, HWLOC_CPUBIND_PROCESS);
#endif

    return mpi_errno;
}

int MPII_hwtopo_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_HWLOC
    hwloc_topology_destroy(hwloc_topology);
    hwloc_bitmap_free(bindset);
#endif

    bindset_is_valid = 0;

    return mpi_errno;
}

bool MPIR_hwtopo_is_initialized(void)
{
    return (bindset_is_valid == 1);
}


MPIR_hwtopo_gid_t MPIR_hwtopo_get_leaf(void)
{
    MPIR_hwtopo_gid_t gid = MPIR_HWTOPO_GID_ROOT;

    if (!bindset_is_valid)
        return gid;

#ifdef HAVE_HWLOC
    hwloc_obj_t leaf = hwloc_get_obj_covering_cpuset(hwloc_topology, bindset);
    hwtopo_class_e class = get_type_class(leaf->type);
    gid = HWTOPO_GET_GID(class, leaf->depth, leaf->logical_index);
#endif

    return gid;
}

int MPIR_hwtopo_get_lid(MPIR_hwtopo_gid_t gid)
{
    return HWTOPO_GET_INDEX(gid);
}

int MPIR_hwtopo_get_depth(MPIR_hwtopo_gid_t gid)
{
    int hwloc_obj_depth = 0;

#ifdef HAVE_HWLOC
    /* gid sanity check */
    int hwloc_obj_index = HWTOPO_GET_INDEX(gid);
    hwloc_obj_depth = HWTOPO_GET_DEPTH(gid);

    hwloc_obj_t hwloc_obj =
        hwloc_get_obj_by_depth(hwloc_topology, hwloc_obj_depth, hwloc_obj_index);
    if (!hwloc_obj) {
        return 0;
    }
#endif

    return hwloc_obj_depth;
}

MPIR_hwtopo_gid_t MPIR_hwtopo_get_ancestor(MPIR_hwtopo_gid_t gid, int depth)
{
    MPIR_hwtopo_gid_t ancestor_gid = MPIR_HWTOPO_GID_ROOT;

#ifdef HAVE_HWLOC
    int hwloc_obj_index = HWTOPO_GET_INDEX(gid);
    int hwloc_obj_depth = HWTOPO_GET_DEPTH(gid);

    hwloc_obj_t obj = hwloc_get_obj_by_depth(hwloc_topology, hwloc_obj_depth, hwloc_obj_index);
    if (obj == NULL)
        return ancestor_gid;

    while (obj && obj->parent && obj->depth != depth)
        obj = obj->parent;

    hwtopo_class_e class = get_type_class(obj->type);
    ancestor_gid = HWTOPO_GET_GID(class, obj->depth, obj->logical_index);
#endif

    return ancestor_gid;
}


MPIR_hwtopo_type_e MPIR_hwtopo_get_type_id(const char *name)
{
    MPIR_hwtopo_type_e query_type = MPIR_HWTOPO_TYPE__MAX;

    struct node_info_table {
        const char *val;
        MPIR_hwtopo_type_e type;
    };

    /* hwtopo node object table */
    struct node_info_table node_info[] = {
        {"node", MPIR_HWTOPO_TYPE__NODE},
        {"machine", MPIR_HWTOPO_TYPE__NODE},
        {"socket", MPIR_HWTOPO_TYPE__SOCKET},
        {"package", MPIR_HWTOPO_TYPE__PACKAGE},
        {"cpu", MPIR_HWTOPO_TYPE__CPU},
        {"core", MPIR_HWTOPO_TYPE__CORE},
        {"hwthread", MPIR_HWTOPO_TYPE__HWTHREAD},
        {"pu", MPIR_HWTOPO_TYPE__HWTHREAD},
        {"l1dcache", MPIR_HWTOPO_TYPE__L1CACHE},
        {"l1ucache", MPIR_HWTOPO_TYPE__L1CACHE},
        {"l1cache", MPIR_HWTOPO_TYPE__L1CACHE},
        {"l2dcache", MPIR_HWTOPO_TYPE__L2CACHE},
        {"l2ucache", MPIR_HWTOPO_TYPE__L2CACHE},
        {"l2cache", MPIR_HWTOPO_TYPE__L2CACHE},
        {"l3dcache", MPIR_HWTOPO_TYPE__L3CACHE},
        {"l3ucache", MPIR_HWTOPO_TYPE__L3CACHE},
        {"l3cache", MPIR_HWTOPO_TYPE__L3CACHE},
        {"l4dcache", MPIR_HWTOPO_TYPE__L4CACHE},
        {"l4ucache", MPIR_HWTOPO_TYPE__L4CACHE},
        {"l4cache", MPIR_HWTOPO_TYPE__L4CACHE},
        {"l5dcache", MPIR_HWTOPO_TYPE__L5CACHE},
        {"l5ucache", MPIR_HWTOPO_TYPE__L5CACHE},
        {"l5cache", MPIR_HWTOPO_TYPE__L5CACHE},
        {"numanode", MPIR_HWTOPO_TYPE__DDR},
        {"numa", MPIR_HWTOPO_TYPE__DDR},
        {"ddr", MPIR_HWTOPO_TYPE__DDR},
        {"hbm", MPIR_HWTOPO_TYPE__HBM},
        {NULL, MPIR_HWTOPO_TYPE__MAX}
    };

    for (int i = 0; node_info[i].val; i++) {
        if (!strcasecmp(node_info[i].val, name)) {
            query_type = node_info[i].type;
            break;
        }
    }

    return query_type;
}

MPIR_hwtopo_gid_t MPIR_hwtopo_get_obj_by_type(MPIR_hwtopo_type_e type)
{
    MPIR_hwtopo_gid_t gid = MPIR_HWTOPO_GID_ROOT;

    if (!bindset_is_valid || type <= MPIR_HWTOPO_TYPE__NONE || type >= MPIR_HWTOPO_TYPE__MAX)
        return gid;

#ifdef HAVE_HWLOC
    hwloc_obj_type_t hw_obj_type = get_hwloc_obj_type(type);

    hwloc_obj_t tmp = NULL;
    while ((tmp = hwloc_get_next_obj_by_type(hwloc_topology, hw_obj_type, tmp)) != NULL) {
        if (hwloc_bitmap_isincluded(bindset, tmp->cpuset) ||
            hwloc_bitmap_isequal(bindset, tmp->cpuset)) {
            hwtopo_class_e class = get_type_class(tmp->type);
            if (type == MPIR_HWTOPO_TYPE__DDR) {
                if (!tmp->subtype)
                    gid = HWTOPO_GET_GID(class, tmp->depth, tmp->logical_index);
                else
                    continue;
            } else if (type == MPIR_HWTOPO_TYPE__HBM) {
                if (tmp->subtype)
                    gid = HWTOPO_GET_GID(class, tmp->depth, tmp->logical_index);
                else
                    continue;
            } else {
                gid = HWTOPO_GET_GID(class, tmp->depth, tmp->logical_index);
            }
            break;
        }
    }
#endif

    return gid;
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

MPIR_hwtopo_gid_t MPIR_hwtopo_get_obj_by_name(const char *name)
{
    MPIR_hwtopo_gid_t gid = MPIR_HWTOPO_GID_ROOT;

    if (!name || !bindset_is_valid)
        return gid;

#ifdef HAVE_HWLOC
    hwloc_obj_t io_device = NULL;
    hwloc_obj_t non_io_ancestor = NULL;

    if (!strncmp(name, "pci:", strlen("pci:"))) {
        io_device = hwloc_get_pcidev_by_busidstring(hwloc_topology, name + strlen("pci:"));
        if (io_device) {
            non_io_ancestor = hwloc_get_non_io_ancestor_obj(hwloc_topology, io_device);
            hwtopo_class_e class = get_type_class(non_io_ancestor->type);
            gid = HWTOPO_GET_GID(class, non_io_ancestor->depth, non_io_ancestor->logical_index);
        }
    } else if (!strncmp(name, "hfi", strlen("hfi")) || !strncmp(name, "ib", strlen("ib")) ||
               !strncmp(name, "eth", strlen("eth")) || !strncmp(name, "en", strlen("en")) ||
               !strncmp(name, "gpu", strlen("gpu"))) {

        hwloc_obj_t obj_containing_cpuset = hwloc_get_obj_covering_cpuset(hwloc_topology, bindset);

        while ((io_device = hwloc_get_next_osdev(hwloc_topology, io_device))) {
            if (!io_device_found(name, "hfi", io_device, HWLOC_OBJ_OSDEV_OPENFABRICS))
                continue;
            if (!io_device_found(name, "ib", io_device, HWLOC_OBJ_OSDEV_NETWORK))
                continue;
            if (!io_device_found(name, "eth", io_device, HWLOC_OBJ_OSDEV_NETWORK) &&
                !io_device_found(name, "en", io_device, HWLOC_OBJ_OSDEV_NETWORK))
                continue;

            if (!strncmp(name, "gpu", strlen("gpu"))) {
                if (io_device->attr->osdev.type == HWLOC_OBJ_OSDEV_GPU) {
                    if ((*(name + strlen("gpu")) != '\0') &&
                        atoi(name + strlen("gpu")) != io_device->logical_index) {
                        continue;
                    }
                } else {
                    continue;
                }
            }

            non_io_ancestor = hwloc_get_non_io_ancestor_obj(hwloc_topology, io_device);
            while (!hwloc_obj_type_is_normal(non_io_ancestor->type))
                non_io_ancestor = non_io_ancestor->parent;
            MPIR_Assert(non_io_ancestor && non_io_ancestor->depth >= 0);

            if (!hwloc_obj_is_in_subtree(hwloc_topology, obj_containing_cpuset, non_io_ancestor))
                continue;

            break;
        }

        if (non_io_ancestor) {
            hwtopo_class_e class = get_type_class(non_io_ancestor->type);
            gid = HWTOPO_GET_GID(class, non_io_ancestor->depth, non_io_ancestor->logical_index);
        }
    } else if (!strcmp(name, "bindset")) {
        char buf[100];          /* covers up to 800 PUs */
        memset(buf, 0, 100);
        int num_pus = hwloc_get_nbobjs_by_type(hwloc_topology, HWLOC_OBJ_PU);
        int n = MPL_MIN(8 * 100, num_pus);
        int j = 0;
        int k = 0;
        for (int i = 0; i < n; i++) {
            if (hwloc_bitmap_isset(bindset, i)) {
                buf[j] |= (1 << k);
            }
            k++;
            if (k >= 8) {
                j++;
                k = 0;
            }
        }
        HASH_VALUE(buf, j, gid);
    } else
#endif
    {
        MPIR_hwtopo_type_e type = MPIR_hwtopo_get_type_id(name);
        if (type != MPIR_HWTOPO_TYPE__MAX)
            gid = MPIR_hwtopo_get_obj_by_type(type);
    }

    return gid;
}

int MPIR_hwtopo_mem_bind(void *baseaddr, size_t len, MPIR_hwtopo_gid_t gid)
{
    int ret = MPI_SUCCESS;

#ifdef HAVE_HWLOC
    const struct hwloc_topology_support *support = hwloc_topology_get_support(hwloc_topology);
    if (!support->membind->set_area_membind) {
#ifdef HAVE_ERROR_CHECKING
        ret =
            MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**nomembind", 0);
#endif /* HAVE_ERROR_CHECKING */
        return ret;
    }

    hwloc_membind_policy_t policy = HWLOC_MEMBIND_BIND;
    hwloc_membind_flags_t flags = HWLOC_MEMBIND_STRICT;

    int hwloc_obj_index = HWTOPO_GET_INDEX(gid);
    int hwloc_obj_depth = HWTOPO_GET_DEPTH(gid);

    hwloc_obj_t hwloc_obj =
        hwloc_get_obj_by_depth(hwloc_topology, hwloc_obj_depth, hwloc_obj_index);

    hwloc_bitmap_t bitmap = hwloc_bitmap_alloc();
    hwloc_bitmap_or(bitmap, bitmap, hwloc_obj->nodeset);

    if (hwloc_obj->type == HWLOC_OBJ_NUMANODE) {
        flags = (hwloc_membind_flags_t) ((int) flags | (int) HWLOC_MEMBIND_BYNODESET);
    } else {
#ifdef HAVE_ERROR_CHECKING
        ret =
            MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                 MPI_ERR_OTHER, "**invalidmembind", "**invalidmembind %d", gid);
#endif /* HAVE_ERROR_CHECKING */
        hwloc_bitmap_free(bitmap);
        return ret;
    }

    ret = hwloc_set_area_membind(hwloc_topology, baseaddr, len, bitmap, policy, flags);

    hwloc_bitmap_free(bitmap);
#endif

    return ret;
}


uint64_t MPIR_hwtopo_get_node_mem(void)
{
    uint64_t size = 0;

    if (!bindset_is_valid)
        return size;

#ifdef HAVE_HWLOC
    hwloc_obj_t tmp = NULL;
    while ((tmp = hwloc_get_next_obj_by_type(hwloc_topology, HWLOC_OBJ_NUMANODE, tmp)))
        size += tmp->total_memory;
#endif

    return size;
}

#ifdef HAVE_HWLOC
static hwloc_obj_t get_first_non_io_obj_by_pci(int domain, int bus, int dev, int func)
{
    hwloc_obj_t io_device = hwloc_get_pcidev_by_busid(hwloc_topology, domain, bus, dev, func);
    MPIR_Assert(io_device);
    hwloc_obj_t first_non_io = hwloc_get_non_io_ancestor_obj(hwloc_topology, io_device);
    MPIR_Assert(first_non_io);
    return first_non_io;
}

/* Determine if PCI device is "close" to this process by checking if this process's affinity is
 * included in PCI device's affinity or if PCI device's affinity is included in this process's
 * affinity */
static bool pci_device_is_close(hwloc_obj_t device)
{
    return (hwloc_bitmap_isincluded(bindset, device->cpuset) ||
            hwloc_bitmap_isincluded(device->cpuset, bindset));
}
#endif

bool MPIR_hwtopo_is_dev_close_by_name(const char *name)
{
    bool is_close = false;
    if (!bindset_is_valid)
        return is_close;
#ifdef HAVE_HWLOC
    MPIR_hwtopo_gid_t gid = MPIR_hwtopo_get_obj_by_name(name);
    int hwloc_obj_index = HWTOPO_GET_INDEX(gid);
    int hwloc_obj_depth = HWTOPO_GET_DEPTH(gid);
    is_close = pci_device_is_close(hwloc_get_obj_by_depth(hwloc_topology, hwloc_obj_depth,
                                                          hwloc_obj_index));
#endif
    return is_close;
}

bool MPIR_hwtopo_is_dev_close_by_pci(int domain, int bus, int dev, int func)
{
    bool is_close = false;
    if (!bindset_is_valid)
        return is_close;
#ifdef HAVE_HWLOC
    is_close = pci_device_is_close(get_first_non_io_obj_by_pci(domain, bus, dev, func));
#endif
    return is_close;
}

MPIR_hwtopo_gid_t MPIR_hwtopo_get_dev_parent_by_pci(int domain, int bus, int dev, int func)
{
    MPIR_hwtopo_gid_t gid = MPIR_HWTOPO_GID_ROOT;
    if (!bindset_is_valid)
        return gid;
#ifdef HAVE_HWLOC
    hwloc_obj_t first_non_io = get_first_non_io_obj_by_pci(domain, bus, dev, func);
    hwtopo_class_e class = get_type_class(first_non_io->type);
    gid = HWTOPO_GET_GID(class, first_non_io->depth, first_non_io->logical_index);
#endif
    return gid;
}
