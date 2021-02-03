/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ialltoallw_tsp_inplace_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ialltoallw_intra_gentran_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                          const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                          void *recvbuf, const MPI_Aint recvcounts[],
                                          const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ialltoallw_intra_inplace(sendbuf, sendcounts, sdispls, sendtypes,
                                                      recvbuf, recvcounts, rdispls, recvtypes,
                                                      comm, req);

    return mpi_errno;
}
