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

#define NUM_ITER    10

int main(int argc, char **argv)
{
    int i, rank;
    MPI_Comm comms[NUM_ITER];
    MPI_Request req[NUM_ITER];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 0; i < NUM_ITER; i++)
        MPI_Comm_idup(MPI_COMM_WORLD, &comms[i], &req[i]);

    MPI_Waitall(NUM_ITER, req, MPI_STATUSES_IGNORE);

    for (i = 0; i < NUM_ITER; i++)
        MPI_Comm_free(&comms[i]);

    if (rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
