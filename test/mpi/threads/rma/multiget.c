/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 4
#define COUNT       16
#define LOOPS       10000

MPI_Win win;

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    int i;
    double *local_b;

    MPI_Alloc_mem(COUNT * sizeof(double), MPI_INFO_NULL, &local_b);

    for (i = 0; i < LOOPS; i++) {
        MPI_Get(local_b, COUNT, MPI_DOUBLE, 0, 0, COUNT, MPI_DOUBLE, win);
        MPI_Win_flush_all(win);
    }

    MPI_Free_mem(local_b);

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, nprocs, i, pmode;
    double *win_mem;

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE is not supported\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        errs += MPI_Win_allocate(COUNT * sizeof(double), sizeof(double),
                                 MPI_INFO_NULL, MPI_COMM_WORLD, &win_mem, &win);
    }
    else {
        errs += MPI_Win_allocate(0, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win_mem, &win);
    }

    errs += MPI_Win_lock_all(0, win);

    for (i = 0; i < NUM_THREADS; i++)
        errs += MTest_Start_thread(run_test, NULL);
    errs += MTest_Join_threads();

    errs += MPI_Win_unlock_all(win);

    errs += MPI_Win_free(&win);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
