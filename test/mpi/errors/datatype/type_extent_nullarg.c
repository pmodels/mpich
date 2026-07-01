/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Datatype type;
    int errclass, errs = 0, mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* Checking type_extent for NULL variable */
    type = MPI_INT;
    mpi_errno = MPI_Type_extent(type, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MTest_Finalize(errs);
    return 0;
}
