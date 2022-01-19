/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Routine to schedule a disdem based barrier with radix k */
int MPIR_TSP_Ibarrier_sched_intra_recexch(MPIR_Comm * comm, int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    void *recvbuf = NULL;
    MPIR_FUNC_ENTER;

    mpi_errno =
        MPIR_TSP_Iallreduce_sched_intra_recexch(MPI_IN_PLACE, recvbuf, 0, MPI_BYTE, MPI_SUM,
                                                comm,
                                                MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                k, sched);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
