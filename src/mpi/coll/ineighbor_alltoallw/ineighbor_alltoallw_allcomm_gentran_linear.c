/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ineighbor_alltoallw_tsp_linear_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ineighbor_alltoallw_allcomm_gentran_linear(const void *sendbuf, const int sendcounts[],
                                                    const MPI_Aint sdispls[],
                                                    const MPI_Datatype sendtypes[], void *recvbuf,
                                                    const int recvcounts[],
                                                    const MPI_Aint rdispls[],
                                                    const MPI_Datatype recvtypes[],
                                                    MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPII_Gentran_Ineighbor_alltoallw_allcomm_linear(sendbuf, sendcounts, sdispls, sendtypes,
                                                        recvbuf, recvcounts, rdispls, recvtypes,
                                                        comm_ptr, request);

    return mpi_errno;
}
