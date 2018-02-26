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

/* Multi-thread One-Sided MPI Request-based RMA Test
 *
 * This code performs RPUT, RGET, RACC and RGET_ACC operations into a 2D array using
 * multiple threads. The array has dimensions [X, Y] and the transfer is
 * done by NTHREADS threads concurrently.
 */

#include "mt_rma_common.h"

#define TEST_ROUNDS 10

int mpi_kernel(enum op_type mpi_op, void *origin_addr, int origin_count,
               MPI_Datatype origin_datatype, void *result_addr, int result_count,
               MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
               MPI_Request * request)
{
    switch (mpi_op) {
        case RPUT:
            MPI_Rput(origin_addr, origin_count, origin_datatype,
                     target_rank, target_disp, target_count, target_datatype, win, request);
            break;
        case RGET:
            MPI_Rget(origin_addr, origin_count, origin_datatype,
                     target_rank, target_disp, target_count, target_datatype, win, request);
            break;
        case RACC:
            MPI_Raccumulate(origin_addr, origin_count, origin_datatype,
                            target_rank, target_disp, target_count, target_datatype,
                            op, win, request);
            break;
        case RGET_ACC:
            MPI_Rget_accumulate(origin_addr, origin_count, origin_datatype,
                                result_addr, result_count, result_datatype,
                                target_rank, target_disp, target_count, target_datatype,
                                op, win, request);
            break;
        default:
            fprintf(stderr, "invalid RMA operation.\n");
            abort();
    }
    MPI_Wait(request, MPI_STATUS_IGNORE);

    return MPI_SUCCESS;
}

int main(int argc, char **argv)
{
    int pmode, i, rank, nranks, left_neighbor, right_neighbor, bufsize, err, errors = 0;
    int *win_buf, *local_buf, *result_buf;
    MPI_Win buf_win;
    thread_param_t params[NTHREADS];

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);

    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    err = MTest_thread_barrier_init();
    if (err) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    right_neighbor = rank == nranks - 1 ? 0 : rank + 1;
    left_neighbor = rank > 0 ? rank - 1 : nranks - 1;

    for (i = 0; i < NTHREADS; i++) {
        params[i].th_id = i;
        params[i].rank = rank;
        params[i].target = right_neighbor;
    }

    bufsize = XDIM * YDIM * sizeof(int);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &win_buf);
    local_buf = (int *) malloc(bufsize);
    result_buf = (int *) malloc(bufsize);

    run_init_data(local_buf, rank);

    MPI_Win_create(win_buf, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);
    MPI_Win_fence(0, buf_win);

    /* a: PUT */
    for (i = 0; i < NTHREADS; i++) {
        params[i].origin_buf = local_buf;
        params[i].win = &buf_win;
        params[i].op = RPUT;
        params[i].rounds = 1;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();
    MPI_Barrier(MPI_COMM_WORLD);

    run_verify(win_buf, &errors, rank, left_neighbor);

    /* b: GET */
    MPI_Win_fence(0, buf_win);
    run_reset_data(local_buf);
    run_init_data(win_buf, rank);
    MPI_Win_fence(0, buf_win);

    for (i = 0; i < NTHREADS; i++) {
        params[i].target = left_neighbor;
        params[i].op = RGET;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();
    /* No need to have MPI_Barrier() for Get. */
    run_verify(local_buf, &errors, rank, left_neighbor);

    /* c: ACC */
    MPI_Win_fence(0, buf_win);
    run_init_data(local_buf, rank);
    run_reset_data(win_buf);
    MPI_Win_fence(0, buf_win);

    /* Need a few repetition for ACC and GET_ACC */
    for (i = 0; i < NTHREADS; i++) {
        params[i].target = right_neighbor;
        params[i].op = RACC;
        params[i].rounds = TEST_ROUNDS;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();
    MPI_Barrier(MPI_COMM_WORLD);
    run_verify(win_buf, &errors, rank, TEST_ROUNDS * left_neighbor);

    /* d: GET_ACC */
    MPI_Win_fence(0, buf_win);
    run_reset_data(result_buf);
    run_reset_data(win_buf);
    MPI_Win_fence(0, buf_win);

    /* Need a few repetition for ACC and GET_ACC */
    for (i = 0; i < NTHREADS; i++) {
        params[i].result_buf = result_buf;
        params[i].op = RGET_ACC;
        if (i == NTHREADS - 1) {
            run_test(&params[i]);
        } else {
            MTest_Start_thread(run_test, &params[i]);
        }
    }

    MTest_Join_threads();
    MPI_Barrier(MPI_COMM_WORLD);
    run_verify(win_buf, &errors, rank, TEST_ROUNDS * left_neighbor);
    run_verify(result_buf, &errors, rank, (TEST_ROUNDS - 1) * rank);

    MPI_Win_fence(0, buf_win);
    MPI_Win_free(&buf_win);

    MPI_Free_mem(win_buf);
    free(local_buf);
    free(result_buf);

    MTest_thread_barrier_free();
    MTest_Finalize(errors);

    return 0;
}
