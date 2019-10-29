/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_HW_TOPO_H_INCLUDED
#define MPIR_HW_TOPO_H_INCLUDED

/*
 * Node hardware object types
 */
typedef enum {
    MPIR_NODE_OBJ_TYPE__NONE = -1,
    MPIR_NODE_OBJ_TYPE__MACHINE,
    MPIR_NODE_OBJ_TYPE__PACKAGE,
    MPIR_NODE_OBJ_TYPE__CORE,
    MPIR_NODE_OBJ_TYPE__PU,
    MPIR_NODE_OBJ_TYPE__L1CACHE,
    MPIR_NODE_OBJ_TYPE__L2CACHE,
    MPIR_NODE_OBJ_TYPE__L3CACHE,
    MPIR_NODE_OBJ_TYPE__L4CACHE,
    MPIR_NODE_OBJ_TYPE__L5CACHE,
    MPIR_NODE_OBJ_TYPE__L1ICACHE,
    MPIR_NODE_OBJ_TYPE__L2ICACHE,
    MPIR_NODE_OBJ_TYPE__L3ICACHE,
    MPIR_NODE_OBJ_TYPE__GROUP,
    MPIR_NODE_OBJ_TYPE__NUMANODE,
    MPIR_NODE_OBJ_TYPE__BRIDGE,
    MPIR_NODE_OBJ_TYPE__PCI_DEVICE,
    MPIR_NODE_OBJ_TYPE__OS_DEVICE,
    MPIR_NODE_OBJ_TYPE__MISC,
    MPIR_NODE_OBJ_TYPE__MAX
} MPIR_Node_obj_type;

/*
 * Node hardware OS device object types
 */
typedef enum {
    MPIR_NODE_OBJ_OSDEV_TYPE__NONE = -1,
    MPIR_NODE_OBJ_OSDEV_TYPE__BLOCK,
    MPIR_NODE_OBJ_OSDEV_TYPE__GPU,
    MPIR_NODE_OBJ_OSDEV_TYPE__NETWORK,
    MPIR_NODE_OBJ_OSDEV_TYPE__OPENFABRICS,
    MPIR_NODE_OBJ_OSDEV_TYPE__DMA,
    MPIR_NODE_OBJ_OSDEV_TYPE__COPROC
} MPIR_Node_obj_osdev_type;

/*
 * Network topology types
 */
typedef enum {
    MPIR_NETWORK_TOPOLOGY_TYPE__FAT_TREE,
    MPIR_NETWORK_TOPOLOGY_TYPE__CLOS_NETWORK,
    MPIR_NETWORK_TOPOLOGY_TYPE__TORUS,
    MPIR_NETWORK_TOPOLOGY_TYPE__INVALID
} MPIR_Network_topology_type;

/*
 * Network node types
 */
typedef enum {
    MPIR_NETWORK_NODE_TYPE__HOST,
    MPIR_NETWORK_NODE_TYPE__SWITCH,
    MPIR_NETWORK_NODE_TYPE__INVALID
} MPIR_Network_node_type;

/*
 * Definitions for node objects
 */
typedef void *MPIR_Node_obj;

/*
 * Definitions for network objects
 */
typedef void *MPIR_Network_node;
typedef void *MPIR_Network_edge;

/*
 * Initialize hardware topology
 */
int MPII_hw_topo_init(void);

/*
 * Finalize hardware topology
 */
int MPII_hw_topo_finalize(void);

/*
 * Check whether hardware topology is initialized or not
 */
bool MPIR_hw_topo_is_initialized(void);

/*
 * Return the min level object covering this process
 */
MPIR_Node_obj MPIR_Node_get_covering_obj(void);

/*
 * Return covering object by its type
 */
MPIR_Node_obj MPIR_Node_get_covering_obj_by_type(MPIR_Node_obj_type obj_type);

/*
 * Return object parent
 */
MPIR_Node_obj MPIR_Node_get_parent_obj(MPIR_Node_obj obj);

/*
 * Return the logical id of the object
 */
int MPIR_Node_get_obj_index(MPIR_Node_obj obj);

/*
 * Return the depth of the object
 */
int MPIR_Node_get_obj_depth(MPIR_Node_obj obj);

/*
 * Return type of the object
 */
MPIR_Node_obj_type MPIR_Node_get_obj_type(MPIR_Node_obj obj);

/*
 * Return node total memory
 */
uint64_t MPIR_Node_get_total_mem(void);

/*
 * Return non I/O ancestor object for dev_obj
 */
MPIR_Node_obj MPIR_Node_get_non_io_ancestor_obj(MPIR_Node_obj dev_obj);

/*
 * Return device obj for bus_id_string
 */
MPIR_Node_obj MPIR_Node_get_osdev_obj_by_busidstring(const char *bus_id_string);

/*
 * Return the non I/O ancestor shared by current process and device with dev_name
 */
MPIR_Node_obj MPIR_Node_get_common_non_io_ancestor_obj(const char *dev_name);

/*
 * Return OS device type of dev_obj
 */
MPIR_Node_obj_osdev_type MPIR_Node_get_osdev_obj_type(MPIR_Node_obj dev_obj);

/*
 * Return network topology type
 */
MPIR_Network_topology_type MPIR_Net_get_topo_type(void);

/*
 * Return network topology node type
 */
MPIR_Network_node_type MPIR_Net_get_node_type(MPIR_Network_node node);

/*
 * Return a pointer to this node in network topology
 */
MPIR_Network_node MPIR_Net_get_endpoint(void);

/*
 * Return edge destination node
 */
MPIR_Network_node MPIR_Net_get_edge_dest_node(MPIR_Network_edge edge);

/*
 * Return __uid__ of node in network topology
 */
int MPIR_Net_get_node_uid(MPIR_Network_node node);

/*
 * Return number of nodes in network topology
 */
int MPIR_Net_get_num_nodes(void);

/*
 * Return the number of edges and edge pointers for this node in network topology
 */
int MPIR_Net_get_all_edges(MPIR_Network_node node, int *num_edges, MPIR_Network_edge ** edges);

/*
 * Return node level in tree network topology
 */
int *MPIR_Net_tree_topo_get_node_levels(void);

/*
 * Return the hostnode index in the tree network topology
 */
int MPIR_Net_tree_topo_get_hostnode_index(int *node_index, int *num_nodes);

/*
 * Return list of switches at certain level in the tree network topology
 */
int MPIR_Net_tree_topo_get_switches_at_level(int switch_level,
                                             MPIR_Network_node ** switches_at_level,
                                             int *switch_count);

/*
 * Return dimension of the torus network topology
 */
int MPIR_Net_torus_topo_get_dimension(void);

/*
 * Return geometry of the torus network topology
 */
int *MPIR_Net_torus_topo_get_geometry(void);

/*
 * Return node index in the torus network topology
 */
int MPIR_Net_torus_topo_get_node_index(void);

#endif /* MPIR_HW_TOPO_H_INCLUDED */
