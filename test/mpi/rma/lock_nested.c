/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run rma_lock_nested
int run(const char *arg);
#endif

#define BUFSIZE 4

/* This program tests nested lock. Process 0 locks the other processes
 * one by one, then unlock each of them.*/

int run(const char *arg)
{
    int rank = 0, nprocs = 0, dst = 0;
    int winbuf[BUFSIZE];
    MPI_Win win = MPI_WIN_NULL;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    memset(winbuf, 0, sizeof(int) * BUFSIZE);
    MPI_Win_create(winbuf, sizeof(int) * BUFSIZE, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    if (rank == 0) {
        /* lock each process */
        for (dst = 0; dst < nprocs; dst++) {
            MPI_Win_lock(MPI_LOCK_SHARED, dst, 0, win);
        }

        /* unlock each process */
        for (dst = nprocs - 1; dst >= 0; dst--) {
            MPI_Win_unlock(dst, win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_free(&win);

    return 0;
}
