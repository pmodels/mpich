/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ialltoallv_tsp_scattered_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ialltoallv_intra_gentran_scattered(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], MPI_Datatype sendtype,
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], MPI_Datatype recvtype,
                                            MPIR_Comm * comm, int batch_size, int bblock,
                                            MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ialltoallv_intra_scattered(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm, batch_size, bblock, req);

    return mpi_errno;
}
