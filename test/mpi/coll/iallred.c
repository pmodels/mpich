/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run coll_iallred
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Request request;
    int size, rank;
    int one = 1, two = 2, isum, sum;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    assert(size == 2);
    MPI_Iallreduce(&one, &isum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &request);
    MPI_Allreduce(&two, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Wait(&request, MPI_STATUS_IGNORE);

    assert(isum == 2);
    assert(sum == 4);

    return 0;
}
