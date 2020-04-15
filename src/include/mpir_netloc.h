/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_NETLOC_H_INCLUDED
#define MPIR_NETLOC_H_INCLUDED

#include "netloc.h"
#include "hwloc.h"
#include "mpir_hw_topo.h"

typedef struct {
    MPIR_Network_topology_type type;

    union {
        struct {
            /* The levels at which the host and switch nodes are in
             * the network */
            int *node_levels;
        } tree;
        struct {
            int dimension;
            int *geometry;
            /* Flat index of the node the current
             * rank is mapped to */
            int node_idx;
        } torus;
    } u;

    netloc_node_t *network_endpoint;
} MPIR_Netloc_network_attributes;

int MPIR_Netloc_parse_topology(hwloc_topology_t hwloc_topology, netloc_topology_t topology,
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

#endif /* MPIR_NETLOC_H_INCLUDED */
