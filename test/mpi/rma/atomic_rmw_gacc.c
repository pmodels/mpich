/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test is going to test the atomicity for "read-modify-write" in GACC
 * operations */

/* This test is similiar with atomic_rmw_fop.c.
 * There are three processes involved in this test: P0 (origin_shm), P1 (origin_am),
 * and P2 (dest). P0 and P1 issues multiple GACC with MPI_SUM and OP_COUNT integers
 * (value 1) to P2 via SHM and AM respectively. The correct results should be that the
 * results on P0 and P1 never be the same for intergers on the corresponding index
 * in [0...OP_COUNT-1].
 */

#include "mpi.h"
#include <stdio.h>

#define OP_COUNT 10
#define AM_BUF_NUM  10
#define SHM_BUF_NUM 10000
#define WIN_BUF_NUM 1

#define LOOP_SIZE 15
#define CHECK_TAG 123

int rank, size;
int dest, origin_shm, origin_am;
int *orig_buf = NULL, *result_buf = NULL, *target_buf = NULL, *check_buf = NULL;
MPI_Win win;

void checkResults(int loop_k, int *errors)
{
    int i, j, m;
    MPI_Status status;

    if (rank != dest) {
        /* check results on P0 and P2 (origin) */
        if (rank == origin_am) {
            MPI_Send(result_buf, AM_BUF_NUM * OP_COUNT, MPI_INT, origin_shm, CHECK_TAG,
                     MPI_COMM_WORLD);
        }
        else if (rank == origin_shm) {
            MPI_Alloc_mem(sizeof(int) * AM_BUF_NUM * OP_COUNT, MPI_INFO_NULL, &check_buf);
            MPI_Recv(check_buf, AM_BUF_NUM * OP_COUNT, MPI_INT, origin_am, CHECK_TAG,
                     MPI_COMM_WORLD, &status);
            for (i = 0; i < AM_BUF_NUM; i++) {
                for (j = 0; j < SHM_BUF_NUM; j++) {
                    for (m = 0; m < OP_COUNT; m++) {
                        if (check_buf[i * OP_COUNT + m] == result_buf[j * OP_COUNT + m]) {
                            printf
                                ("LOOP=%d, rank=%d, FOP, both check_buf[%d] and result_buf[%d] equal to %d, expected to be different. \n",
                                 loop_k, rank, i * OP_COUNT + m, j * OP_COUNT + m,
                                 check_buf[i * OP_COUNT + m]);
                            (*errors)++;
                        }
                    }
                }
            }
            MPI_Free_mem(check_buf);
        }
    }
    else {
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
        /* check results on P1 */
        for (i = 0; i < OP_COUNT; i++) {
            if (target_buf[i] != AM_BUF_NUM + SHM_BUF_NUM) {
                printf("LOOP=%d, rank=%d, FOP, target_buf[%d] = %d, expected %d. \n",
                       loop_k, rank, i, target_buf[i], AM_BUF_NUM + SHM_BUF_NUM);
                (*errors)++;
            }
        }
        MPI_Win_unlock(rank, win);
    }
}

