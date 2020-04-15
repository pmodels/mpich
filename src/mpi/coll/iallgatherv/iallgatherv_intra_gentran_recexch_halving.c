/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iallgatherv_tsp_recexch_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iallgatherv_intra_gentran_recexch_halving(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts, const int *displs,
                                                   MPI_Datatype recvtype, MPIR_Comm * comm, int k,
                                                   MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iallgatherv_intra_recexch(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcounts, displs, recvtype,
                                                       comm, req,
                                                       MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_HALVING,
                                                       k);

    return mpi_errno;
}
