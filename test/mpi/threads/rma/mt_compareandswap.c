/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Multi-thread One-Sided MPI Compare and Swap Test
 *
 * This code performs compare_and_swap operations using multiple threads.
 * Self communication, neighbor communication, contention cases are tested.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"
#include "squelch.h"

#define NTHREADS 4
#define ITER 100

typedef struct thread_param {
    int rank;
    int target;
    int target_disp;
    MPI_Win *win;
    const int *origin;
    const int *compare;
    int *result;
} thread_param_t;

MTEST_THREAD_RETURN_TYPE run_single_test(void *arg)
{
    thread_param_t *p = (thread_param_t *) arg;
    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    MPI_Compare_and_swap(p->origin, p->compare, p->result, MPI_INT, p->target, p->target_disp,
                         *(p->win));

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

int main(int argc, char **argv)
{
    int pmode, i, j, rank, nranks, err, errors = 0;
    MPI_Win win;
    int val = 0, next, result = -1;
    thread_param_t params[NTHREADS];

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);

    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    if (nranks != 4) {
        fprintf(stderr, "Need 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MTest_thread_barrier_init();
    if (err) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Win_create(&val, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    /* Test self communication */
    for (i = 0; i < NTHREADS; i++) {
        params[i].rank = rank;
        params[i].target_disp = 0;
        params[i].win = &win;
    }

    MPI_Win_fence(0, win);

    for (i = 0; i < ITER; i++) {
        next = i + 1;
        for (j = 0; j < NTHREADS; j++) {
            params[j].target = rank;
            params[j].origin = &next;
            params[j].compare = &i;
            params[j].result = &result;
            if (j == NTHREADS - 1) {
                run_single_test(&params[j]);
            } else {
                MTest_Start_thread(run_single_test, &params[j]);
            }
        }
        MTest_Join_threads();
        MPI_Win_fence(0, win);

        if (result != i + 1) {
            SQUELCH(printf
                    ("%d->%d [%d] -- SELF: expected result %d, got result %d\n",
                     rank, rank, i, i + 1, result););
            errors++;
        }
        if (val != i + 1) {
            SQUELCH(printf
                    ("%d->%d [%d] -- SELF: expected %d, got %d\n", rank, rank, i, i + 1, val););
            errors++;
        }
    }

    /* Test neighbor communication */
    val = 0;
    MPI_Win_fence(0, win);

    for (i = 0; i < ITER; i++) {
        /* Barrier here to make sure all ranks finish checking. */
        MPI_Barrier(MPI_COMM_WORLD);
        next = i + 1;
        for (j = 0; j < NTHREADS; j++) {
            params[j].target = (rank + 1) % nranks;
            params[j].origin = &next;
            params[j].compare = &i;
            params[j].result = &result;
            if (j == NTHREADS - 1) {
                run_single_test(&params[j]);
            } else {
                MTest_Start_thread(run_single_test, &params[j]);
            }
        }
        MTest_Join_threads();
        MPI_Win_fence(0, win);

        if (result != i + 1) {
            SQUELCH(printf
                    ("%d->%d [%d] -- NEIGHBOR: expected result %d, got result %d, val: %d.\n",
                     rank, (rank + 1) % nranks, i, i + 1, result, val););
            errors++;
        }
        if (val != i + 1) {
            SQUELCH(printf
                    ("%d->%d [%d] -- NEIGHBOR: expected %d, got %d, result: %d\n",
                     rank, (rank + 1) % nranks, i, i + 1, val, result););
            errors++;
        }
    }

    /* Test contention */
    val = 0;
    MPI_Win_fence(0, win);

    for (i = 0; i < ITER; i++) {
        /* Barrier here to make sure rank 0 finishes checking. */
        MPI_Barrier(MPI_COMM_WORLD);
        next = i + 1;

        if (rank != 0) {
            for (j = 0; j < NTHREADS; j++) {
                params[j].target = 0;
                params[j].origin = &next;
                params[j].compare = &i;
                params[j].result = &result;
                if (j == NTHREADS - 1) {
                    run_single_test(&params[j]);
                } else {
                    MTest_Start_thread(run_single_test, &params[j]);
                }
            }
            MTest_Join_threads();
        }
        MPI_Win_fence(0, win);

        if (rank == 0) {
            if (val != i + 1) {
                SQUELCH(printf
                        ("*->%d [%d] -- CONTENTION: expected %d, got %d\n", 0, i, i + 1, val););
                errors++;
            }
        }
    }

    MPI_Win_free(&win);

    MTest_thread_barrier_free();
    MTest_Finalize(errors);

    return 0;
}
