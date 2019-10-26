/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"

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

void MPII_init_topo(void)
{
    MPII_hw_topo_init();
}

void MPII_finalize_topo(void)
{
    MPII_hw_topo_finalize();
}
