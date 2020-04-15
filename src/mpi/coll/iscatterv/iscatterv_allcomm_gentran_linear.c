/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iscatterv_tsp_linear_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iscatterv_allcomm_gentran_linear(const void *sendbuf, const int sendcounts[],
                                          const int displs[], MPI_Datatype sendtype, void *recvbuf,
                                          int recvcount, MPI_Datatype recvtype, int root,
                                          MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iscatterv_allcomm_linear(sendbuf, sendcounts, displs,
                                                      sendtype, recvbuf, recvcount,
                                                      recvtype, root, comm_ptr, request);

    return mpi_errno;
}
