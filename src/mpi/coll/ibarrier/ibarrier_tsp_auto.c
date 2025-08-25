/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* sched version of CVAR and json based collective selection. Meant only for gentran scheduler */
int MPIR_TSP_Ibarrier_sched_intra_tsp_auto(MPIR_Comm * comm, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch(MPI_IN_PLACE, NULL, 0,
                                                        MPIR_BYTE_INTERNAL, MPI_SUM, comm, 0, 2,
                                                        sched);

    return mpi_errno;
}
