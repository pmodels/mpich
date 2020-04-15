/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* generate gentran algo prototypes */
#include "tsp_gentran.h"
#include "iscan_tsp_recursive_doubling_algos_prototypes.h"
#include "tsp_undef.h"

int MPIR_Iscan_intra_gentran_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPII_Gentran_Iscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                            comm, req);

    return mpi_errno;
}
