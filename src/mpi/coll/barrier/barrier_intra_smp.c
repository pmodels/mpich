/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Barrier_intra_smp(MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr));

    /* do the intranode barrier on all nodes */
    if (comm_ptr->node_comm != NULL) {
        MPIR_COLL_INNER(Barrier, coll_sig, comm_ptr->node_comm);
    }

    /* do the barrier across roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_COLL_INNER(Barrier, coll_sig, comm_ptr->node_roots_comm);
    }

    /* release the local processes on each node with a 1-byte
     * broadcast (0-byte broadcast just returns without doing
     * anything) */
    if (comm_ptr->node_comm != NULL) {
        int i = 0;
        MPIR_COLL_INNER(Bcast, coll_sig, &i, 1, MPIR_BYTE_INTERNAL, 0, comm_ptr->node_comm);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
