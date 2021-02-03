/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ialltoallv_tsp_inplace_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ialltoallv_intra_gentran_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                          const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                          void *recvbuf, const MPI_Aint recvcounts[],
                                          const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ialltoallv_intra_inplace(sendbuf, sendcounts, sdispls, sendtype,
                                                      recvbuf, recvcounts, rdispls, recvtype,
                                                      comm, req);

    return mpi_errno;
}
