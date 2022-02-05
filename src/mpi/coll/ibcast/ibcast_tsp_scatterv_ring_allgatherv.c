/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a scatter followed by ring based broadcast */
int MPIR_TSP_Ibcast_sched_intra_scatterv_ring_allgatherv(void *buffer, MPI_Aint count,
                                                         MPI_Datatype datatype, int root,
                                                         MPIR_Comm * comm, int scatterv_k,
                                                         MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype, root,
                                                                comm,
                                                                MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_tsp_ring,
                                                                scatterv_k, 0, sched);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}
