/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"
#include "mpithreadtest.h"

#define COUNT 1
#define NUM_THREADS 4
#define LOOPS 100000

MPI_Win win;
int errs = 0, dummy;

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    int i;

    for (i = 0; i < LOOPS; i++) {
        /* send a global variable, rather than a stack variable, so
         * other threads can access the address during flush */
        MPI_Put(&dummy, 1, MPI_INT, 0, 0, 1, MPI_INT, win);
        MPI_Win_flush(0, win);
    }

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

int main(int argc, char *argv[])
{
    int nprocs, i, pmode;
    char *win_buf;

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "Thread Multiple not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    errs +=
        MPI_Win_allocate(COUNT * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win_buf,
                         &win);
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
