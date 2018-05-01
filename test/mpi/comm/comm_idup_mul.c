/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Test creating multiple communicators with MPI_Comm_idup.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

#define NUM_ITER    10

int main(int argc, char **argv)
{
    int i, rank;
    MPI_Comm comms[NUM_ITER];
    MPI_Request req[NUM_ITER];

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 0; i < NUM_ITER; i++)
        MPI_Comm_idup(MPI_COMM_WORLD, &comms[i], &req[i]);

    MPI_Waitall(NUM_ITER, req, MPI_STATUSES_IGNORE);

    for (i = 0; i < NUM_ITER; i++)
        MPI_Comm_free(&comms[i]);

    MTest_Finalize(0);

    return 0;
}
