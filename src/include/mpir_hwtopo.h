/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_HWTOPO_H_INCLUDED
#define MPIR_HWTOPO_H_INCLUDED

/*
 * Node hardware object types
 */
typedef enum {
    MPIR_HWTOPO_TYPE__NONE = -1,
    MPIR_HWTOPO_TYPE__MACHINE,
    MPIR_HWTOPO_TYPE__PACKAGE,
    MPIR_HWTOPO_TYPE__CORE,
    MPIR_HWTOPO_TYPE__PU,
    MPIR_HWTOPO_TYPE__L1CACHE,
    MPIR_HWTOPO_TYPE__L2CACHE,
    MPIR_HWTOPO_TYPE__L3CACHE,
    MPIR_HWTOPO_TYPE__L4CACHE,
    MPIR_HWTOPO_TYPE__L5CACHE,
    MPIR_HWTOPO_TYPE__L1ICACHE,
    MPIR_HWTOPO_TYPE__L2ICACHE,
    MPIR_HWTOPO_TYPE__L3ICACHE,
    MPIR_HWTOPO_TYPE__GROUP,
    MPIR_HWTOPO_TYPE__NUMANODE,
    MPIR_HWTOPO_TYPE__BRIDGE,
    MPIR_HWTOPO_TYPE__PCI_DEVICE,
    MPIR_HWTOPO_TYPE__OS_DEVICE,
    MPIR_HWTOPO_TYPE__MISC,
    MPIR_HWTOPO_TYPE__MAX
} MPIR_hwtopo_type_e;

/*
 * Network topology types
 */
typedef enum {
    MPIR_HWTOPO_NET_TYPE__FAT_TREE,
    MPIR_HWTOPO_NET_TYPE__CLOS_NETWORK,
    MPIR_HWTOPO_NET_TYPE__TORUS,
    MPIR_HWTOPO_NET_TYPE__INVALID
} MPIR_hwtopo_net_type_e;

/*
 * Network node types
 */
typedef enum {
    MPIR_HWTOPO_NET_NODE_TYPE__HOST,
    MPIR_HWTOPO_NET_NODE_TYPE__SWITCH,
    MPIR_HWTOPO_NET_NODE_TYPE__INVALID
} MPIR_hwtopo_net_node_type_e;

/*
 * Definitions for node objects
 */
typedef void *MPIR_hwtopo_obj_t;

/*
 * Definitions for network objects
 */
typedef void *MPIR_hwtopo_net_node_t;
typedef void *MPIR_hwtopo_net_edge_t;

/*
 * Initialize hardware topology
 */
int MPII_hwtopo_init(void);

/*
 * Finalize hardware topology
 */
int MPII_hwtopo_finalize(void);

/*
 * Check whether hardware topology is initialized or not
 */
bool MPIR_hwtopo_is_initialized(void);

/*
 * Return the min level object covering this process
 */
MPIR_hwtopo_obj_t MPIR_hwtopo_get_covering_obj(void);

/*
 * Return covering object by its type
 */
MPIR_hwtopo_obj_t MPIR_hwtopo_get_covering_obj_by_type(MPIR_hwtopo_type_e obj_type);

/*
 * Return object parent
 */
MPIR_hwtopo_obj_t MPIR_hwtopo_get_parent_obj(MPIR_hwtopo_obj_t obj);

/*
 * Return the logical id of the object
 */
int MPIR_hwtopo_get_obj_index(MPIR_hwtopo_obj_t obj);

/*
 * Return the depth of the object
 */
int MPIR_hwtopo_get_obj_depth(MPIR_hwtopo_obj_t obj);

/*
 * Return type of the object
 */
MPIR_hwtopo_type_e MPIR_hwtopo_get_obj_type(MPIR_hwtopo_obj_t obj);

/*
 * Return node total memory
 */
uint64_t MPIR_hwtopo_get_total_mem(void);

/*
 * Return the non I/O ancestor shared by current process and device with dev_name
 */
MPIR_hwtopo_obj_t MPIR_hwtopo_get_non_io_ancestor_obj(const char *dev_name);

/*
 * Return network topology type
 */
MPIR_hwtopo_net_type_e MPIR_hwtopo_get_net_type(void);

/*
 * Return network topology node type
 */
MPIR_hwtopo_net_node_type_e MPIR_hwtopo_get_net_node_type(MPIR_hwtopo_net_node_t node);

/*
 * Return a pointer to this node in network topology
 */
MPIR_hwtopo_net_node_t MPIR_hwtopo_get_net_endpoint(void);

/*
 * Return edge destination node
 */
MPIR_hwtopo_net_node_t MPIR_hwtopo_get_net_edge_dest_node(MPIR_hwtopo_net_edge_t edge);

/*
 * Return __uid__ of node in network topology
 */
int MPIR_hwtopo_get_net_node_uid(MPIR_hwtopo_net_node_t node);

/*
 * Return number of nodes in network topology
 */
int MPIR_hwtopo_get_net_num_nodes(void);

/*
 * Return the number of edges and edge pointers for this node in network topology
 */
int MPIR_hwtopo_get_net_all_edges(MPIR_hwtopo_net_node_t node, int *num_edges,
                                  MPIR_hwtopo_net_edge_t ** edges);

/*
 * Return node level in tree network topology
 */
int *MPIR_hwtopo_tree_get_node_levels(void);

/*
 * Return the hostnode index in the tree network topology
 */
int MPIR_hwtopo_tree_get_hostnode_index(int *node_index, int *num_nodes);

/*
 * Return list of switches at certain level in the tree network topology
 */
int MPIR_hwtopo_tree_get_switches_at_level(int switch_level,
                                           MPIR_hwtopo_net_node_t ** switches_at_level,
                                           int *switch_count);

/*
 * Return dimension of the torus network topology
 */
int MPIR_hwtopo_torus_get_dimension(void);

/*
 * Return geometry of the torus network topology
 */
int *MPIR_hwtopo_torus_get_geometry(void);

/*
 * Return node index in the torus network topology
 */
int MPIR_hwtopo_torus_get_node_index(void);

#endif /* MPIR_HWTOPO_H_INCLUDED */
