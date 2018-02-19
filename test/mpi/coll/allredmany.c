/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/*
 * This example should be run with 2 processes and tests the ability of the
 * implementation to handle a flood of one-way messages.
 */

int main(int argc, char **argv)
{
    double wscale = 10.0, scale;
    int numprocs, myid, i;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    for (i = 0; i < 10000; i++) {
        MPI_Allreduce(&wscale, &scale, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    }

    MTest_Finalize(0);

    return 0;
}
