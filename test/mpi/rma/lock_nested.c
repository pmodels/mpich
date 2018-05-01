/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2016 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "mpitest.h"

#define BUFSIZE 4

/* This program tests nested lock. Process 0 locks the other processes
 * one by one, then unlock each of them.*/

int main(int argc, char *argv[])
{
    int rank = 0, nprocs = 0, dst = 0;
    int winbuf[BUFSIZE];
    MPI_Win win = MPI_WIN_NULL;

    MTest_Init(&argc, &argv);
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

    MTest_Finalize(0);
    return 0;
}
