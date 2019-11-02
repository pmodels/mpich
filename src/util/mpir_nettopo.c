/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_NETLOC_NODE_FILE
      category    : DEBUGGER
      type        : string
      default     : auto
      class       : device
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

int MPII_nettopo_init(void)
{
}

int MPII_nettopo_finalize(void)
{
}

MPIR_nettopo_type_e MPIR_nettopo_get_type(void)
{
}

MPIR_nettopo_node_type_e MPIR_nettopo_get_node_type(MPIR_nettopo_node_t node)
{
}

MPIR_nettopo_node_t MPIR_nettopo_get_endpoint(void)
{
}

MPIR_nettopo_node_t MPIR_nettopo_get_edge_dest_node(MPIR_nettopo_edge_t edge)
{
}

int MPIR_nettopo_get_node_uid(MPIR_nettopo_node_t node)
{
}

int MPIR_nettopo_get_num_nodes(void)
{
}

int MPIR_nettopo_get_all_edges(MPIR_nettopo_node_t node, int *num_edges,
                               MPIR_nettopo_edge_t ** edges)
{
}

int *MPIR_nettopo_tree_get_node_levels(void)
{
}

int MPIR_nettopo_tree_get_hostnode_index(int *node_index, int *num_nodes)
{
}

int MPIR_nettopo_tree_get_switches_at_level(int switch_level,
                                            MPIR_nettopo_node_t ** switches_at_level,
                                            int *switch_count)
{
}

int MPIR_nettopo_torus_get_dimension(void)
{
}

int *MPIR_nettopo_torus_get_geometry(void)
{
}

int MPIR_nettopo_torus_get_node_index(void)
{
}
