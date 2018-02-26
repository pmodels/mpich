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
 * This code performs several put/get operations into a 2D array using
 * multiple threads. The array has dimensions [X, Y] and the transfer is
 * done by NTHREADS threads concurrently. Both small msg and huge msg transfers
 * are tested.
 */

#include "mt_rma_common.h"

#define TEST_ROUNDS 1

#ifdef PUT_TEST
#define PUT_GET_FUNC MPI_Put
#else
#define PUT_GET_FUNC MPI_Get
#endif /* #ifdef PUT_TEST */

int mpi_kernel(enum op_type mpi_op, void *origin_addr, int origin_count,
               MPI_Datatype origin_datatype, void *result_addr, int result_count,
               MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
               int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
               MPI_Request * req)
{
    return PUT_GET_FUNC(origin_addr, origin_count, origin_datatype,
                        target_rank, target_disp, target_count, target_datatype, win);
}

/* Sync type */
enum sync_type { FENCE = 0, PSCW, LOCK, LOCK_ALL };

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

int main(int argc, char **argv)
{
    int pmode, i, j, rank, nranks, peer, bufsize, err, errors = 0;
    int *win_buf, *local_buf;
    MPI_Win buf_win;
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
        params[i].rounds = TEST_ROUNDS;
        params[i].op = NOREQ;
    }

    bufsize = XDIM * YDIM * sizeof(int);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &win_buf);
    local_buf = (int *) malloc(bufsize);
#ifdef PUT_TEST
    if (rank == 0) {
        run_init_data(local_buf, nranks);
        run_reset_data(win_buf);
    }
#else
    if (rank == 1) {
        run_init_data(win_buf, nranks);
        run_reset_data(local_buf);
    }
#endif /* #ifdef PUT_TEST */

    MPI_Win_create(win_buf, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);
    MPI_Win_fence(0, buf_win);

    /* a: sync by win_fence */
    run_sync(buf_win, peer_group, rank, peer, FENCE, 1);

    if (rank == 0) {
        for (i = 0; i < NTHREADS; i++) {
            params[i].origin_buf = local_buf;
            params[i].win = &buf_win;
            if (i == NTHREADS - 1) {
                run_test(&params[i]);
            } else {
                MTest_Start_thread(run_test, &params[i]);
            }
        }

        MTest_Join_threads();
    }

    run_sync(buf_win, peer_group, rank, peer, FENCE, 0);

#ifdef PUT_TEST
    if (rank == 1)
        run_verify(win_buf, &errors, rank, nranks);
#else
    if (rank == 0)
        run_verify(local_buf, &errors, rank, nranks);
#endif /* #ifdef PUT_TEST */

    /* b: sync by PSCW */
    /* reset data */
#ifdef PUT_TEST
    if (rank == 1)
        run_reset_data(win_buf);
#else
    if (rank == 0)
        run_reset_data(local_buf);
#endif /* #ifdef PUT_TEST */
    MPI_Win_fence(0, buf_win);
    run_sync(buf_win, peer_group, rank, peer, PSCW, 1);

    if (rank == 0) {
        for (i = 0; i < NTHREADS; i++) {
            params[i].origin_buf = local_buf;
            params[i].win = &buf_win;
            if (i == NTHREADS - 1) {
                run_test(&params[i]);
            } else {
                MTest_Start_thread(run_test, &params[i]);
            }
        }

        MTest_Join_threads();
    }

    run_sync(buf_win, peer_group, rank, peer, PSCW, 0);

#ifdef PUT_TEST
    if (rank == 1)
        run_verify(win_buf, &errors, rank, nranks);
#else
    if (rank == 0)
        run_verify(local_buf, &errors, rank, nranks);
#endif /* #ifdef PUT_TEST */

    /* c & d:
     * Passive target synchronization used for Get only. */
#ifdef GET_TEST
    /* c: sync by win_lock */
    if (rank == 0)
        run_reset_data(local_buf);

    MPI_Win_fence(0, buf_win);

    run_sync(buf_win, peer_group, rank, peer, LOCK, 1);

    if (rank == 0) {
        for (i = 0; i < NTHREADS; i++) {
            params[i].origin_buf = local_buf;
            params[i].win = &buf_win;
            if (i == NTHREADS - 1) {
                run_test(&params[i]);
            } else {
                MTest_Start_thread(run_test, &params[i]);
            }
        }

        MTest_Join_threads();
    }

    run_sync(buf_win, peer_group, rank, peer, LOCK, 0);

    if (rank == 0)
        run_verify(local_buf, &errors, rank, nranks);

    /* d: sync by win_lock_all */
    if (rank == 0)
        run_reset_data(local_buf);

    MPI_Win_fence(0, buf_win);

    run_sync(buf_win, peer_group, rank, peer, LOCK_ALL, 1);

    if (rank == 0) {
        for (i = 0; i < NTHREADS; i++) {
            params[i].origin_buf = local_buf;
            params[i].win = &buf_win;
            if (i == NTHREADS - 1) {
                run_test(&params[i]);
            } else {
                MTest_Start_thread(run_test, &params[i]);
            }
        }

        MTest_Join_threads();
    }

    run_sync(buf_win, peer_group, rank, peer, LOCK_ALL, 0);

    if (rank == 0)
        run_verify(local_buf, &errors, rank, nranks);
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
