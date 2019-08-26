/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"
#ifdef HAVE_NETLOC
#include "netloc_util.h"
#endif

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

void init_topo(void)
{
#ifdef HAVE_HWLOC
    MPIR_Process.bindset = hwloc_bitmap_alloc();
    hwloc_topology_init(&MPIR_Process.hwloc_topology);
    MPIR_Process.bindset_is_valid = 0;
    hwloc_topology_set_io_types_filter(MPIR_Process.hwloc_topology, HWLOC_TYPE_FILTER_KEEP_ALL);
    if (!hwloc_topology_load(MPIR_Process.hwloc_topology)) {
        MPIR_Process.bindset_is_valid =
            !hwloc_get_proc_cpubind(MPIR_Process.hwloc_topology, getpid(), MPIR_Process.bindset,
                                    HWLOC_CPUBIND_PROCESS);
    }
#endif

#ifdef HAVE_NETLOC
    MPIR_Process.network_attr.u.tree.node_levels = NULL;
    MPIR_Process.network_attr.network_endpoint = NULL;
    MPIR_Process.netloc_topology = NULL;
    MPIR_Process.network_attr.type = MPIR_NETLOC_NETWORK_TYPE__INVALID;
    if (strlen(MPIR_CVAR_NETLOC_NODE_FILE)) {
        mpi_errno =
            netloc_parse_topology(&MPIR_Process.netloc_topology, MPIR_CVAR_NETLOC_NODE_FILE);
        if (mpi_errno == NETLOC_SUCCESS) {
            MPIR_Netloc_parse_topology(MPIR_Process.netloc_topology, &MPIR_Process.network_attr);
        }
    }
#endif
}

void finalize_topo(void)
{
#ifdef HAVE_HWLOC
    hwloc_topology_destroy(MPIR_Process.hwloc_topology);
    hwloc_bitmap_free(MPIR_Process.bindset);
#endif

#ifdef HAVE_NETLOC
    switch (MPIR_Process.network_attr.type) {
        case MPIR_NETLOC_NETWORK_TYPE__TORUS:
            MPL_free(MPIR_Process.network_attr.u.torus.geometry);
            break;
        case MPIR_NETLOC_NETWORK_TYPE__FAT_TREE:
        case MPIR_NETLOC_NETWORK_TYPE__CLOS_NETWORK:
        default:
            MPL_free(MPIR_Process.network_attr.u.tree.node_levels);
            break;
    }
#endif
}
