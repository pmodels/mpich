/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
/* This test borrows some ideas from a test provided by Rolf
 * Rabenseifner (HLRS), but is a rewrite of the primary logic to make
 * it more suitable for the MPICH test suite. */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MTest_Init(&argc, &argv);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size > 2) {
        fprintf(stderr, "run this test with 1 or 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int dims[1] = { size };
    int periods[1] = { 1 };
    int reorder = 1;

    MPI_Comm newcomm;
    MPI_Cart_create(MPI_COMM_WORLD, 1, dims, periods, reorder, &newcomm);

    int sbuf[2], rbuf[2] = { 0 };
    sbuf[0] = -1;
    sbuf[1] = 1;
    MPI_Neighbor_alltoall(sbuf, 1, MPI_INT, rbuf, 1, MPI_INT, newcomm);

    MPI_Comm_free(&newcomm);

    int errs = 0;
    if (rbuf[0] != 1 || rbuf[1] != -1) {
        printf("expected rbuf to be (1,-1), but found (%d,%d)\n", rbuf[0], rbuf[1]);
        errs++;
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
