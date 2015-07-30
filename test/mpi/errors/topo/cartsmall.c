/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int dims[2];
    int periods[2];
    int size, rank;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    dims[0] = size;
    dims[1] = size;
    periods[0] = 0;
    periods[1] = 0;

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    err = MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &comm);
    if (err == MPI_SUCCESS) {
        errs++;
        printf("Cart_create returned success when dims > size\n");
    }
    else if (comm != MPI_COMM_NULL) {
        errs++;
        printf("Expected a null comm from cart create\n");
    }

    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}
