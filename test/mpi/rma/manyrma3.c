/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#define MAX_COUNT 4096

int main(int argc, char *argv[])
{
    int i, winbuf, one = 1, rank;
    MPI_Win win;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Win_create(&winbuf, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_fence(0, win);
    for (i = 0; i < MAX_COUNT; i++)
        MPI_Accumulate(&one, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_SUM, win);
    MPI_Win_fence(0, win);

    MPI_Win_free(&win);

    MTest_Finalize(0);

    return 0;
}
