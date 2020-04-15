/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_NETTOPO_H_INCLUDED
#define MPIR_NETTOPO_H_INCLUDED

/*
 * Network topology types
 */
typedef enum {
    MPIR_NETTOPO_TYPE__FAT_TREE,
    MPIR_NETTOPO_TYPE__CLOS_NETWORK,
    MPIR_NETTOPO_TYPE__TORUS,
    MPIR_NETTOPO_TYPE__INVALID
} MPIR_nettopo_type_e;

/*
 * Network node types
 */
typedef enum {
    MPIR_NETTOPO_NODE_TYPE__HOST,
    MPIR_NETTOPO_NODE_TYPE__SWITCH,
    MPIR_NETTOPO_NODE_TYPE__INVALID
} MPIR_nettopo_node_type_e;

/*
 * Definitions for network objects
 */
typedef void *MPIR_nettopo_node_t;
typedef void *MPIR_nettopo_edge_t;

int MPII_nettopo_init(void);

int MPII_nettopo_finalize(void);

/*
 * Return network topology type
 */
MPIR_nettopo_type_e MPIR_nettopo_get_type(void);

/*
 * Return network topology node type
 */
MPIR_nettopo_node_type_e MPIR_nettopo_get_node_type(MPIR_nettopo_node_t node);

/*
 * Return a pointer to this node in network topology
 */
MPIR_nettopo_node_t MPIR_nettopo_get_endpoint(void);

/*
 * Return edge destination node
 */
MPIR_nettopo_node_t MPIR_nettopo_get_edge_dest_node(MPIR_nettopo_edge_t edge);

/*
 * Return __uid__ of node in network topology
 */
int MPIR_nettopo_get_node_uid(MPIR_nettopo_node_t node);

/*
 * Return number of nodes in network topology
 */
int MPIR_nettopo_get_num_nodes(void);

/*
 * Return the number of edges and edge pointers for this node in network topology
 */
int MPIR_nettopo_get_all_edges(MPIR_nettopo_node_t node, int *num_edges,
                               MPIR_nettopo_edge_t ** edges);

/*
 * Return node level in tree network topology
 */
int *MPIR_nettopo_tree_get_node_levels(void);

/*
 * Return the hostnode index in the tree network topology
 */
int MPIR_nettopo_tree_get_hostnode_index(int *node_index, int *num_nodes);

/*
 * Return list of switches at certain level in the tree network topology
 */
int MPIR_nettopo_tree_get_switches_at_level(int switch_level,
                                            MPIR_nettopo_node_t ** switches_at_level,
                                            int *switch_count);

/*
 * Return dimension of the torus network topology
 */
int MPIR_nettopo_torus_get_dimension(void);

/*
 * Return geometry of the torus network topology
 */
int *MPIR_nettopo_torus_get_geometry(void);

/*
 * Return node index in the torus network topology
 */
int MPIR_nettopo_torus_get_node_index(void);

#endif /* MPIR_NETTOPO_H_INCLUDED */
