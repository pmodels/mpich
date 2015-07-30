/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* This is a simple test program to see how well balanced the output from
   MPI_Dims_create is.  This test can be run with a single process
   and does not need mpiexec.

   There is no standard result.  Instead, the output should be examined to
   see if the results are acceptable.  For example, make sure that Dims_create
   of 2*2*2 in three dimensions returns 2,2,2 (and not 1,1,8).
 */

#include "mpi.h"
#include <stdio.h>

static int verbose = 1;

int main(int argc, char *argv[])
{
    int i, j, dims[3], size;

    MPI_Init(&argc, &argv);

    for (i = 2; i < 12; i++) {
        for (j = 0; j < 2; j++)
            dims[j] = 0;
        MPI_Dims_create(i * i, 2, dims);
        if (verbose)
            printf("Dims_create(%d,2) = (%d,%d)\n", i * i, dims[0], dims[1]);
        for (j = 0; j < 3; j++)
            dims[j] = 0;
        MPI_Dims_create(i * i * i, 3, dims);
        if (verbose)
            printf("Dims_create(%d,3) = (%d,%d,%d)\n", i * i * i, dims[0], dims[1], dims[2]);
    }
    size = 2 * 5 * 7 * 11;
    for (j = 0; j < 3; j++)
        dims[j] = 0;
    MPI_Dims_create(size, 2, dims);
    if (verbose)
        printf("Dims_create(%d,2) = (%d,%d)\n", size, dims[0], dims[1]);
    for (j = 0; j < 3; j++)
        dims[j] = 0;
    MPI_Dims_create(size, 3, dims);
    if (verbose)
        printf("Dims_create(%d,3) = (%d,%d,%d)\n", size, dims[0], dims[1], dims[2]);
    size = 5 * 5 * 2 * 7 * 7 * 7;
    for (j = 0; j < 3; j++)
        dims[j] = 0;
    MPI_Dims_create(size, 2, dims);
    if (verbose)
        printf("Dims_create(%d,2) = (%d,%d)\n", size, dims[0], dims[1]);
    for (j = 0; j < 3; j++)
        dims[j] = 0;
    MPI_Dims_create(size, 3, dims);
    if (verbose)
        printf("Dims_create(%d,3) = (%d,%d,%d)\n", size, dims[0], dims[1], dims[2]);

    size = 12;
    for (j = 0; j < 3; j++)
        dims[j] = 0;
    MPI_Dims_create(size, 2, dims);
    if (verbose)
        printf("Dims_create(%d,2) = (%d,%d)\n", size, dims[0], dims[1]);

    MPI_Finalize();

    return 0;
}
