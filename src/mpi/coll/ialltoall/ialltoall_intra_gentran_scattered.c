/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ialltoall_tsp_scattered_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ialltoall_intra_gentran_scattered(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           int recvcount,
                                           MPI_Datatype recvtype, MPIR_Comm * comm, int batch_size,
                                           int bblock, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ialltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm,
                                                       batch_size, bblock, req);

    return mpi_errno;
}
