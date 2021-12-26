/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_manyrma3
int run(const char *arg);
#endif

#define MAX_COUNT 4096

int run(const char *arg)
{
    int i, winbuf, one = 1, rank;
    MPI_Win win;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Win_create(&winbuf, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_fence(0, win);
    for (i = 0; i < MAX_COUNT; i++)
        MPI_Accumulate(&one, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_SUM, win);
    MPI_Win_fence(0, win);

    MPI_Win_free(&win);

    return 0;
}
