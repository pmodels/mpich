/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_NETLOC_NODE_FILE
      category    : DEBUGGER
      type        : string
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Subnet json file

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"

#ifdef HAVE_NETLOC
#include "mpir_netloc.h"
#endif

#ifdef HAVE_NETLOC
static netloc_topology_t netloc_topology;
static MPIR_Netloc_network_attributes network_attr;
#endif

#ifdef HAVE_NETLOC
static MPIR_nettopo_type_e get_nettopo_type(netloc_node_type_t type)
{
    MPIR_nettopo_type_e ret;

    switch (type) {
        case NETLOC_NODE_TYPE_HOST:
            ret = MPIR_NETTOPO_NODE_TYPE__HOST;
            break;
        case NETLOC_NODE_TYPE_SWITCH:
            ret = MPIR_NETTOPO_NODE_TYPE__SWITCH;
            break;
        case NETLOC_NODE_TYPE_INVALID:
            ret = MPIR_NETTOPO_NODE_TYPE__INVALID;
            break;
        default:
            ret = -1;
    }

    return ret;
}
#endif

int MPII_nettopo_init(void)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_NETLOC
    network_attr.u.tree.node_levels = NULL;
    network_attr.network_endpoint = NULL;
    netloc_topology = NULL;
    network_attr.type = MPIR_NETTOPO_TYPE__INVALID;
    if (strlen(MPIR_CVAR_NETLOC_NODE_FILE)) {
        mpi_errno = netloc_parse_topology(&netloc_topology, MPIR_CVAR_NETLOC_NODE_FILE);
        if (mpi_errno == NETLOC_SUCCESS)
            MPIR_Netloc_parse_topology(hwloc_topology, netloc_topology, &network_attr);
    }
#endif

    return mpi_errno;
}

int MPII_nettopo_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef HAVE_NETLOC
    switch (network_attr.type) {
        case MPIR_NETTOPO_TYPE__TORUS:
            if (network_attr.u.torus.geometry != NULL)
                MPL_free(network_attr.u.torus.geometry);
            break;
        case MPIR_NETTOPO_TYPE__FAT_TREE:
        case MPIR_NETTOPO_TYPE__CLOS_NETWORK:
        default:
            if (network_attr.u.tree.node_levels != NULL)
                MPL_free(network_attr.u.tree.node_levels);
            break;
    }
#endif

    return mpi_errno;
}

MPIR_nettopo_type_e MPIR_nettopo_get_type(void)
{
    MPIR_nettopo_type_e type = MPIR_NETTOPO_TYPE__INVALID;

#ifdef HAVE_NETLOC
    type = network_attr.type;
#endif

    return type;
}

MPIR_nettopo_node_type_e MPIR_nettopo_get_node_type(MPIR_nettopo_node_t node)
{
    MPIR_nettopo_node_type_e type = MPIR_NETTOPO_NODE_TYPE__INVALID;

    if (node == NULL)
        return type;

#ifdef HAVE_NETLOC
    type = get_nettopo_type(((netloc_node_t *) node)->node_type);
#endif

    return type;
}

MPIR_nettopo_node_t MPIR_nettopo_get_endpoint(void)
{
    MPIR_nettopo_node_t node = NULL;

#ifdef HAVE_NETLOC
    node = network_attr.network_endpoint;
#endif

    return node;
}

MPIR_nettopo_node_t MPIR_nettopo_get_edge_dest_node(MPIR_nettopo_edge_t edge)
{
    MPIR_nettopo_node_t node = NULL;

    if (edge == NULL)
        return node;

#ifdef HAVE_NETLOC
    node = ((netloc_edge_t *) edge)->dest_node;
#endif

    return node;
}

int MPIR_nettopo_get_node_uid(MPIR_nettopo_node_t node)
{
    int uid = -1;

    if (node == NULL)
        return uid;

#ifdef HAVE_NETLOC
    uid = ((netloc_node_t *) node)->__uid__;
#endif

    return uid;
}

int MPIR_nettopo_get_num_nodes(void)
{
    int nodes = 0;

#ifdef HAVE_NETLOC
    nodes = netloc_topology->num_nodes;
#endif

    return nodes;
}

int MPIR_nettopo_get_all_edges(MPIR_nettopo_node_t node, int *num_edges,
                               MPIR_nettopo_edge_t ** edges)
{
    int mpi_errno = MPI_SUCCESS;

    *num_edges = 0;
    *edges = NULL;

    if (node == NULL)
        return MPI_ERR_OTHER;

#ifdef HAVE_NETLOC
    mpi_errno = netloc_get_all_edges(netloc_topology, node, num_edges, (netloc_edge_t ***) edges);
#endif

    return mpi_errno;
}

int *MPIR_nettopo_tree_get_node_levels(void)
{
    int *levels = NULL;

#ifdef HAVE_NETLOC
    levels = network_attr.u.tree.node_levels;
#endif

    return levels;
}

int MPIR_nettopo_tree_get_hostnode_index(int *node_index, int *num_nodes)
{
    int mpi_errno = MPI_SUCCESS;

    *node_index = 0;
    *num_nodes = 0;

#ifdef HAVE_NETLOC
    mpi_errno =
        MPIR_Netloc_get_hostnode_index_in_tree(network_attr, netloc_topology,
                                               network_attr.network_endpoint, node_index,
                                               num_nodes);
#endif

    return mpi_errno;
}

int MPIR_nettopo_tree_get_switches_at_level(int switch_level,
                                            MPIR_nettopo_node_t ** switches_at_level,
                                            int *switch_count)
{
    int mpi_errno = MPI_SUCCESS;

    *switches_at_level = NULL;
    *switch_count = 0;

#ifdef HAVE_NETLOC
    mpi_errno =
        MPIR_Netloc_get_switches_at_level(netloc_topology, network_attr,
                                          switch_level, (netloc_node_t ***) switches_at_level,
                                          switch_count);
#endif

    return mpi_errno;
}

int MPIR_nettopo_torus_get_dimension(void)
{
    int dim = 0;

#ifdef HAVE_NETLOC
    dim = network_attr.u.torus.dimension;
#endif

    return dim;
}

int *MPIR_nettopo_torus_get_geometry(void)
{
    int *geometry = NULL;

#ifdef HAVE_NETLOC
    geometry = network_attr.u.torus.geometry;
#endif

    return geometry;
}

int MPIR_nettopo_torus_get_node_index(void)
{
    int idx = 0;

#ifdef HAVE_NETLOC
    idx = network_attr.u.torus.node_index;
#endif

    return idx;
}
