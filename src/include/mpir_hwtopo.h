/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_HWTOPO_H_INCLUDED
#define MPIR_HWTOPO_H_INCLUDED

/* hwtopo types */
typedef enum {
    MPIR_HWTOPO_TYPE__NONE = -1,
    MPIR_HWTOPO_TYPE__NODE,
    MPIR_HWTOPO_TYPE__PACKAGE,
    MPIR_HWTOPO_TYPE__SOCKET,
    MPIR_HWTOPO_TYPE__CPU,
    MPIR_HWTOPO_TYPE__CORE,
    MPIR_HWTOPO_TYPE__HWTHREAD,
    MPIR_HWTOPO_TYPE__L1CACHE,
    MPIR_HWTOPO_TYPE__L2CACHE,
    MPIR_HWTOPO_TYPE__L3CACHE,
    MPIR_HWTOPO_TYPE__L4CACHE,
    MPIR_HWTOPO_TYPE__L5CACHE,
    MPIR_HWTOPO_TYPE__DDR,
    MPIR_HWTOPO_TYPE__HBM,
    MPIR_HWTOPO_TYPE__PCI_DEVICE,
    MPIR_HWTOPO_TYPE__MAX
} MPIR_hwtopo_type_e;

/* ROOT GID is same as MACHINE GID (refer to mpir_hwtopo.h) */
#define MPIR_HWTOPO_GID_ROOT (0x00030000)

typedef int MPIR_hwtopo_gid_t;

/* Topology managing APIs start here. These allow for the
 * users to to init/finalize and query the status of the
 * topology layer during runtime */

int MPII_hwtopo_init(void);

int MPII_hwtopo_finalize(void);

bool MPIR_hwtopo_is_initialized(void);


/* Topology tree APIs start here. These allow for users to
 * get an entry point in the topology tree, usually this is
 * the lowest object in the tree that has matching cpuset
 * with the current process */

/*
 * Return the global id of the leaf object for this process. The leaf
 * is the lowest object in the topology tree such that the cpuset of
 * the process invoking the function is equal to the cpuset of the leaf
 * or a subset of it, that is, the leaf contains the cpuset of the
 * calling process or it matches it.
 */
MPIR_hwtopo_gid_t MPIR_hwtopo_get_leaf(void);

/*
 * Return the local id for the object with global id: gid. The local
 * id corresponds to hwloc logical index for the object. This API is
 * used by the topotree code, which expects an hwloc logical index to
 * function correctly.
 */
int MPIR_hwtopo_get_lid(MPIR_hwtopo_gid_t gid);

/*
 * Return the depth in the topology tree for the given gid. A gid
 * returned by the topology layer can be passed to this function to
 * query the level of the object in the topology tree. For Normal
 * class objects (e.g., cores, hwthreads, caches, etc) this is a
 * positive integer. For non-Normal objects (e.g., Numa nodes) this
 * is a negative integer (-3 for Numa nodes).
 */
int MPIR_hwtopo_get_depth(MPIR_hwtopo_gid_t gid);

/*
 * Return the global id of the ancestor object located at level depth.
 * This function allows users to traverse the topology tree from a
 * leaf up to the root. The following example walks from leaf to root:
 *
 *   MPIR_hwtopo_gid_t gid = MPIR_hwtopo_get_leaf();
 *   int depth = MPIR_hwtopo_get_depth(gid);
 *   while (depth > 0) {
 *      gid = MPIR_hwtopo_get_ancestor(gid, --depth);
 *   }
 *
 * NOTE: as already mentioned, non-Normal objects are not part of the
 *       main tree and thus MPIR_hwtopo_get_leaf() will never return
 *       the gid of a non-Normal object with negative depth.
 */
MPIR_hwtopo_gid_t MPIR_hwtopo_get_ancestor(MPIR_hwtopo_gid_t gid, int depth);


/* Topology flat APIs start here. These allow for users to
 * get any object in the topology tree based on the process
 * locality and the type of the requested object */

/*
 * Return the enum type corresponding to the requested name. For example,
 * MPIR_hwtopo_get_type_id("ddr") returns MPIR_HWTOPO_TYPE__DDR.
 */
MPIR_hwtopo_type_e MPIR_hwtopo_get_type_id(const char *name);

/*
 * Return the gid of the object affine to the querying process by type.
 * Similar to MPIR_hwtopo_get_leaf() but will also return non-Normal
 * objects (with negative depth).
 */
MPIR_hwtopo_gid_t MPIR_hwtopo_get_obj_by_type(MPIR_hwtopo_type_e type);

/*
 * Similar to MPIR_hwtopo_get_obj_by_type but takes a name instead.
 * MPIR_hwtopo_get_obj_by_type(MPIR_HWTOPO_TYPE__DDR) is equivalent to
 * MPIR_hwtopo_get_obj_by_name("ddr").
 */
MPIR_hwtopo_gid_t MPIR_hwtopo_get_obj_by_name(const char *name);

/*
 * Bind the buffer starting at 'baseaddr' and with length 'len' to the
 * memory object identified by gid.
 */
int MPIR_hwtopo_mem_bind(void *baseaddr, size_t len, MPIR_hwtopo_gid_t gid);


/* Topology miscellaneous APIs start here. These allow for the
 * user to query general properties of the system like total amount
 * of memory in the node, for example */

/*
 * Return the total amount of system memory (includes HBM)
 */
uint64_t MPIR_hwtopo_get_node_mem(void);

/*
 * Return true if device is close to this process
 */
bool MPIR_hwtopo_is_dev_close_by_name(const char *name);

/*
 * Return true if pci device is close to this process
 */
bool MPIR_hwtopo_is_dev_close_by_pci(int domain, int bus, int dev, int func);

/*
 * Return the global id of the first non-io object above the PCI device
 */
MPIR_hwtopo_gid_t MPIR_hwtopo_get_dev_parent_by_pci(int domain, int bus, int dev, int func);
#endif /* MPIR_HWTOPO_H_INCLUDED */
