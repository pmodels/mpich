/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_allredmany
int run(const char *arg);
#endif

/*
 * This example should be run with 2 processes and tests the ability of the
 * implementation to handle a flood of one-way messages.
 */

int run(const char *arg)
{
    double wscale = 10.0, scale;
    int numprocs, myid, i;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    for (i = 0; i < 10000; i++) {
        MPI_Allreduce(&wscale, &scale, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    }

    return 0;
}
