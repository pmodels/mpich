/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Regression test for ticket #1785, contributed by Jed Brown.  The test was
 * hanging indefinitely under a buggy version of ch3:sock. */

#include "mpitest.h"
#include <unistd.h>

#ifdef MULTI_TESTS
#define run coll_ibarrier
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Request barrier;
    int rank, i, done;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Ibarrier(MPI_COMM_WORLD, &barrier);
    for (i = 0, done = 0; !done; i++) {
        usleep(1000);
        /*printf("[%d] MPI_Test: %d\n",rank,i); */
        MPI_Test(&barrier, &done, MPI_STATUS_IGNORE);
    }

    return 0;
}
