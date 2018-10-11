/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef NETLOC_UTIL_H_INCLUDED
#define NETLOC_UTIL_H_INCLUDED

#include "netloc.h"

typedef enum {
    MPIR_NETLOC_NETWORK_TYPE__FAT_TREE,
    MPIR_NETLOC_NETWORK_TYPE__CLOS_NETWORK,
    MPIR_NETLOC_NETWORK_TYPE__TORUS,
    MPIR_NETLOC_NETWORK_TYPE__INVALID,
} MPIR_Netloc_network_topo_type;

typedef struct {
    MPIR_Netloc_network_topo_type type;

    union {
        struct {
            /* the levels at which the host and switch nodes are in
             * the network */
            int *node_levels;
        } tree;
        struct {
            int dimension;
            int *geometry;
            /*Flat index assignment in round robin manner to PUs
             * that translates to a tuple, indexed by the uid of nodes */
            int *node_coordinates;
        } torus;
    } u;

    netloc_node_t *network_endpoint;
} MPIR_Netloc_network_attributes;

int MPIR_Netloc_parse_topology(netloc_topology_t topology,
                               MPIR_Netloc_network_attributes * network_attr);

int MPIR_Netloc_get_network_end_point(MPIR_Netloc_network_attributes,
                                      netloc_topology_t netloc_topology,
                                      hwloc_topology_t hwloc_topology, netloc_node_t ** end_point);

int MPIR_Netloc_get_switches_at_level(netloc_topology_t netloc_topology,
                                      MPIR_Netloc_network_attributes attributes, int level,
                                      netloc_node_t *** switches_at_level, int *switch_count);

int MPIR_Netloc_get_hostnode_index_in_tree(MPIR_Netloc_network_attributes attributes,
                                           netloc_topology_t topology,
                                           netloc_node_t * network_endpoint,
                                           int *index, int *num_nodes);
int MPIR_Netloc_get_torus_network_coordinates(MPIR_Netloc_network_attributes network_attr,
                                              netloc_topology_t netloc_topology,
                                              netloc_node_t * node, long *node_coordinates);

void MPIR_Netloc_get_cart_graph_comm_matrix(int ndim, int *dim, int ** period, double ***comm_matrix);

void MPIR_Netloc_get_mpi_graph_comm_matrix(int num_nodes, const int* index,
	    const int* edges, double ***comm_matrix);

int MPIR_Netloc_get_reordered_rank(int rank, int * newrank, int comm_size, double ** comm_matrix);

#endif /* NETLOC_UTIL_H_INCLUDED */
