/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Barrier_intra_smp(MPIR_Comm * comm_ptr, int coll_group, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr, coll_group));
    int local_rank = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].rank;
    int local_size = comm_ptr->subgroups[MPIR_SUBGROUP_NODE].size;

    /* do the intranode barrier on all nodes */
    if (local_size > 1) {
        mpi_errno = MPIR_Barrier(comm_ptr, MPIR_SUBGROUP_NODE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* do the barrier across roots of all nodes */
    if (local_rank == 0) {
        mpi_errno = MPIR_Barrier(comm_ptr, MPIR_SUBGROUP_NODE_CROSS, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* release the local processes on each node with a 1-byte
     * broadcast (0-byte broadcast just returns without doing
     * anything) */
    if (local_size > 1) {
        int i = 0;
        mpi_errno = MPIR_Bcast(&i, 1, MPI_BYTE, 0, comm_ptr, MPIR_SUBGROUP_NODE, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
