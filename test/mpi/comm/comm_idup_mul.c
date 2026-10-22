/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * Test creating multiple communicators with MPI_Comm_idup.
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_comm_idup_mul
int run(const char *arg);
#endif

#define NUM_ITER    10

int run(const char *arg)
{
    int i, rank;
    MPI_Comm comms[NUM_ITER];
    MPI_Request req[NUM_ITER];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 0; i < NUM_ITER; i++)
        MPI_Comm_idup(MPI_COMM_WORLD, &comms[i], &req[i]);

    MPI_Waitall(NUM_ITER, req, MPI_STATUSES_IGNORE);

    for (i = 0; i < NUM_ITER; i++)
        MPI_Comm_free(&comms[i]);

    return 0;
}
