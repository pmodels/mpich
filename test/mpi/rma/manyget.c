/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_manyget
int run(const char *arg);
#endif

#define BUFSIZE (128*1024)

int run(const char *arg)
{
    int i, rank, size;
    int *buf;
    MPI_Win win;

    buf = malloc(BUFSIZE);
    MTEST_VG_MEM_INIT(buf, BUFSIZE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        printf("test must be run with 2 processes!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0)
        MPI_Win_create(buf, BUFSIZE, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    else
        MPI_Win_create(MPI_BOTTOM, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_fence(0, win);

    if (rank == 1) {
        for (i = 0; i < 100000; i++)
            MPI_Get(buf, BUFSIZE / sizeof(int), MPI_INT, 0, 0, BUFSIZE / sizeof(int), MPI_INT, win);
    }

    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    free(buf);

    return 0;
}
