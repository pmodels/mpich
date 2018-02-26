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

/* Multi-thread One-Sided MPI Fetch and Op Test
 *
 * This code performs fetch_and_op operations using multiple threads.
 * Self communication, neighbor communication, contention and all to all
 * cases are tested.
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
#define CMP(x, y) ((x - (double)(y)) > 1.0e-9)

typedef struct thread_param {
    int rank;
    int target;
    int target_disp;
    MPI_Win *win;
    const double *origin;
    double *result;
} thread_param_t;

void reset_vars(double *val_ptr, double *res_ptr, int nproc)
{
    int i;

    for (i = 0; i < nproc; i++) {
        val_ptr[i] = 0;
        res_ptr[i] = -1;
    }
}

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    thread_param_t *p = (thread_param_t *) arg;
    int i;
    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    for (i = 0; i < ITER; i++) {
        MPI_Fetch_and_op(p->origin, p->result, MPI_DOUBLE, p->target, 0, MPI_SUM, *(p->win));
    }

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

MTEST_THREAD_RETURN_TYPE run_single_test(void *arg)
{
    thread_param_t *p = (thread_param_t *) arg;
    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    MPI_Fetch_and_op(p->origin, p->result, MPI_DOUBLE, p->target, p->target_disp, MPI_SUM,
                     *(p->win));

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

int main(int argc, char **argv)
{
    int pmode, i, rank, nranks, err, errors = 0;
    MPI_Win win;
    double *val_ptr, *res_ptr;
    thread_param_t params[NTHREADS];

    /* Test self communication */
    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);

    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    val_ptr = (double *) malloc(sizeof(double) * nranks);
    res_ptr = (double *) malloc(sizeof(double) * nranks);

    if (nranks != 4) {
        fprintf(stderr, "Need 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MTest_thread_barrier_init();
    if (err) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Win_create(val_ptr, nranks * sizeof(double), sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD,
                   &win);

    reset_vars(val_ptr, res_ptr, nranks);

    double one = 1, result = -1;
    for (i = 0; i < NTHREADS; i++) {
        params[i].rank = rank;
        params[i].target = rank;
        params[i].win = &win;
    }

    MPI_Win_fence(0, win);

    for (i = 0; i < NTHREADS; i++) {
        params[i].origin = &one;
        params[i].result = &result;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();

    MPI_Win_fence(0, win);

    if (CMP(val_ptr[0], ITER * NTHREADS)) {
        SQUELCH(printf
                ("%d->%d -- SELF: expected %f, got %f\n", rank, rank,
                 (double) (ITER * NTHREADS), val_ptr[0]););
        errors++;
    }

    /* Test neighbor communication */

    reset_vars(val_ptr, res_ptr, nranks);

    MPI_Win_fence(0, win);

    for (i = 0; i < NTHREADS; i++) {
        params[i].target = (rank + 1) % nranks;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();

    MPI_Win_fence(0, win);

    if (CMP(val_ptr[0], ITER * NTHREADS)) {
        SQUELCH(printf
                ("%d->%d -- SELF: expected %f, got %f\n", rank, rank,
                 (double) (ITER * NTHREADS), val_ptr[0]););
        errors++;
    }

    /* Test contention */

    reset_vars(val_ptr, res_ptr, nranks);

    MPI_Win_fence(0, win);

    if (rank != 0) {
        for (i = 0; i < NTHREADS; i++) {
            params[i].target = 0;
            if (i == NTHREADS - 1) {
                run_test(&params[i]);
            } else {
                MTest_Start_thread(run_test, &params[i]);
            }
        }
    }

    MTest_Join_threads();

    MPI_Win_fence(0, win);
    if (rank == 0) {
        if (CMP(val_ptr[0], ITER * NTHREADS * (nranks - 1))) {
            SQUELCH(printf
                    ("*->%d - CONTENTION: expected %f, got %f\n", rank,
                     (double) (NTHREADS * ITER * (nranks - 1)), val_ptr[0]););
            errors++;
        }
    }

    /* Test all-to-all (verify result_addr) */
    reset_vars(val_ptr, res_ptr, nranks);
    MPI_Win_fence(0, win);

    double rank_cnv = rank;
    for (i = 0; i < ITER; i++) {
        /* Barrier here to make sure all ranks finish checking. */
        MPI_Barrier(MPI_COMM_WORLD);
        int j, k;

        for (j = 0; j < nranks; j++) {
            for (k = 0; k < NTHREADS; k++) {
                params[k].target = j;
                params[k].origin = &rank_cnv;
                params[k].result = &(res_ptr[j]);
                params[k].target_disp = rank;
                if (k == NTHREADS - 1) {
                    run_single_test(&params[k]);
                } else {
                    MTest_Start_thread(run_single_test, &params[k]);
                }
            }
            MTest_Join_threads();
            MPI_Win_fence(0, win);
        }

        for (j = 0; j < nranks; j++) {
            if (CMP(res_ptr[j], (i + 1) * rank * NTHREADS - rank)) {
                SQUELCH(printf
                        ("%d->%d -- ALL-TO-ALL (FENCE) [%d]: expected result %f, got result %f\n",
                         rank, j, i, (double) ((i + 1) * rank * NTHREADS - rank), res_ptr[j]););
                errors++;
            }
            if (CMP(val_ptr[j], (i + 1) * j * NTHREADS)) {
                SQUELCH(printf
                        ("%d->%d -- ALL-TO-ALL (FENCE) [%d]: expected %f, got %f\n",
                         rank, j, i, (double) ((i + 1) * j * NTHREADS), val_ptr[j]););
                errors++;
            }
        }
    }

    MPI_Win_free(&win);

    free(val_ptr);
    free(res_ptr);

    MTest_thread_barrier_free();
    MTest_Finalize(errors);

    return 0;
}
