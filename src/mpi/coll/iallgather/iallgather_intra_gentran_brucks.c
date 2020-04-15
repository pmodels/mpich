/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iallgather_tsp_brucks_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iallgather_intra_gentran_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr, int k, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iallgather_intra_brucks(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype,
                                                     comm_ptr, request, k);

    return mpi_errno;
}
