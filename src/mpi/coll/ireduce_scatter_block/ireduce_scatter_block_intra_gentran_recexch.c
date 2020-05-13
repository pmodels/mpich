/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "ireduce_scatter_block_tsp_recexch_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Ireduce_scatter_block_intra_gentran_recexch(const void *sendbuf, void *recvbuf,
                                                     int recvcount, MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm, int k,
                                                     MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Ireduce_scatter_block_intra_recexch(sendbuf, recvbuf, recvcount,
                                                                 datatype, op, comm, req, k);

    return mpi_errno;
}
