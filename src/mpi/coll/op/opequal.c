/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

void MPIR_EQUAL(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    /* to be implemented */
    MPIR_Assert(0);
}

int MPIR_EQUAL_check_dtype(MPI_Datatype type)
{
    return MPI_SUCCESS;
}

int MPIR_Reduce_equal(const void *sendbuf, MPI_Aint count, MPI_Datatype datatype,
                      int *is_equal, int root, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}


int MPIR_Allreduce_equal(const void *sendbuf, MPI_Aint count, MPI_Datatype datatype,
                         int *is_equal, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}
