/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CIRC_GRAPH_Q_LEN
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Determines how many messages can be in flight at once for the circ_graph algorithm

    - name        : MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 131072
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the chunk size (in bytes) for pipeline data transfer.

    - name        : MPIR_CVAR_CIRC_GRAPH_NUM_CHUNKS
      category    : COLLECTIVE
      type        : int
      default     : 32
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the number of chunk buffers for pipeline data transfer.

    - name        : MPIR_CVAR_CIRC_GRAPH_MAX_CHUNKS
      category    : COLLECTIVE
      type        : int
      default     : 1024
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Specifies the max number of chunk buffers to be reserved for pipeline data transfer.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "circ_graph.h"

int MPIR_cga_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}

int MPIR_cga_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}
