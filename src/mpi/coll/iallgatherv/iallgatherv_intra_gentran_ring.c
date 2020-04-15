/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iallgatherv_tsp_ring_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iallgatherv_intra_gentran_ring(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iallgatherv_intra_ring(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcounts, displs, recvtype, comm,
                                                    req);

    return mpi_errno;
}
