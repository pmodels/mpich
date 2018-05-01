/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 4
#define COUNT 1024
MPI_Win win[NUM_THREADS];
int *win_mem[NUM_THREADS];
int rank, nprocs;
int thread_errs[NUM_THREADS];
MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    int i;
    int *local_b;
    int id = *((int *) arg);
    int result;
    thread_errs[id] = 0;

    MPI_Alloc_mem(COUNT * sizeof(int), MPI_INFO_NULL, &local_b);
    for (i = 0; i < COUNT; i++) {
        local_b[i] = rank * 100 + id;
    }
    MPI_Win_fence(0, win[id]);
    if (rank == 1) {
        MPI_Put(local_b, COUNT, MPI_INT, 0, 0, COUNT, MPI_INT, win[id]);
    }
    MPI_Win_fence(0, win[id]);
    if (rank == 0) {
        result = 100 + id;
        for (i = 0; i < COUNT; i++) {
            if (win_mem[id][i] != result) {
                thread_errs[id]++;
                if (thread_errs[id] < 10) {
                    fprintf(stderr, "win_mem[%d][%d] = %d expected %d\n", id, i, win_mem[id][i],
                            result);
                }
            }
        }
    }
    if (rank == 2) {
        MPI_Get(local_b, COUNT, MPI_INT, 0, 0, COUNT, MPI_INT, win[id]);
    }
    MPI_Win_fence(0, win[id]);
    if (rank == 2) {
        result = id + 100;
        for (i = 0; i < COUNT; i++) {
            if (local_b[i] != result) {
                thread_errs[id]++;
                if (thread_errs[id] < 10) {
                    fprintf(stderr, "thread %d: local_b[%d] = %d expected %d\n", id, i, local_b[i],
                            result);
                }
            }
        }
    }
    MPI_Free_mem(local_b);

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int i, pmode;
    int thread_args[NUM_THREADS];

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE is not supported\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs < 3) {
        printf("Run this program with 3 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        errs += MPI_Win_allocate(COUNT * sizeof(int), sizeof(int),
                                 MPI_INFO_NULL, MPI_COMM_WORLD, &win_mem[i], &win[i]);
    }


    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        errs += MTest_Start_thread(run_test, &thread_args[i]);
    }

    errs += MTest_Join_threads();

    for (i = 0; i < NUM_THREADS; i++) {
        errs += thread_errs[i];
    }
    for (i = 0; i < NUM_THREADS; i++) {
        errs += MPI_Win_free(&win[i]);
    }
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
