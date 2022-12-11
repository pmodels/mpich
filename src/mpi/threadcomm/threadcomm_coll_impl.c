/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifdef ENABLE_THREADCOMM

int MPIR_Threadcomm_allreduce_impl(const void *sendbuf, void *recvbuf,
                                   MPI_Aint count, MPI_Datatype datatype,
                                   MPI_Op op, MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                        comm, MPIR_ERR_NONE);

    return mpi_errno;
}

#endif
