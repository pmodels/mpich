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
#ifndef MT_RMA_COMMON_H_INCLUDED
#define MT_RMA_COMMON_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"
#include "squelch.h"

#define NTHREADS 4

#define XDIM 256
#define YDIM 256

enum op_type { NOREQ = 0, RPUT, RGET, RACC, RGET_ACC };

typedef struct thread_param {
    int th_id;                  /* Thread id */
    int rank;
    int target;
    MPI_Win *win;
    int *origin_buf;
    int *result_buf;
    int rounds;                 /* Number of repitition */
    enum op_type op;
} thread_param_t;

static void run_init_data(int *buf, int val)
{
    int i, j;
    for (i = 0; i < XDIM; i++) {
        for (j = 0; j < YDIM; j++) {
            buf[j * XDIM + i] = val;
        }
    }
}

static void run_reset_data(int *buf)
{
    memset(buf, 0, sizeof(int) * XDIM * YDIM);
}

static void run_verify(int *buf, int *errors, int my_rank, int result)
{
    int i, j;
    for (i = 0; i < XDIM; i++) {
        for (j = 0; j < YDIM; j++) {
            if (buf[j * XDIM + i] != result) {
                SQUELCH(printf
                        ("rank: %d: Data validation failed at [%d, %d] expected=%d actual=%d\n",
                         my_rank, j, i, result, buf[j * XDIM + i]););
                (*errors)++;
                fflush(stdout);
            }
        }
    }
}

extern int mpi_kernel(enum op_type mpi_op, void *origin_addr, int origin_count,
                      MPI_Datatype origin_datatype, void *result_addr, int result_count,
                      MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                      int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                      MPI_Request * request);

static int test_huge(enum op_type mpi_op, int th_id, int rounds,
                     MPI_Win * win, int target, int *origin, int *result)
{
    int row_start, row_end, count, i;
    MPI_Request req;
    row_start = th_id * (YDIM / NTHREADS);
    row_end = (row_start + (YDIM / NTHREADS) - 1);
    if (row_end > (YDIM - 1))
        row_end = YDIM - 1;     /* handle the case of out of bound */

    count = (row_end - row_start + 1) * XDIM;
    for (i = 0; i < rounds; i++) {
        mpi_kernel(mpi_op, (origin + row_start * XDIM), count, MPI_INT,
                   (result + row_start * XDIM), count, MPI_INT,
                   target, (row_start * XDIM * sizeof(int)), count, MPI_INT, MPI_SUM, *win, &req);
    }
    return MPI_SUCCESS;
}

static int test_normal(enum op_type mpi_op, int th_id, int rounds,
                       MPI_Win * win, int target, int *origin, int *result)
{
    int j, row_start, row_end, count, i;
    MPI_Request req;
    row_start = th_id * (YDIM / NTHREADS);
    row_end = (row_start + (YDIM / NTHREADS) - 1);
    if (row_end > (YDIM - 1))
        row_end = YDIM - 1;     /* handle the case of out of bound */

    count = (row_end - row_start + 1) * XDIM;
    for (j = 0; j < count; j++) {
        for (i = 0; i < rounds; i++) {
            mpi_kernel(mpi_op, (origin + row_start * XDIM + j), 1, MPI_INT,
                       (result + row_start * XDIM + j), 1, MPI_INT,
                       target, ((row_start * XDIM + j) * sizeof(int)), 1, MPI_INT,
                       MPI_SUM, *win, &req);
        }
    }
    return MPI_SUCCESS;
}

static MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    thread_param_t *p = (thread_param_t *) arg;

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);
#ifdef HUGE_COUNT
    test_huge(p->op, p->th_id, p->rounds, p->win, p->target, p->origin_buf, p->result_buf);
#else /* Small data test */
    test_normal(p->op, p->th_id, p->rounds, p->win, p->target, p->origin_buf, p->result_buf);
#endif /* #ifdef HUGE_COUNT */

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

#endif /* MT_RMA_COMMON_INCLUDED */
