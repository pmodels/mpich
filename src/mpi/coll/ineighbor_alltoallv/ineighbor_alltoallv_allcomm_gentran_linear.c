/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ineighbor_alltoallv_tsp_linear_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ineighbor_alltoallv_allcomm_gentran_linear(const void *sendbuf, const int sendcounts[],
                                                    const int sdispls[], MPI_Datatype sendtype,
                                                    void *recvbuf, const int recvcounts[],
                                                    const int rdispls[], MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPII_Gentran_Ineighbor_alltoallv_allcomm_linear(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm_ptr, request);

    return mpi_errno;
}
