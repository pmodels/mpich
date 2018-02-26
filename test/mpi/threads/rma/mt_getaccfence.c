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

/* Multi-thread One-Sided MPI Get_accumulate Test
 *
 * This code performs get_accumulate operations to sum two 2D arrays using
 * multiple threads. The array has dimensions [X, Y] and the transfer is
 * done by NTHREADS threads concurrently. Both small msg and huge msg transfers
 * are tested.
 */

#include "mt_rma_common.h"

#define TEST_ROUNDS 10

int mpi_kernel(enum op_type mpi_op, void *origin_addr, int origin_count,
               MPI_Datatype origin_datatype, void *result_addr, int result_count,
               MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
               MPI_Request * req)
{
    return MPI_Get_accumulate(origin_addr, origin_count,
                              origin_datatype, result_addr, result_count,
                              result_datatype, target_rank, target_disp,
                              target_count, target_datatype, op, win);
}

int main(int argc, char **argv)
{
    int pmode, i, j, rank, nranks, bufsize, err, errors = 0;
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

    if (nranks != 2) {
        fprintf(stderr, "Need two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MTest_thread_barrier_init();
    if (err) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i < NTHREADS; i++) {
        params[i].th_id = i;
        params[i].rank = rank;
        params[i].target = rank == 0 ? 1 : 0;
        params[i].rounds = TEST_ROUNDS;
        params[i].op = NOREQ;
    }

    bufsize = XDIM * YDIM * sizeof(int);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &win_buf);
    local_buf = (int *) malloc(bufsize);
    result_buf = (int *) calloc((XDIM * YDIM), sizeof(int));
    run_init_data(local_buf, nranks);
    run_init_data(win_buf, nranks);

    MPI_Win_create(win_buf, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);
    MPI_Win_fence(0, buf_win);

    if (rank == 0) {
        for (i = 0; i < NTHREADS; i++) {
            params[i].origin_buf = local_buf;
            params[i].result_buf = result_buf;
            params[i].win = &buf_win;
            if (i == NTHREADS - 1) {
                run_test(&params[i]);
            } else {
                MTest_Start_thread(run_test, &params[i]);
            }
        }

        MTest_Join_threads();
    }

    MPI_Win_fence(0, buf_win);

    /* Verify target */
    if (rank == 1)
        run_verify(win_buf, &errors, rank, (TEST_ROUNDS + 1) * nranks);

    /* Verify result */
    if (rank == 0)
        run_verify(result_buf, &errors, rank, (TEST_ROUNDS) * nranks);

    MPI_Win_free(&buf_win);

    MPI_Free_mem(win_buf);
    free(local_buf);
    free(result_buf);

    MTest_thread_barrier_free();
    MTest_Finalize(errors);

    return 0;
}
