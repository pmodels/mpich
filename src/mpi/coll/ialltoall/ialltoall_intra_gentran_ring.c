/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ialltoall_tsp_ring_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ialltoall_intra_gentran_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ialltoall_intra_ring(sendbuf, sendcount, sendtype,
                                                  recvbuf, recvcount, recvtype, comm, req);

    return mpi_errno;
}
