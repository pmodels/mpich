/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Send_stream_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int tag, MPIR_Comm * comm_ptr, void *stream)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int MPIR_Recv_stream_impl(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int source, int tag, MPIR_Comm * comm_ptr, MPI_Status * status,
                          void *stream)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}
