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
#define MAX_DISTANCE 65535

typedef struct semicube_edge {
    netloc_node_t **src;
    netloc_node_t **dest;
} semicube_edge;

typedef struct tree {
    netloc_node_t ***vertices;
    int num_nodes;
    semicube_edge **edges;
    int num_edges;
} tree;

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

                errno = MPIR_Netloc_get_network_end_point(MPIR_Process.network_attr,
                                                          MPIR_Process.netloc_topology,
                                                          MPIR_Process.hwloc_topology,
                                                          &MPIR_Process.
                                                          network_attr.network_endpoint);
                if (errno != MPI_SUCCESS) {
                    network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
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

static unsigned int get_hamming_distance(const unsigned long long src_label,
                                         const unsigned long long dest_label)
{
    unsigned int distance = 0;
    unsigned long long hamming_distance;

    hamming_distance = (unsigned long long) src_label ^ dest_label;
    while (hamming_distance) {
        if (hamming_distance & 1) {
            distance++;
        }
        hamming_distance = hamming_distance >> 1;
    }

  fn_exit:
    return distance;

  fn_fail:
    goto fn_exit;
}

static int get_distance(netloc_node_t ** exposed_vertex, netloc_node_t ** root, tree * subtree)
{
    int distance = 0;
    int i;
    if (exposed_vertex == root) {
        return distance;
    }

    for (i = 0; i < subtree->num_edges; i++) {
        int temp_distance = 0;
        if (subtree->edges[0]->dest == exposed_vertex) {
            temp_distance = get_distance(subtree->edges[0]->src, root, subtree);
            if (temp_distance + 1 > distance) {
                distance = temp_distance + 1;
            }
        }
    }

  fn_exit:
    return distance;

  fn_fail:
    goto fn_exit;
}

static void get_path(netloc_node_t ** src, netloc_node_t ** dest, tree ** forest, int tree_count,
                     semicube_edge *** path, int *path_length)
{
    int i, j;
    for (i = 0; i < tree_count; i++) {
        tree *subtree = forest[i];
        int subtree_edge_count = subtree->num_edges;
        for (j = 0; j < subtree_edge_count; j++) {
            int path_size = -1;
            if (subtree->edges[j]->dest == dest) {
                semicube_edge edge;
                edge.src = src;
                edge.dest = dest;
                *path[*path_length] = &edge;
                *path_length = *path_length + 1;
            } else if (subtree->edges[j]->src == src) {
                int destination_reached = 0;
                int current_path_length = *path_length;
                get_path(subtree->edges[j]->dest, dest, forest, tree_count, path, path_length);
                if (path_length >= 0) {
                    semicube_edge *edge = (*path)[path_size - 1];
                    if (edge->dest == dest) {
                        destination_reached = 1;
                        break;
                    }
                }
                if (!destination_reached) {
                    *path_length = current_path_length;
                }
            }
        }
    }

  fn_exit:
    return;

  fn_fail:
    goto fn_exit;
}

static void get_root_of_tree(tree * subtree, netloc_node_t *** root)
{
    int i, j;
    for (i = 0; i < subtree->num_nodes; i++) {
        netloc_node_t **vertex = subtree->vertices[i];
        int rootnode = 1;
        for (j = 0; j < subtree->num_edges; j++) {
            if (subtree->edges[j]->dest == vertex) {
                rootnode = 0;
                break;
            }
        }
        if (rootnode) {
            *root = vertex;
            break;
        }
    }

  fn_exit:
    return;

  fn_fail:
    goto fn_exit;

}

static void get_blossom(netloc_node_t *** nodes, int num_nodes,
                        semicube_edge ** edges, int num_edges, netloc_node_t *** unmarked_vertices,
                        int num_unmarked, netloc_node_t **** blossom_nodes, int *num_blossom_nodes,
                        int *stem_index)
{
    int i, j;
    netloc_node_t **exposed_vertex;
    for (j = 0; j < num_unmarked; j++) {
        if (nodes[i] == unmarked_vertices[j]) {
            netloc_node_t ***traversed_nodes =
                (netloc_node_t ***) MPL_calloc(num_nodes, sizeof(netloc_node_t **), MPL_MEM_OTHER);
            int cycle_index = 0;
            int odd_cycle_found = 0;
            exposed_vertex = nodes[i];
            traversed_nodes[cycle_index++] = exposed_vertex;
            /* Find an odd cycle starting from the exposed vertex */
            while (1) {
                /* Find an edge adjacent to the current node */
                for (i = 0; i < num_edges; i++) {
                    if (edges[i]->src == exposed_vertex) {
                        traversed_nodes[cycle_index++] = edges[i]->dest;
                        exposed_vertex = edges[i]->dest;
                        break;
                    }
                }
                for (i = 0; i < cycle_index - 1; i++) {
                    if (traversed_nodes[i] == exposed_vertex && (cycle_index - 1 - i) & 1) {
                        odd_cycle_found = 1;
                        *num_blossom_nodes = cycle_index;
                        *blossom_nodes = traversed_nodes;
                        *stem_index = i;
                        break;
                    }
                }
                if (odd_cycle_found) {
                    break;
                }
            }
            if (odd_cycle_found) {
                break;
            }
        }
    }
  fn_exit:
    return;

  fn_fail:
    goto fn_exit;
}

static void find_augmenting_path(netloc_topology_t topology, netloc_node_t *** nodes, int num_nodes,
                                 semicube_edge ** edges, int num_edges,
                                 semicube_edge *** augmenting_path, int *augmenting_path_length,
                                 semicube_edge ** maximum_matching, int num_matching_edges)
{
    semicube_edge **current_augmenting_path = NULL;
    int path_length;
    int i, j, k, l;
    int unmarked_vertex_insert, unmarked_edge_insert = 0;
    netloc_node_t ***unmarked_vertices = NULL;
    semicube_edge **unmarked_edges = NULL;
    tree **forest;
    int forest_insert_index;
    /* Unmark all vertices and edges in the semicube graph, mark all edges of the matching */
    unmarked_vertex_insert = 0;
    unmarked_vertices =
        (netloc_node_t ***) MPL_calloc(num_nodes, sizeof(netloc_node_t **), MPL_MEM_OTHER);
    for (i = 0; i < num_nodes; i++) {
        netloc_node_t **semicube_vertex = nodes[i];
        int exposed_vertex = 1;
        for (j = 0; j < num_matching_edges; j++) {
            semicube_edge *edge = maximum_matching[j];
            if (edge->src == semicube_vertex || edge->dest == semicube_vertex) {
                exposed_vertex = 0;
                break;
            }
        }
        if (exposed_vertex) {
            unmarked_vertices[unmarked_vertex_insert++] = semicube_vertex;
        }
    }

    unmarked_edge_insert = 0;
    unmarked_edges =
        (semicube_edge **) MPL_calloc(num_edges, sizeof(semicube_edge *), MPL_MEM_OTHER);
    /* Unmark all vertices and edges in the semicube graph, mark all edges of the matching */
    for (i = 0; i < num_edges; i++) {
        int marked_edge = 0;
        semicube_edge *edge = edges[i];
        for (j = 0; j < num_matching_edges; j++) {
            semicube_edge *local_edge = maximum_matching[j];
            if (local_edge->src == edge->src && local_edge->dest == edge->dest) {
                marked_edge = 1;
                break;
            }
        }
        if (!marked_edge) {
            unmarked_edges[unmarked_edge_insert++] = edge;
        }
    }
    forest = (tree **) MPL_calloc(unmarked_vertex_insert, sizeof(tree *), MPL_MEM_OTHER);
    forest_insert_index = 0;
    /* Create a forest with each exposed vertex as a singleton set */
    for (i = 0; i < unmarked_vertex_insert; i++) {
        tree *singleton_tree = (tree *) MPL_malloc(sizeof(tree), MPL_MEM_OTHER);
        singleton_tree->vertices =
            (netloc_node_t ***) MPL_malloc(sizeof(netloc_node_t **), MPL_MEM_OTHER);
        singleton_tree->edges =
            (semicube_edge **) MPL_malloc(sizeof(semicube_edge *), MPL_MEM_OTHER);
        singleton_tree->vertices[0] = unmarked_vertices[i];
        singleton_tree->num_nodes = 1;
        singleton_tree->num_edges = 0;
        forest[forest_insert_index++] = singleton_tree;
    }

    while (1) {
        int exposed_vertex = 0;
        netloc_node_t **vertex;
        netloc_node_t **root;
        int distance_from_root;
        int unmarked_vertex_index = -1;
        int unmarked_edge_index = -1;
        tree *subtree;

        exposed_vertex = 0;
        for (i = 0; i < forest_insert_index; i++) {
            subtree = forest[i];
            /* Check if the marked vertex is at an even distance from the
             * root node in its tree */
            for (j = 0; j < subtree->num_nodes; j++) {
                vertex = subtree->vertices[j];
                for (l = 0; l < unmarked_vertex_insert; l++) {
                    if (unmarked_vertices[l] == vertex) {
                        exposed_vertex = 1;
                        unmarked_vertex_index = l;
                        break;
                    }
                }
                if (exposed_vertex) {
                    break;
                }
            }
            if (exposed_vertex) {
                break;
            }
        }

        if (!exposed_vertex) {
            break;
        }
        get_root_of_tree(subtree, &root);
        distance_from_root = get_distance(vertex, root, subtree);
        if (!(distance_from_root & 1)) {
            /* Find an unmarked edge from exposed_vertex */
            int unmarked_edge = 0;
            semicube_edge *edge;
            netloc_node_t **dest;
            for (j = 0; j < unmarked_edge_insert; j++) {
                if (unmarked_edges[j]->src == vertex || unmarked_edges[j]->dest == vertex) {
                    unmarked_edge = 1;
                    unmarked_edge_index = j;
                    edge = unmarked_edges[j];
                    dest = unmarked_edges[j]->dest;
                    break;
                }
            }
            if (unmarked_edge) {
                /* Check if the destination is not in the forest */
                int dest_in_forest = 0;
                tree *dest_subtree;
                netloc_node_t **dest_root;
                for (j = 0; j < forest_insert_index; j++) {
                    tree *tempsubtree = forest[j];
                    for (k = 0; k < tempsubtree->num_nodes; k++) {
                        if (tempsubtree->vertices[k] == dest) {
                            dest_in_forest = 1;
                            dest_subtree = tempsubtree;
                            break;
                        }

                    }
                    if (dest_in_forest) {
                        break;
                    }
                }
                get_root_of_tree(dest_subtree, &dest_root);

                if (!dest_in_forest) {
                    int vertex_insert_position;
                    int edge_insert_position = 0;
                    /* The destination node is in matching graph */
                    semicube_edge *matching_edge = NULL;
                    for (j = 0; j < num_matching_edges; j++) {
                        semicube_edge *temp_matching_edge = maximum_matching[j];
                        if (temp_matching_edge->src == dest || temp_matching_edge->dest == dest) {
                            matching_edge = temp_matching_edge;
                            break;
                        }
                    }
                    MPIR_Assert(matching_edge != NULL);
                    /* Add edge to subtree */
                    vertex_insert_position = subtree->num_nodes + 1;
                    subtree->vertices =
                        (netloc_node_t ***) MPL_realloc(subtree->vertices,
                                                        (vertex_insert_position + 1) *
                                                        sizeof(netloc_node_t **), MPL_MEM_OTHER);
                    subtree->vertices[vertex_insert_position] = dest;
                    subtree->num_nodes++;

                    subtree->edges =
                        (semicube_edge **) MPL_realloc(subtree->edges,
                                                       (edge_insert_position + 3) *
                                                       sizeof(subtree->edges[0]), MPL_MEM_OTHER);

                    for (j = 0; j < unmarked_edge_index; j++) {
                        semicube_edge *temp_matching_edge = unmarked_edges[j];
                        if (temp_matching_edge->src == vertex && temp_matching_edge->dest == dest) {
                            subtree->edges[edge_insert_position++] = temp_matching_edge;
                            break;
                        }
                    }
                    subtree->edges[edge_insert_position++] = matching_edge;
                    subtree->num_edges += 2;
                } else if (get_distance(dest, dest_root, dest_subtree) & 1) {
                    /* Do nothing */
                } else {
                    if (root != dest_root) {
                        /* Found an augmenting path */
                        semicube_edge **first_path, **second_path;
                        int insert_index = 0;
                        int first_length = 0, second_length = 0;
                        get_path(root, vertex, forest, forest_insert_index, &first_path,
                                 &first_length);
                        get_path(dest_root, dest, forest, forest_insert_index, &second_path,
                                 &second_length);
                        current_augmenting_path =
                            MPL_calloc(current_augmenting_path, (first_length + second_length + 1) *
                                       sizeof(semicube_edge *), MPL_MEM_OTHER);
                        for (j = 0; j < first_length; j++) {
                            current_augmenting_path[insert_index++] = first_path[j];
                        }
                        current_augmenting_path[insert_index++] = edge;
                        for (j = 0; j < second_length; j++) {
                            current_augmenting_path[insert_index++] = second_path[j];
                        }
                        path_length = first_length + second_length + 1;
                        goto fn_exit;
                    } else {
                        netloc_node_t ***blossom_edges;
                        int num_blossom_nodes, stem_index, m, n;
                        get_blossom(nodes, num_nodes, edges, num_edges, unmarked_vertices,
                                    unmarked_vertex_insert, &blossom_edges,
                                    &num_blossom_nodes, &stem_index);
                        /* Contract a blossom in G and look for the path in the contracted graph */
                        for (j = stem_index + 1; j < num_blossom_nodes; j++) {
                            netloc_node_t **blossom_node = blossom_edges[j];
                            for (k = 0; k < num_nodes; k++) {
                                if (nodes[k] == blossom_node) {
                                    for (l = k; l < num_nodes - 1; l++) {
                                        nodes[l] = nodes[l + 1];
                                    }
                                    num_nodes--;
                                    k--;
                                }
                            }
                            if (j < num_blossom_nodes - 1) {
                                netloc_node_t **adj_blossom_node = blossom_edges[j];
                                for (k = 0; k < num_edges; k++) {
                                    if (edges[k]->src == blossom_node &&
                                        edges[k]->dest == adj_blossom_node) {
                                        for (l = k; l < num_edges - 1; l++) {
                                            edges[l] = edges[l + 1];
                                        }
                                        num_edges--;
                                        k--;
                                    }
                                }
                            }
                        }
                        find_augmenting_path(topology, nodes, num_nodes, edges, num_edges,
                                             &current_augmenting_path, &path_length,
                                             maximum_matching, num_matching_edges);
                        /* Find the node that connects to the stem */
                        for (j = 0; j < path_length; j++) {
                            if (current_augmenting_path[j]->src == blossom_edges[stem_index]) {
                                break;
                            }
                        }
                        for (k = stem_index + 1; k < num_blossom_nodes; k++) {
                            if (current_augmenting_path[j]->dest == blossom_edges[k]) {
                                break;
                            }
                        }
                        current_augmenting_path =
                            (semicube_edge **) MPL_realloc(current_augmenting_path,
                                                           sizeof(semicube_edge *) * (k -
                                                                                      stem_index
                                                                                      - 1),
                                                           MPL_MEM_OTHER);

                        current_augmenting_path[j]->dest =
                            current_augmenting_path[j + 1]->src = blossom_edges[stem_index + 1];
                        /* Shift edges to make room to lift blossom edges */
                        for (l = j + 1; l < path_length; l++) {
                            current_augmenting_path[l] =
                                current_augmenting_path[l + k - stem_index - 1];
                        }
                        for (l = j; l < (j + k - stem_index - 2); l++) {
                            semicube_edge edge;
                            edge.src = blossom_edges[l];
                            edge.dest = blossom_edges[l + 1];
                            current_augmenting_path[l] = &edge;
                        }
                        MPL_free(blossom_edges);
                        goto fn_exit;
                    }
                }
            }
            /* Mark edge */
            if (unmarked_edge) {
                for (j = unmarked_edge_index; j < unmarked_edge_insert - 1; j++) {
                    unmarked_edges[j] = unmarked_edges[j + 1];
                }
                unmarked_edge_insert--;
            }
        }
        if (exposed_vertex) {
            /* Mark vertex */
            for (j = unmarked_vertex_index; j < unmarked_vertex_insert - 1; j++) {
                unmarked_vertices[j] = unmarked_vertices[j + 1];
            }
            unmarked_vertex_insert--;
        }
    }

  fn_exit:
    if (forest != NULL) {
        MPL_free(forest);
    }
    if (unmarked_vertices != NULL) {
        MPL_free(unmarked_vertices);
    }
    if (unmarked_edges != NULL) {
        MPL_free(unmarked_edges);
    }
    *augmenting_path = current_augmenting_path;
    *augmenting_path_length = path_length;
    return;

  fn_fail:
    goto fn_exit;
}

static void find_maximum_matching(netloc_topology_t topology, netloc_node_t *** nodes,
                                  int num_nodes, semicube_edge ** edges, int num_edges,
                                  semicube_edge *** maximum_matching, int *num_matching_edges)
{
    int i, j;
    semicube_edge **augmenting_path =
        (semicube_edge **) MPL_malloc(sizeof(semicube_edge *), MPL_MEM_OTHER);
    int path_length;
    int insert_location = 0;

    find_augmenting_path(topology, nodes, num_nodes, edges, num_edges, &augmenting_path,
                         &path_length, *maximum_matching, *num_matching_edges);
    if (augmenting_path != NULL && path_length > 0) {
        semicube_edge **matching =
            (semicube_edge **) MPL_malloc(sizeof(semicube_edge *), MPL_MEM_OTHER);
        /* Update the maximum matching */
        for (i = 0; i < *num_matching_edges; i++) {
            semicube_edge *edge = (*maximum_matching)[i];
            int edge_in_both_sets = 0;
            for (j = 0; j < path_length; j++) {
                semicube_edge *augmenting_path_edge = augmenting_path[j];
                if (edge->src == augmenting_path_edge->src &&
                    edge->dest == augmenting_path_edge->dest) {
                    edge_in_both_sets = 1;
                    break;
                }
            }
            if (!edge_in_both_sets) {
                matching =
                    (semicube_edge **) MPL_realloc(matching,
                                                   sizeof(semicube_edge *) * (insert_location + 1),
                                                   MPL_MEM_OTHER);
                matching[insert_location++] = edge;
            }
        }

        for (i = 0; i < path_length; i++) {
            semicube_edge *edge = augmenting_path[i];
            int edge_in_both_sets = 0;
            for (j = 0; j < *num_matching_edges; j++) {
                semicube_edge *augmenting_path_edge = (*maximum_matching)[i];
                if (edge->src == augmenting_path_edge->src &&
                    edge->dest == augmenting_path_edge->dest) {
                    edge_in_both_sets = 1;
                    break;
                }
            }
            if (!edge_in_both_sets) {
                matching =
                    (semicube_edge **) MPL_realloc(matching,
                                                   sizeof(semicube_edge *) * (insert_location + 1),
                                                   MPL_MEM_OTHER);
                matching[insert_location++] = edge;
            }
        }
        find_maximum_matching(topology, nodes, num_nodes, edges, num_edges, &matching,
                              &insert_location);
        *maximum_matching = matching;
        *num_matching_edges = insert_location;
    }

  fn_exit:
    if (augmenting_path != NULL) {
        MPL_free(augmenting_path);
    }
    return;
}

static int get_torus_attributes(netloc_topology_t topology,
                                MPIR_Netloc_network_attributes * network_attr)
{
    netloc_dt_lookup_table_t *nodes;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_node_t *start_node = NULL;
    int mpi_errno = MPI_SUCCESS;
    int num_edges = -1;

    if (network_attr->type == MPIR_NETLOC_NETWORK_TYPE__INVALID) {
        network_attr->type = MPIR_NETLOC_NETWORK_TYPE__TORUS;
        /* Check necessary condition for a torus i.e., outdegree of each node is the same */
        int node_count = 0;
        nodes =
            (netloc_dt_lookup_table_t *) MPL_malloc(sizeof(netloc_dt_lookup_table_t),
                                                    MPL_MEM_OTHER);
        mpi_errno = netloc_get_all_nodes(topology, nodes);
        hti = netloc_dt_lookup_table_iterator_t_construct(*nodes);

        while (!netloc_lookup_table_iterator_at_end(hti)) {
            netloc_node_t *node = (netloc_node_t *) netloc_lookup_table_iterator_next_entry(hti);
            netloc_edge_t **edges;
            int num_edges_per_node;
            if (node == NULL) {
                break;
            }
            netloc_get_all_edges(topology, node, &num_edges_per_node, &edges);
            if (!node_count || num_edges_per_node < num_edges) {
                num_edges = num_edges_per_node;
                start_node = node;
                node_count++;
            }
        }
    }
    if (start_node != NULL && network_attr->type != MPIR_NETLOC_NETWORK_TYPE__INVALID) {
        /* Assuming that hypercube dimension size is less than the bit width of long long */
        unsigned long long *hypercube_labels = NULL;
        int i, j, k, l;
        int start_index, end_index;
        /* Bitmap of non-zero entries in the labels */
        unsigned long long covered_bits;
        int covered_bits_count;
        int index;
        /* Get rid of the extra bidirectional and cycle edges */
        netloc_edge_t **ignore_edges = NULL;
        netloc_node_t **traversal_order = NULL, ***semicube_vertices = NULL;
        semicube_edge **semicube_edges = NULL;
        semicube_edge **complementary_edges = NULL;
        semicube_edge **maximum_matching = NULL;
        int hypercube_dimension = 0;
        unsigned int **distance_matrix;

        end_index = 0;
        start_index = 0;
        traversal_order =
            (netloc_node_t **) MPL_malloc(sizeof(netloc_node_t *) * topology->num_nodes,
                                          MPL_MEM_OTHER);
        MPIR_Assert(end_index < topology->num_nodes);
        traversal_order[end_index++] = start_node;
        /* Compute the depth first order of nodes to associate hamming code */
        start_index = end_index = 0;
        traversal_order[end_index++] = start_node;
        while (start_index < end_index) {
            netloc_node_t *vertex = traversal_order[start_index++];
            int num_edges;
            netloc_edge_t **edges;
            netloc_get_all_edges(topology, vertex, &num_edges, &edges);
            for (i = 0; i < num_edges; i++) {
                int dest_added = 0;
                int node_added_previously = 0;
                for (j = 0; j < end_index; j++) {
                    for (k = 0; k < end_index; k++) {
                        if (edges[i]->dest_node == traversal_order[k]) {
                            node_added_previously = 1;
                            break;
                        }
                    }
                    if (!node_added_previously) {
                        traversal_order[end_index++] = edges[i]->dest_node;
                    }
                }
            }
        }

        MPIR_Assert(end_index == topology->num_nodes);
        distance_matrix =
            (unsigned int **) MPL_calloc(topology->num_nodes * topology->num_nodes,
                                         sizeof(unsigned int *), MPL_MEM_OTHER);
        for (i = 0; i < topology->num_nodes; i++) {
            netloc_node_t *node = topology->nodes[i];
            int num_edges;
            netloc_edge_t **edges;
            netloc_get_all_edges(topology, node, &num_edges, &edges);
            distance_matrix[node->__uid__] =
                (unsigned int *) MPL_calloc(topology->num_nodes, sizeof(unsigned int),
                                            MPL_MEM_OTHER);
            for (j = 0; j < topology->num_nodes; j++) {
                distance_matrix[node->__uid__][topology->nodes[j]->__uid__] = MAX_DISTANCE;
            }
            for (j = 0; j < num_edges; j++) {
                netloc_node_t *neighbor = edges[j]->dest_node;
                distance_matrix[node->__uid__][neighbor->__uid__] = 1;
            }
        }

        /* Compute the transitive closure */
        for (k = 0; k < topology->num_nodes; k++) {
            for (i = 0; i < topology->num_nodes; i++) {
                int first_index = topology->nodes[i]->__uid__;
                for (j = 0; j < topology->num_nodes; j++) {
                    int second_index = topology->nodes[j]->__uid__;
                    int third_index = topology->nodes[k]->__uid__;
                    if (distance_matrix[first_index][second_index] >
                        distance_matrix[first_index][third_index] +
                        distance_matrix[third_index][second_index]) {
                        distance_matrix[first_index][second_index] =
                            distance_matrix[first_index][third_index] +
                            distance_matrix[third_index][second_index];
                    }
                }
            }
        }

        if (start_index == topology->num_nodes) {
            int complementary_edge_index;
            int semicube_vertex_count;
            int *semicube_vertices_index;
            int semicube_edge_count;
            int num_matching_edges;
            netloc_node_t ****path_graphs;
            int *path_graph_count;
            int path_graph_insert;
            int num_nodes_covered;
            int **temp_coordinate_map = NULL;
            long node_index;
            int *coordinates;

            /* Initialize labels with 0 */
            hypercube_labels =
                (unsigned long long *) MPL_calloc(start_index, sizeof(unsigned long long),
                                                  MPL_MEM_OTHER);
            covered_bits = 0;
            covered_bits_count = 0;
            /* Get the isometric hypercube embedding of the input graph */
            for (i = 1; i < start_index; i++) {
                /* Find a neighbor node 1 hop away to the current and try new labels till a feasible label is found */
                netloc_node_t *neighbor = traversal_order[i];
                unsigned long long neighbor_label, bit_flip, new_label;
                netloc_node_t *node;
                int valid_label;
                int num_shifts, total_shifts;
                node = NULL;
                for (j = 0; j < i; j++) {
                    int num_edges;
                    netloc_edge_t **edges;
                    netloc_get_all_edges(topology, traversal_order[j], &num_edges, &edges);
                    node = NULL;
                    for (l = 0; l < num_edges; l++) {
                        if (edges[l]->dest_node == neighbor) {
                            node = traversal_order[j];
                            break;
                        }
                    }
                    if (node != NULL) {
                        break;
                    }
                }
                MPIR_Assert(node != NULL);
                /* Generate a new label at hamming distance 1 from neighbor */
                neighbor_label = hypercube_labels[node->__uid__];
                bit_flip = 1;
                num_shifts = 0;
                valid_label = 0;
                while (num_shifts++ < (sizeof(unsigned long long) * 8)) {
                    int current_valid_label = 1;
                    unsigned long long temp_label = neighbor_label ^ bit_flip;
                    bit_flip = bit_flip << 1;

                    for (j = 0; j < i; j++) {
                        netloc_node_t *prev_node = traversal_order[j];
                        unsigned int distance;
                        unsigned int hamming_distance;
                        /* Check if the new label is consistent with previous labels
                         * with hamming distance */
                        if (temp_label == hypercube_labels[prev_node->__uid__]) {
                            current_valid_label = 0;
                            break;
                        }
                        /* Find the distance of `neighbor` to `prev_node` */
                        distance = distance_matrix[prev_node->__uid__][neighbor->__uid__];
                        hamming_distance =
                            get_hamming_distance(hypercube_labels[prev_node->__uid__], temp_label);
                        if (distance != MAX_DISTANCE && hamming_distance != distance) {
                            current_valid_label = 0;
                            break;
                        }
                    }
                    if (current_valid_label) {
                        valid_label = 1;
                        /* Check if this covers more bits than before */
                        int temp_bit_count;
                        long temp_covered_bits = covered_bits | temp_label;
                        temp_bit_count = 0;
                        while (temp_covered_bits) {
                            if (temp_covered_bits & 1) {
                                temp_bit_count++;
                            }
                            temp_covered_bits = temp_covered_bits / 2;
                        }
                        if (covered_bits_count <= temp_bit_count) {
                            new_label = temp_label;
                            if (covered_bits_count < temp_bit_count) {
                                break;
                            }
                            covered_bits_count = temp_bit_count;
                        }
                    }
                }
                if (!valid_label) {
                    network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
                    goto cleanup;
                }
                hypercube_labels[neighbor->__uid__] = new_label;
                covered_bits = covered_bits | new_label;
            }

            MPL_free(distance_matrix);
            network_attr->type = MPIR_NETLOC_NETWORK_TYPE__TORUS;

            hypercube_dimension = 0;
            index = 0;
            while (covered_bits) {
                int value = covered_bits & 1;
                if (!value) {
                    for (i = 0; i < topology->num_nodes; i++) {
                        unsigned long long label = hypercube_labels[i];
                        if (!index) {
                            label =
                                ((label >> (index + 1)) << index) | (label & ((2 << index) - 1));
                        }
                        hypercube_labels[i] = label;
                    }
                } else {
                    hypercube_dimension++;
                }
                covered_bits = covered_bits >> 1;
                index++;
            }

            semicube_vertices =
                (netloc_node_t ***) MPL_calloc(2 * hypercube_dimension, sizeof(netloc_node_t **),
                                               MPL_MEM_OTHER);

            semicube_vertices_index = (int *) MPL_calloc(2 * hypercube_dimension, sizeof(int),
                                                         MPL_MEM_OTHER);
            semicube_vertex_count = 0;
            /* Compute the semicubes of the hypercube embedding */
            for (j = 0; j < topology->num_nodes; j++) {
                unsigned long long label = hypercube_labels[topology->nodes[j]->__uid__];
                for (i = 0; i < hypercube_dimension; i++) {
                    int semicube_index;
                    int insert_position;
                    if (label & (1u << i)) {
                        semicube_index = 2 * i + 1;
                    } else {
                        semicube_index = 2 * i;
                    }
                    if (semicube_vertices[semicube_index] == NULL) {
                        semicube_vertex_count++;
                        insert_position = 0;
                        semicube_vertices[semicube_index] =
                            (netloc_node_t **) MPL_malloc(sizeof(netloc_node_t *), MPL_MEM_OTHER);
                    } else {
                        insert_position = semicube_vertices_index[semicube_index] + 1;
                        semicube_vertices[semicube_index] =
                            (netloc_node_t **) MPL_realloc(semicube_vertices[semicube_index],
                                                           (insert_position +
                                                            1) * sizeof(netloc_node_t *),
                                                           MPL_MEM_OTHER);
                    }
                    semicube_vertices[semicube_index][insert_position] = topology->nodes[j];
                    semicube_vertices_index[semicube_index] = insert_position;
                }
            }

            /* Compute the semicube graph edges from the semicubes of the hypercube embedding */
            semicube_edges =
                (semicube_edge **) MPL_malloc(hypercube_dimension * 2 * sizeof(semicube_edge *),
                                              MPL_MEM_OTHER);
            MPIR_Assert(semicube_edges != NULL);
            semicube_edge_count = 0;
            for (i = 0; i < hypercube_dimension * 2 - 1; i++) {
                netloc_node_t **first_set = semicube_vertices[i];
                if (first_set != NULL) {
                    for (j = i + 1; j < hypercube_dimension * 2; j++) {
                        netloc_node_t **second_set = semicube_vertices[j];
                        if (semicube_vertices[j] != NULL) {
                            int insert_count = 0;
                            int valid_edge = 0;
                            netloc_node_t **union_set = (netloc_node_t **)
                                MPL_malloc((semicube_vertices_index[i] +
                                            semicube_vertices_index[j] +
                                            2) * sizeof(netloc_node_t *),
                                           MPL_MEM_OTHER);
                            for (k = 0; k <= semicube_vertices_index[i]; k++) {
                                union_set[insert_count++] = first_set[k];
                            }
                            for (k = 0; k <= semicube_vertices_index[j]; k++) {
                                int added_before = 0;
                                for (l = 0; l < insert_count; l++) {
                                    if (union_set[l] == second_set[k]) {
                                        added_before = 1;
                                        valid_edge = 1;
                                        break;
                                    }
                                }
                                if (!added_before) {
                                    union_set[insert_count++] = second_set[k];
                                }
                            }
                            if (valid_edge && insert_count == topology->num_nodes) {
                                semicube_edge *edge =
                                    (semicube_edge *) MPL_malloc(sizeof(semicube_edge *),
                                                                 MPL_MEM_OTHER);
                                edge->src = first_set;
                                edge->dest = second_set;
                                semicube_edges[semicube_edge_count++] = edge;
                            }
                            MPL_free(union_set);
                        }
                    }
                }
            }
            num_matching_edges = 0;
            /* Compute the maximum matching for semicube graph */
            find_maximum_matching(topology, semicube_vertices, semicube_vertex_count,
                                  semicube_edges, semicube_edge_count, &maximum_matching,
                                  &num_matching_edges);
            complementary_edges =
                (semicube_edge **) MPL_malloc(hypercube_dimension * 2 * sizeof(semicube_edge *),
                                              MPL_MEM_OTHER);
            complementary_edge_index = 0;
            /* Compute complementary paths in the graph */
            for (i = 0; i < hypercube_dimension * 2 - 1; i++) {
                netloc_node_t **first_set = semicube_vertices[i];
                if (first_set != NULL) {
                    for (j = i + 1; j < hypercube_dimension * 2; j++) {
                        netloc_node_t **second_set = semicube_vertices[j];
                        if (semicube_vertices[j] != NULL) {
                            int insert_count = 0;
                            int valid_edge = 1;
                            netloc_node_t **union_set = (netloc_node_t **)
                                MPL_malloc(topology->num_nodes * sizeof(netloc_node_t *),
                                           MPL_MEM_OTHER);
                            for (k = 0; k <= semicube_vertices_index[i]; k++) {
                                union_set[insert_count++] = first_set[k];
                            }
                            for (k = 0; k <= semicube_vertices_index[j]; k++) {
                                int added_before = 0;
                                for (l = 0; l < insert_count; l++) {
                                    if (union_set[l] == second_set[k]) {
                                        added_before = 1;
                                        valid_edge = 0;
                                        break;
                                    }
                                }
                                if (!added_before) {
                                    union_set[insert_count++] = second_set[k];
                                }
                            }
                            if (valid_edge && insert_count == topology->num_nodes) {
                                semicube_edge *edge =
                                    (semicube_edge *) MPL_malloc(sizeof(semicube_edge *),
                                                                 MPL_MEM_OTHER);
                                edge->src = first_set;
                                edge->dest = second_set;
                                complementary_edges[complementary_edge_index++] = edge;
                            }
                            MPL_free(union_set);
                        }
                    }
                }
            }

            /* Compute path graphs */
            path_graphs =
                (netloc_node_t ****) MPL_malloc((hypercube_dimension - num_matching_edges) *
                                                sizeof(netloc_node_t ***), MPL_MEM_OTHER);
            path_graph_count =
                MPL_calloc((hypercube_dimension - num_matching_edges), sizeof(int), MPL_MEM_OTHER);
            path_graph_insert = 0;
            num_nodes_covered = 0;
            for (i = 0; i < semicube_vertex_count; i++) {
                netloc_node_t **semicube_vertex = semicube_vertices[i];
                int vertex_covered = 0;
                for (j = 0; j < path_graph_insert; j++) {
                    for (k = 0; k < path_graph_count[j]; k++) {
                        if (path_graphs[j][k] == semicube_vertex) {
                            vertex_covered = 1;
                            break;
                        }
                    }
                    if (vertex_covered) {
                        break;
                    }
                }

                if (!vertex_covered) {
                    int num_edges = 0;
                    for (j = 0; j < num_matching_edges; j++) {
                        if (maximum_matching[j]->src == semicube_vertex ||
                            maximum_matching[j]->dest == semicube_vertex) {
                            num_edges++;
                        }
                    }
                    for (j = 0; j < complementary_edge_index; j++) {
                        if (complementary_edges[j]->src == semicube_vertex ||
                            complementary_edges[j]->dest == semicube_vertex) {
                            num_edges++;
                        }
                    }
                    /* Start from a vertex with 1 incoming/outgoing edge */
                    if (num_edges == 1) {
                        int num_nodes;
                        path_graphs[path_graph_insert] =
                            (netloc_node_t ***) MPL_calloc(semicube_vertex_count,
                                                           sizeof(netloc_node_t **), MPL_MEM_OTHER);
                        num_nodes = 0;
                        path_graphs[path_graph_insert][num_nodes++] = semicube_vertex;
                        MPIR_Assert(path_graph_insert <=
                                    (hypercube_dimension - num_matching_edges));

                        while (semicube_vertex) {
                            netloc_node_t **neighbor = NULL;
                            for (j = 0; j < num_matching_edges; j++) {
                                if (maximum_matching[j]->src == semicube_vertex ||
                                    maximum_matching[j]->dest == semicube_vertex) {
                                    fflush(stdout);
                                    netloc_node_t **temp_neighbor = NULL;
                                    if (maximum_matching[j]->src == semicube_vertex) {
                                        temp_neighbor = maximum_matching[j]->dest;
                                    } else {
                                        temp_neighbor = maximum_matching[j]->src;
                                    }
                                    for (k = 0; k < num_nodes; k++) {
                                        vertex_covered = 0;
                                        if (path_graphs[path_graph_insert][k] == temp_neighbor) {
                                            vertex_covered = 1;
                                            break;
                                        }
                                    }
                                    if (!vertex_covered) {
                                        neighbor = temp_neighbor;
                                        break;
                                    }
                                }
                            }
                            if (!neighbor) {
                                for (j = 0; j < complementary_edge_index; j++) {
                                    if (complementary_edges[j]->src == semicube_vertex ||
                                        complementary_edges[j]->dest == semicube_vertex) {
                                        netloc_node_t **temp_neighbor = NULL;
                                        if (complementary_edges[j]->src == semicube_vertex) {
                                            temp_neighbor = complementary_edges[j]->dest;
                                        } else {
                                            temp_neighbor = complementary_edges[j]->src;
                                        }
                                        for (k = 0; k < num_nodes; k++) {
                                            vertex_covered = 0;
                                            if (path_graphs[path_graph_insert][k] == temp_neighbor) {
                                                vertex_covered = 1;
                                                break;
                                            }
                                        }
                                        if (!vertex_covered) {
                                            neighbor = temp_neighbor;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (neighbor) {
                                path_graphs[path_graph_insert][num_nodes++] = neighbor;
                            }
                            semicube_vertex = neighbor;
                        }
                        path_graph_count[path_graph_insert] = num_nodes;
                        num_nodes_covered += num_nodes;
                        path_graph_insert++;
                    }
                }
            }

            MPIR_Assert(num_nodes_covered == semicube_vertex_count);
            network_attr->u.torus.dimension = path_graph_insert;
            network_attr->u.torus.geometry =
                (int *) MPL_calloc(path_graph_insert, sizeof(int), MPL_MEM_OTHER);
            temp_coordinate_map =
                (int **) MPL_calloc(topology->num_nodes, sizeof(int *), MPL_MEM_OTHER);
            /* Traverse path graphs to assign coordinates of each node and size of torus along each dimension */
            for (i = 0; i < topology->num_nodes; i++) {
                netloc_node_t *vertex = topology->nodes[i];
                temp_coordinate_map[vertex->__uid__] =
                    MPL_calloc(path_graph_insert, sizeof(int), MPL_MEM_OTHER);
                for (j = 0; j < path_graph_insert; j++) {
                    int current_edge = 0;
                    /* Required to keep track of the number of complementary edges */
                    temp_coordinate_map[vertex->__uid__][j] = -1;
                    while (current_edge < (path_graph_count[j] + 1)) {
                        int vertex_in_first_set, vertex_in_second_set;
                        vertex_in_first_set = 0;
                        if (current_edge == 0) {
                            vertex_in_first_set = 1;
                        } else {
                            netloc_node_t **semicube_node = path_graphs[j][current_edge - 1];
                            int semicube_node_index = -1;
                            for (k = 0; k < semicube_vertex_count; k++) {
                                if (semicube_vertices[k] == semicube_node) {
                                    semicube_node_index = k;
                                    break;
                                }
                            }
                            for (k = 0; k <= semicube_vertices_index[semicube_node_index]; k++) {
                                if (vertex == semicube_node[k]) {
                                    vertex_in_first_set = 1;
                                    break;
                                }
                            }
                        }
                        if (!vertex_in_first_set) {
                            current_edge++;
                            continue;
                        }

                        vertex_in_second_set = 0;
                        if (current_edge == path_graph_count[j]) {
                            vertex_in_second_set = 1;
                        } else {
                            netloc_node_t **semicube_node = path_graphs[j][current_edge];
                            int semicube_node_index = -1;
                            for (k = 0; k < semicube_vertex_count; k++) {
                                if (semicube_vertices[k] == semicube_node) {
                                    semicube_node_index = k;
                                    break;
                                }
                            }
                            for (k = 0; k <= semicube_vertices_index[semicube_node_index]; k++) {
                                if (vertex == semicube_node[k]) {
                                    vertex_in_second_set = 1;
                                    break;
                                }
                            }
                        }
                        if (vertex_in_second_set) {
                            /* Current edge counter has to be odd */
                            MPIR_Assert(current_edge & 1);
                            temp_coordinate_map[vertex->__uid__][j] = (current_edge + 1) / 2;
                            if (network_attr->u.torus.geometry[j] <
                                (temp_coordinate_map[vertex->__uid__][j] + 1)) {
                                network_attr->u.torus.geometry[j] =
                                    temp_coordinate_map[vertex->__uid__][j] + 1;
                            }
                            break;
                        }
                        current_edge++;
                    }
                    MPIR_Assert(temp_coordinate_map[vertex->__uid__][j] > -1);
                }
            }

            /* Identify the network node corresponding to the current rank's node */
            errno = MPIR_Netloc_get_network_end_point(MPIR_Process.network_attr,
                                                      MPIR_Process.netloc_topology,
                                                      MPIR_Process.hwloc_topology,
                                                      &MPIR_Process.network_attr.network_endpoint);
            if (errno != MPI_SUCCESS) {
                network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
            } else {
                /* Flatten computed coordinates into a long value for the current node */
                coordinates =
                    temp_coordinate_map[MPIR_Process.network_attr.network_endpoint->__uid__];
                node_index = coordinates[0];
                for (j = 1; j < path_graph_insert; j++) {
                    node_index = (node_index * network_attr->u.torus.geometry[j]) + coordinates[j];
                }
                network_attr->u.torus.node_idx = index;
            }
            MPL_free(temp_coordinate_map);
            MPL_free(path_graph_count);
            MPL_free(path_graphs);
        } else {
            network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
        }

      cleanup:
        if (traversal_order != NULL) {
            MPL_free(traversal_order);
        }
        if (hypercube_labels != NULL) {
            MPL_free(hypercube_labels);
        }
        if (semicube_edges != NULL) {
            MPL_free(semicube_edges);
        }
        if (semicube_vertices != NULL) {
            MPL_free(semicube_vertices);
        }
        if (maximum_matching != NULL) {
            MPL_free(maximum_matching);
        }
    } else {
        network_attr->type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
    }

  fn_exit:
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

    if (network_attr->type == MPIR_NETLOC_NETWORK_TYPE__INVALID) {
        mpi_errno = get_torus_attributes(MPIR_Process.netloc_topology, network_attr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

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
