/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"

#ifdef HAVE_NETLOC
#include "netloc_util.h"
#include "mpl.h"

#undef FUNCNAME
#define FUNCNAME get_tree_attributes
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int get_tree_attributes(netloc_topology_t topology,
                               MPIR_Netloc_network_attributes * network_attr)
{
    int i, j, k, l;
    netloc_node_t **traversal_order = NULL;
    int traversal_start, traversal_end;
    int visited_count;
    int mpi_errno = MPI_SUCCESS;
    int max_depth = 0;
    netloc_dt_lookup_table_t *host_nodes = NULL, *nodes = NULL;
    struct netloc_dt_lookup_table_iterator *hti = NULL, *ti = NULL;
    int start_index = 0, end_index = 0;
    int count_at_level_mismatch = 0;
    int bandwidth_mismatch = 0;
    int out_degree_mismatch = 0;
    int prev_width = 0;
    int edges_go_across_levels = 0;
    int out_degree_mismatch_at_level = 0;
    MPIR_CHKPMEM_DECL(3);

    network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;

    host_nodes =
        (netloc_dt_lookup_table_t *) MPL_malloc(sizeof(netloc_dt_lookup_table_t), MPL_MEM_OTHER);
    mpi_errno = netloc_get_all_host_nodes(topology, host_nodes);

    if (!mpi_errno) {
        int host_node_at_leaf_level;

        /* Traversal order never exceeds the total number of nodes */
        MPIR_CHKPMEM_MALLOC(traversal_order, netloc_node_t **,
                            sizeof(netloc_node_t *) * topology->num_nodes, mpi_errno,
                            "traversal_order", MPL_MEM_OTHER);

        hti = netloc_dt_lookup_table_iterator_t_construct(*host_nodes);

        host_node_at_leaf_level = 1;
        while (!netloc_lookup_table_iterator_at_end(hti)) {
            netloc_node_t *node = (netloc_node_t *) netloc_lookup_table_iterator_next_entry(hti);
            int num_edges;
            netloc_edge_t **edges;

            if (node == NULL) {
                break;
            }
            netloc_get_all_edges(topology, node, &num_edges, &edges);
            for (i = 0; i < num_edges; i++) {
                /* Check that outgoing edge connects to switches only */
                if (edges[i]->dest_node->node_type != NETLOC_NODE_TYPE_SWITCH) {
                    host_node_at_leaf_level = 0;
                    break;
                }
            }

            if (!host_node_at_leaf_level)
                break;
            MPIR_Assert(end_index < topology->num_nodes);
            traversal_order[end_index++] = node;
        }

        if (!host_node_at_leaf_level) {
            network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
            goto fn_exit;
        }

        nodes =
            (netloc_dt_lookup_table_t *) MPL_malloc(sizeof(netloc_dt_lookup_table_t),
                                                    MPL_MEM_OTHER);
        mpi_errno = netloc_get_all_nodes(topology, nodes);

        if (!mpi_errno) {
            /* Traverse the graph in topological order */
            while (start_index < end_index) {
                netloc_node_t *node = traversal_order[start_index++];
                /* Add all neighbors to traversal list */
                netloc_edge_t **edges;
                int num_edges;
                netloc_get_all_edges(topology, node, &num_edges, &edges);
                for (j = 0; j < num_edges; j++) {
                    netloc_node_t *neighbor = edges[j]->dest_node;
                    int parents_visited = 1;

                    ti = netloc_dt_lookup_table_iterator_t_construct(*nodes);
                    while (!netloc_lookup_table_iterator_at_end(ti)) {
                        int num_parents_covered = 0, num_parents = 0;
                        netloc_node_t *parent_node =
                            (netloc_node_t *) netloc_lookup_table_iterator_next_entry(ti);
                        netloc_edge_t **neighbor_edges;
                        int neighbor_num_edges;

                        if (parent_node == NULL) {
                            break;
                        }
                        netloc_get_all_edges(topology, parent_node, &neighbor_num_edges,
                                             &neighbor_edges);
                        for (k = 0; k < neighbor_num_edges; k++) {
                            if (neighbor_edges[k]->dest_node != neighbor) {
                                continue;
                            }
                            num_parents++;
                            for (l = 0; l < start_index; l++) {
                                if (traversal_order[l] == parent_node) {
                                    num_parents_covered++;
                                    break;
                                }
                            }
                        }
                        if (num_parents != num_parents_covered) {
                            parents_visited = 0;
                            break;
                        }
                    }
                    if (parents_visited) {
                        int added_previously = 0;
                        for (k = 0; k < end_index; k++) {
                            if (traversal_order[k] == neighbor) {
                                added_previously = 1;
                                break;
                            }
                        }
                        if (!added_previously) {
                            MPIR_Assert(end_index < topology->num_nodes);
                            traversal_order[end_index++] = neighbor;
                        }
                    }
                }
            }
            if (end_index < topology->num_nodes) {
                /* Cycle in the graph, not a tree or close network */
                network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
                goto fn_exit;
            }

            /* Assign depths to nodes of the graph bottom up, in breadth first order */
            /* Indexed by node id, visited_node_list[i] > -1 indicates that the
             * node i has been visited */

            MPIR_CHKPMEM_MALLOC(network_attr->u.tree.node_levels, int *,
                                sizeof(int) * topology->num_nodes, mpi_errno,
                                "network_attr->u.tree.node_levels", MPL_MEM_OTHER);

            for (i = 0; i < topology->num_nodes; i++) {
                network_attr->u.tree.node_levels[i] = -1;
            }

            traversal_end = 0;
            traversal_start = 0;
            visited_count = 0;
            /* Initialize host depths to zero */
            for (i = 0; i < topology->num_nodes; i++) {
                if (topology->nodes[i]->node_type == NETLOC_NODE_TYPE_HOST) {
                    int num_edges;
                    netloc_edge_t **edges;

                    /* Mark the host node as visited */
                    network_attr->u.tree.node_levels[topology->nodes[i]->__uid__] = 0;
                    visited_count++;

                    /* Copy all parents without duplicates to the traversal list */
                    netloc_get_all_edges(topology, topology->nodes[i], &num_edges, &edges);
                    for (j = 0; j < num_edges; j++) {
                        if (network_attr->u.tree.node_levels[edges[j]->dest_node->__uid__] < 0) {
                            traversal_order[traversal_end++] = edges[j]->dest_node;
                            /* Switch levels start from 1 */
                            network_attr->u.tree.node_levels[edges[j]->dest_node->__uid__] = 1;
                            max_depth = 1;
                            visited_count++;
                        }
                    }
                }
            }
            while (visited_count < topology->num_nodes) {
                netloc_node_t *traversed_node = traversal_order[traversal_start++];
                int num_edges;
                netloc_edge_t **edges;
                int depth = network_attr->u.tree.node_levels[traversed_node->__uid__];

                /* Find all nodes not visited with an edge from the current node */
                netloc_get_all_edges(topology, traversed_node, &num_edges, &edges);
                for (j = 0; j < num_edges; j++) {
                    if (edges[j]->dest_node == traversed_node) {
                        continue;
                    }
                    if (network_attr->u.tree.node_levels[edges[j]->dest_node->__uid__] < 0 ||
                        (depth + 1) <
                        network_attr->u.tree.node_levels[edges[j]->dest_node->__uid__]) {
                        traversal_order[traversal_end++] = edges[j]->dest_node;
                        network_attr->u.tree.node_levels[edges[j]->dest_node->__uid__] = depth + 1;
                        if (max_depth < depth + 1) {
                            max_depth = depth + 1;
                        }
                        visited_count++;
                    }
                }
            }
            /* End of depth assignment */

            /* Count the number of nodes and bandwidth at each level */
            for (i = 0; i < max_depth; i++) {
                int width_at_level = 0;
                int num_nodes_at_level = 0;
                int out_degree_at_level = -1;
                for (j = 0; j < topology->num_nodes; j++) {
                    if (network_attr->u.tree.node_levels[topology->nodes[j]->__uid__] == i) {
                        int num_edges;
                        netloc_edge_t **edges;
                        num_nodes_at_level++;
                        int edge_width = 0;
                        int node_out_degree = 0;
                        netloc_get_all_edges(topology, topology->nodes[j], &num_edges, &edges);
                        for (k = 0; k < num_edges; k++) {
                            if (edges[k]->src_node == topology->nodes[j]) {
                                edge_width += edges[k]->bandwidth;
                            }
                            if (edges[k]->src_node == topology->nodes[j] &&
                                network_attr->u.tree.node_levels[edges[k]->dest_node->__uid__] ==
                                i - 1) {
                                node_out_degree++;
                            }
                            if (edges[k]->src_node == topology->nodes[j] &&
                                (network_attr->u.tree.node_levels[edges[k]->dest_node->__uid__] !=
                                 i + 1) &&
                                (network_attr->u.tree.node_levels[edges[k]->dest_node->__uid__] !=
                                 i - 1)) {
                                edges_go_across_levels = 1;
                                break;
                            }
                        }
                        if (i > 0) {
                            if (out_degree_at_level == -1) {
                                out_degree_at_level = node_out_degree;
                            } else if (out_degree_at_level != node_out_degree) {
                                out_degree_mismatch_at_level = i;
                                break;
                            }
                        }
                        if (edges_go_across_levels) {
                            break;
                        }
                        if ((i == max_depth && node_out_degree != 0) ||
                            (i > 0 && node_out_degree != 2)) {
                            out_degree_mismatch = 1;
                        }
                        if (width_at_level == 0) {
                            width_at_level = edge_width;
                        } else if (width_at_level != edge_width) {
                            bandwidth_mismatch = 1;
                        }
                    }
                }
                if (out_degree_mismatch_at_level || edges_go_across_levels) {
                    break;
                }
                if (num_nodes_at_level != ((unsigned) 1 << (max_depth - i))) {
                    count_at_level_mismatch = 1;
                }
                if (i > 0 && width_at_level != 2 * prev_width) {
                    bandwidth_mismatch = 1;
                }
            }
            if (!out_degree_mismatch_at_level && !edges_go_across_levels) {
                network_attr->type = MPIR_NETLOC_NETWORK_TYPE__CLOS_NETWORK;
                if (!count_at_level_mismatch && !bandwidth_mismatch && !out_degree_mismatch) {
                    network_attr->type = MPIR_NETLOC_NETWORK_TYPE__FAT_TREE;
                }
            } else {
                network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
            }
        }
    }

  fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_CHKPMEM_REAP();
    if (host_nodes != NULL) {
        MPL_free(host_nodes);
    }
    if (nodes != NULL) {
        MPL_free(nodes);
    }
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Netloc_parse_topology
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Netloc_parse_topology(netloc_topology_t netloc_topology,
                               MPIR_Netloc_network_attributes * network_attr)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = get_tree_attributes(MPIR_Process.netloc_topology, network_attr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static int get_physical_address(hwloc_obj_t hwloc_obj, char **device_physical_address)
{
    char *physical_address = NULL;
    int i;
    int mpi_errno = MPI_SUCCESS;

    for (i = 0; i < hwloc_obj->infos_count; i++) {
        /* Check if node guid matches for infiniband networks */
        if (!strcmp(hwloc_obj->infos[i].name, "NodeGUID")) {
            physical_address =
                (char *) MPL_malloc(sizeof(hwloc_obj->infos[i].value), MPL_MEM_OTHER);
            strcpy(physical_address, hwloc_obj->infos[i].value);
            break;
        }
    }
  fn_exit:
    *device_physical_address = physical_address;
    return mpi_errno;

  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIR_Netloc_get_network_end_point
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Netloc_get_network_end_point(MPIR_Netloc_network_attributes network_attributes,
                                      netloc_topology_t netloc_topology,
                                      hwloc_topology_t hwloc_topology, netloc_node_t ** end_point)
{
    hwloc_obj_t io_device = NULL;
    int mpi_errno = MPI_SUCCESS;
    int node_query_ret;
    netloc_dt_lookup_table_t *host_nodes;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_node_t *node_end_point = NULL;

    host_nodes =
        (netloc_dt_lookup_table_t *) MPL_malloc(sizeof(netloc_dt_lookup_table_t), MPL_MEM_OTHER);
    node_query_ret = netloc_get_all_host_nodes(netloc_topology, host_nodes);
    if (node_query_ret) {
        /* No nodes found, which means that topology load failed */
        MPIR_ERR_CHKANDJUMP(node_query_ret, mpi_errno, MPI_ERR_OTHER, "**netloc_topo_load");
    }

    while ((io_device = hwloc_get_next_osdev(hwloc_topology, io_device))
           != NULL) {
        char *physical_address = NULL;
        /* Check if the physical id of the io device matches a node in netloc topology tree */
        mpi_errno = get_physical_address(io_device, &physical_address);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        if (physical_address != NULL) {
            /* Find the node in netloc tree with the same physical id */
            hti = netloc_dt_lookup_table_iterator_t_construct(*host_nodes);
            while (!netloc_lookup_table_iterator_at_end(hti)) {
                netloc_node_t *node =
                    (netloc_node_t *) netloc_lookup_table_iterator_next_entry(hti);
                if (node == NULL) {
                    break;
                }
                if (!strcmp(physical_address, node->physical_id)) {
                    node_end_point = node;
                    break;
                }
            }
            MPL_free(physical_address);
        }
    }

    if (node_end_point == NULL) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**netloc_endpoint_mismatch");
    }

  fn_exit:
    if (host_nodes != NULL) {
        MPL_free(host_nodes);
    }
    *end_point = node_end_point;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Netloc_get_hostnode_index_in_tree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Netloc_get_hostnode_index_in_tree(MPIR_Netloc_network_attributes attributes,
                                           netloc_topology_t topology,
                                           netloc_node_t * network_endpoint,
                                           int *index, int *num_leaf_nodes)
{
    int mpi_errno = MPI_SUCCESS;
    int max_level = 0;
    netloc_node_t **traversal_order;
    int start_index, end_index, i, j, node_index, num_nodes;
    MPIR_CHKPMEM_DECL(3);

    MPIR_CHKPMEM_MALLOC(traversal_order, netloc_node_t **,
                        sizeof(netloc_node_t *) * topology->num_nodes, mpi_errno,
                        "traversal_order", MPL_MEM_OTHER);

    /* Assign index to nodes via breadth first traversal starting from nodes with maximum level,
     * corresponding to the top level switches */
    for (i = 0; i < topology->num_nodes; i++) {
        if (attributes.u.tree.node_levels[topology->nodes[i]->__uid__] > max_level) {
            max_level = attributes.u.tree.node_levels[topology->nodes[i]->__uid__];
        }
    }

    end_index = 0;
    for (i = 0; i < topology->num_nodes; i++) {
        if (attributes.u.tree.node_levels[topology->nodes[i]->__uid__] == max_level) {
            MPIR_Assert(end_index < topology->num_nodes);
            traversal_order[end_index++] = topology->nodes[i];
        }
    }
    start_index = 0;
    node_index = 0;
    num_nodes = 0;
    while (start_index < end_index) {
        int num_edges;
        netloc_edge_t **edges;
        netloc_node_t *traversal_node = traversal_order[start_index++];
        if (traversal_node == network_endpoint) {
            node_index = num_nodes;
        }
        if (traversal_node->node_type == NETLOC_NODE_TYPE_HOST) {
            num_nodes++;
        }
        netloc_get_all_edges(topology, traversal_node, &num_edges, &edges);
        for (i = 0; i < num_edges; i++) {
            int node_added_before = 0;
            for (j = 0; j < end_index; j++) {
                if (traversal_order[j] == edges[i]->dest_node) {
                    node_added_before = 1;
                    break;
                }
            }
            if (!node_added_before) {
                MPIR_Assert(end_index < topology->num_nodes);
                traversal_order[end_index++] = edges[i]->dest_node;
            }
        }
    }

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_CHKPMEM_REAP();
    *index = node_index;
    *num_leaf_nodes = num_nodes;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Netloc_get_switches_at_level
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Netloc_get_switches_at_level(netloc_topology_t topology,
                                      MPIR_Netloc_network_attributes attributes, int level,
                                      netloc_node_t *** switches_at_level, int *switch_count)
{
    netloc_node_t **switches = NULL;
    netloc_dt_lookup_table_t *switch_nodes = NULL;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    int mpi_errno = MPI_SUCCESS;
    int i = 0;
    int node_query_ret;

    switches = (netloc_node_t **) MPL_malloc(sizeof(netloc_node_t *), MPL_MEM_OTHER);
    switch_nodes =
        (netloc_dt_lookup_table_t *) MPL_malloc(sizeof(netloc_dt_lookup_table_t), MPL_MEM_OTHER);
    node_query_ret = netloc_get_all_switch_nodes(topology, switch_nodes);

    if (node_query_ret) {
        /* No switch nodes found, which means that topology load failed */
        MPIR_ERR_CHKANDJUMP(node_query_ret, mpi_errno, MPI_ERR_OTHER, "**netloc_topo_load");
    }

    hti = netloc_dt_lookup_table_iterator_t_construct(*switch_nodes);
    while (!netloc_lookup_table_iterator_at_end(hti)) {
        netloc_node_t *switch_node = (netloc_node_t *) netloc_lookup_table_iterator_next_entry(hti);
        if (switch_node == NULL) {
            break;
        }
        if (attributes.u.tree.node_levels[switch_node->__uid__] == level) {
            switches =
                (netloc_node_t **) MPL_realloc(switches, sizeof(netloc_node_t *) * i,
                                               MPL_MEM_OTHER);
            switches[i++] = switch_node;
        }
    }

  fn_exit:
    if (switches != NULL) {
        MPL_free(switches);
    }
    if (switch_nodes != NULL) {
        MPL_free(switch_nodes);
    }
    *switches_at_level = switches;
    *switch_count = i;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif
