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

/* Multi-thread One-Sided MPI Put/Get Test
 *
 * Author: Chongxiao Cao <chongxiao.cao@intel.com>
 * Date: Feb 2018
 *
 * This code performs several put/get operations into a 2D array using
 * multiple threads. The array has dimensions [X, Y] and the transfer is
 * done by NTHREADS threads concurrently. Both small msg and huge msg transfers
 * are tested.
 */

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

#ifdef PUT_TEST
#define PUT_GET_FUNC MPI_Put
#else
#define PUT_GET_FUNC MPI_Get
#endif /* #ifdef PUT_TEST */

typedef struct thread_param {
    int th_id;                  /* Thread id */
    int rank;
    int target;
    MPI_Win *win;
    int *buf;
} thread_param_t;

/* Sync type */
enum sync_type { FENCE = 0, PSCW, LOCK, LOCK_ALL };

void run_init_data(int *buf, int my_rank, int rank)
{
    int i, j;
    if (my_rank == rank) {
        for (i = 0; i < XDIM; i++) {
            for (j = 0; j < YDIM; j++) {
                buf[j * XDIM + i] = j / (YDIM / NTHREADS);
            }
        }
    }
}

void run_reset_data(int *buf, int my_rank, int rank)
{
    if (my_rank == rank)
        memset(buf, 0, sizeof(int) * XDIM * YDIM);
}

int run_sync(MPI_Win win, MPI_Group group, int rank, int peer, enum sync_type sync, int lock)
{
    switch (sync) {
        case FENCE:
            MPI_Win_fence(0, win);
            break;
        case PSCW:
            if (rank == 0) {
                /* rank 0 is origin. */
                if (lock) {
                    MPI_Win_start(group, 0, win);
                } else {
                    MPI_Win_complete(win);
                }
            } else {
                /* rank 1 is target. */
                if (lock) {
                    MPI_Win_post(group, 0, win);
                } else {
                    MPI_Win_wait(win);
                }
            }
            break;
        case LOCK:
            if (rank == 0) {
                if (lock) {
                    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, peer, 0, win);
                } else {
                    MPI_Win_unlock(peer, win);
                }
            }
            break;
        case LOCK_ALL:
            if (rank == 0) {
                if (lock) {
                    MPI_Win_lock_all(0, win);
                } else {
                    MPI_Win_unlock_all(win);
                }
            }
            break;
        default:
            fprintf(stderr, "Invalid synchronization type.\n");
            exit(-1);
    }
    return MPI_SUCCESS;
}

int test_putget_huge(int th_id, MPI_Win * win, int rank, int target, int *buf)
{
    int row_start, row_end, count;
    if (rank == 0) {
        /* rank 0 is origin */
        row_start = th_id * (YDIM / NTHREADS);
        row_end = (row_start + (YDIM / NTHREADS) - 1);
        if (row_end > (YDIM - 1))
            row_end = YDIM - 1; /* handle the case of out of bound */

        count = (row_end - row_start + 1) * XDIM;
        PUT_GET_FUNC((buf + row_start * XDIM), count, MPI_INT,
                     target, (row_start * XDIM * sizeof(int)), count, MPI_INT, *win);
    }
    return MPI_SUCCESS;
}

int test_putget_normal(int th_id, MPI_Win * win, int rank, int target, int *buf)
{
    int j, row_start, row_end, count;
    if (rank == 0) {
        /* rank 0 is origin */
        row_start = th_id * (YDIM / NTHREADS);
        row_end = (row_start + (YDIM / NTHREADS) - 1);
        if (row_end > (YDIM - 1))
            row_end = YDIM - 1; /* handle the case of out of bound */

        count = (row_end - row_start + 1) * XDIM;
        for (j = 0; j < count; j++) {
            PUT_GET_FUNC((buf + row_start * XDIM + j), 1, MPI_INT,
                         target, ((row_start * XDIM + j) * sizeof(int)), 1, MPI_INT, *win);
        }
    }
    return MPI_SUCCESS;
}

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    thread_param_t *p = (thread_param_t *) arg;

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);
#ifdef HUGE_COUNT
    test_putget_huge(p->th_id, p->win, p->rank, p->target, p->buf);
#else /* Small data test */
    test_putget_normal(p->th_id, p->win, p->rank, p->target, p->buf);
#endif /* #ifdef HUGE_COUNT */

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

void run_verify(int *buf, int *errors, int my_rank, int rank)
{
    int i, j;
    if (my_rank == rank) {
        for (i = 0; i < XDIM; i++) {
            for (j = 0; j < YDIM; j++) {
                if (buf[j * XDIM + i] != j / (YDIM / NTHREADS)) {
                    SQUELCH(printf
                            ("rank: %d: Data validation failed at [%d, %d] expected=%d actual=%d\n",
                             my_rank, j, i, j / (YDIM / NTHREADS), buf[j * XDIM + i]););
                    (*errors)++;
                    fflush(stdout);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    int pmode, i, j, rank, nranks, peer, bufsize, err, errors = 0;
    int *win_buf, *local_buf;
    MPI_Win buf_win, buf_wins[NTHREADS];
    MPI_Group world_group, peer_group;
    thread_param_t params[NTHREADS];

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);

    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    if (nranks != 2) {
        fprintf(stderr, "Need two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MTest_thread_barrier_init();
    if (err) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    peer = (rank + 1) % nranks;
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_incl(world_group, 1, &peer, &peer_group);

    for (i = 0; i < NTHREADS; i++) {
        params[i].th_id = i;
        params[i].rank = rank;
        params[i].target = peer;
    }

    bufsize = XDIM * YDIM * sizeof(int);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &win_buf);
    local_buf = (int *) malloc(bufsize);
#ifdef PUT_TEST
    run_init_data(local_buf, rank, 0);
#else
    run_init_data(win_buf, rank, 1);
#endif /* #ifdef PUT_TEST */

    MPI_Win_create(win_buf, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);
    MPI_Win_fence(0, buf_win);

    /* a: sync by win_fence */
    run_sync(buf_win, peer_group, rank, peer, FENCE, 1);

    for (i = 0; i < NTHREADS; i++) {
        params[i].buf = local_buf;
        params[i].win = &buf_win;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();

    run_sync(buf_win, peer_group, rank, peer, FENCE, 0);

#ifdef PUT_TEST
    run_verify(win_buf, &errors, rank, 1);
#else
    run_verify(local_buf, &errors, rank, 0);
#endif /* #ifdef PUT_TEST */

    /* b: sync by PSCW */
    /* reset data */
#ifdef PUT_TEST
    run_reset_data(win_buf, rank, 1);
#else
    run_reset_data(local_buf, rank, 0);
#endif /* #ifdef PUT_TEST */
    MPI_Win_fence(0, buf_win);
    run_sync(buf_win, peer_group, rank, peer, PSCW, 1);

    for (i = 0; i < NTHREADS; i++) {
        params[i].buf = local_buf;
        params[i].win = &buf_win;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();

    run_sync(buf_win, peer_group, rank, peer, PSCW, 0);

#ifdef PUT_TEST
    run_verify(win_buf, &errors, rank, 1);
#else
    run_verify(local_buf, &errors, rank, 0);
#endif /* #ifdef PUT_TEST */

    /* c & d:
     * Passive target synchronization used for Get only. */
#ifdef GET_TEST
    /* c: sync by win_lock */
    run_reset_data(local_buf, rank, 0);
    MPI_Win_fence(0, buf_win);

    run_sync(buf_win, peer_group, rank, peer, LOCK, 1);

    for (i = 0; i < NTHREADS; i++) {
        params[i].buf = local_buf;
        params[i].win = &buf_win;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();
    run_sync(buf_win, peer_group, rank, peer, LOCK, 0);

    run_verify(local_buf, &errors, rank, 0);

    /* d: sync by win_lock_all */
    run_reset_data(local_buf, rank, 0);
    MPI_Win_fence(0, buf_win);

    run_sync(buf_win, peer_group, rank, peer, LOCK_ALL, 1);

    for (i = 0; i < NTHREADS; i++) {
        params[i].buf = local_buf;
        params[i].win = &buf_win;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();
    run_sync(buf_win, peer_group, rank, peer, LOCK_ALL, 0);

    run_verify(local_buf, &errors, rank, 0);
#endif /* #ifdef GET_TEST */

    MPI_Win_free(&buf_win);
    MPI_Group_free(&peer_group);
    MPI_Group_free(&world_group);

    MPI_Free_mem(win_buf);
    free(local_buf);

    MTest_thread_barrier_free();
    MTest_Finalize(errors);

    return 0;
}