int main(int argc, char *argv[])
{
    int i, k;
    int errors = 0, all_errors = 0;
    int my_buf_num;
    MPI_Datatype origin_dtp, target_dtp;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != 3) {
        /* run this test with three processes */
        goto exit_test;
    }

    MPI_Type_contiguous(OP_COUNT, MPI_INT, &origin_dtp);
    MPI_Type_commit(&origin_dtp);
    MPI_Type_contiguous(OP_COUNT, MPI_INT, &target_dtp);
    MPI_Type_commit(&target_dtp);

    /* this works when MPIR_PARAM_CH3_ODD_EVEN_CLIQUES is set */
    dest = 2;
    origin_shm = 0;
    origin_am = 1;

    if (rank == origin_am)
        my_buf_num = AM_BUF_NUM;
    else if (rank == origin_shm)
        my_buf_num = SHM_BUF_NUM;

    if (rank != dest) {
        MPI_Alloc_mem(sizeof(int) * my_buf_num * OP_COUNT, MPI_INFO_NULL, &orig_buf);
        MPI_Alloc_mem(sizeof(int) * my_buf_num * OP_COUNT, MPI_INFO_NULL, &result_buf);
    }

    MPI_Win_allocate(sizeof(int) * WIN_BUF_NUM * OP_COUNT, sizeof(int), MPI_INFO_NULL,
                     MPI_COMM_WORLD, &target_buf, &win);

    for (k = 0; k < LOOP_SIZE; k++) {

        /* ====== Part 1: test basic datatypes ======== */

        /* init buffers */
        if (rank != dest) {
            for (i = 0; i < my_buf_num * OP_COUNT; i++) {
                orig_buf[i] = 1;
                result_buf[i] = 0;
            }
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < WIN_BUF_NUM * OP_COUNT; i++) {
                target_buf[i] = 0;
            }
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Win_lock_all(0, win);
        if (rank != dest) {
            for (i = 0; i < my_buf_num; i++) {
                MPI_Get_accumulate(&(orig_buf[i * OP_COUNT]), OP_COUNT, MPI_INT,
                                   &(result_buf[i * OP_COUNT]), OP_COUNT, MPI_INT,
                                   dest, 0, OP_COUNT, MPI_INT, MPI_SUM, win);
                MPI_Win_flush(dest, win);
            }
        }
        MPI_Win_unlock_all(win);

        MPI_Barrier(MPI_COMM_WORLD);

        checkResults(k, &errors);

        /* ====== Part 2: test derived datatypes (origin derived, target derived) ======== */

        /* init buffers */
        if (rank != dest) {
            for (i = 0; i < my_buf_num * OP_COUNT; i++) {
                orig_buf[i] = 1;
                result_buf[i] = 0;
            }
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < WIN_BUF_NUM * OP_COUNT; i++) {
                target_buf[i] = 0;
            }
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Win_lock_all(0, win);
        if (rank != dest) {
            for (i = 0; i < my_buf_num; i++) {
                MPI_Get_accumulate(&(orig_buf[i * OP_COUNT]), 1, origin_dtp,
                                   &(result_buf[i * OP_COUNT]), 1, origin_dtp,
                                   dest, 0, 1, target_dtp, MPI_SUM, win);
                MPI_Win_flush(dest, win);
            }
        }
        MPI_Win_unlock_all(win);

        MPI_Barrier(MPI_COMM_WORLD);

        checkResults(k, &errors);

        /* ====== Part 3: test derived datatypes (origin basic, target derived) ======== */

        /* init buffers */
        if (rank != dest) {
            for (i = 0; i < my_buf_num * OP_COUNT; i++) {
                orig_buf[i] = 1;
                result_buf[i] = 0;
            }
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < WIN_BUF_NUM * OP_COUNT; i++) {
                target_buf[i] = 0;
            }
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Win_lock_all(0, win);
        if (rank != dest) {
            for (i = 0; i < my_buf_num; i++) {
                MPI_Get_accumulate(&(orig_buf[i * OP_COUNT]), OP_COUNT, MPI_INT,
                                   &(result_buf[i * OP_COUNT]), OP_COUNT, MPI_INT,
                                   dest, 0, 1, target_dtp, MPI_SUM, win);
                MPI_Win_flush(dest, win);
            }
        }
        MPI_Win_unlock_all(win);

        MPI_Barrier(MPI_COMM_WORLD);

        checkResults(k, &errors);

        /* ====== Part 4: test derived datatypes (origin derived target basic) ======== */

        /* init buffers */
        if (rank != dest) {
            for (i = 0; i < my_buf_num * OP_COUNT; i++) {
                orig_buf[i] = 1;
                result_buf[i] = 0;
            }
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < WIN_BUF_NUM * OP_COUNT; i++) {
                target_buf[i] = 0;
            }
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Win_lock_all(0, win);
        if (rank != dest) {
            for (i = 0; i < my_buf_num; i++) {
                MPI_Get_accumulate(&(orig_buf[i * OP_COUNT]), 1, origin_dtp,
                                   &(result_buf[i * OP_COUNT]), 1, origin_dtp,
                                   dest, 0, OP_COUNT, MPI_INT, MPI_SUM, win);
                MPI_Win_flush(dest, win);
            }
        }
        MPI_Win_unlock_all(win);

        MPI_Barrier(MPI_COMM_WORLD);

        checkResults(k, &errors);
    }

    MPI_Win_free(&win);

    if (rank == origin_am || rank == origin_shm) {
        MPI_Free_mem(orig_buf);
        MPI_Free_mem(result_buf);
    }

    MPI_Type_free(&origin_dtp);
    MPI_Type_free(&target_dtp);

  exit_test:
    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();
    return 0;
}
