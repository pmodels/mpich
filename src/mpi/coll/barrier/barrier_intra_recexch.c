/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


/* Algorithm: call Allreduce's recursive exchange algorithm
 */
int MPIR_Barrier_intra_recexch(MPIR_Comm * comm, int k, int single_phase_recv, int collattr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_intra_recexch(MPI_IN_PLACE, NULL, 0,
                                             MPI_BYTE, MPI_SUM, comm,
                                             k, single_phase_recv, collattr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
