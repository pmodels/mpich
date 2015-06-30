/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test is going to test the atomicity for "read-modify-write" in FOP
 * operations */

/* There are three processes involved in this test: P0 (origin_shm), P1 (origin_am),
 * and P2 (dest). P0 and P1 issues multiple FOP with MPI_SUM and integer (value 1)
 * to P2 via SHM and AM respectively. The correct results should be that the
 * results on P0 and P1 never be the same. */

#include "mpi.h"
#include <stdio.h>

#define AM_BUF_SIZE  10
#define SHM_BUF_SIZE 1000
#define WIN_BUF_SIZE 1

#define LOOP_SIZE 15
#define CHECK_TAG 123

int main(int argc, char *argv[])
{
    int rank, size, i, j, k;
    int errors = 0, all_errors = 0;
    int origin_shm, origin_am, dest;
    int my_buf_size;
    int *orig_buf = NULL, *result_buf = NULL, *target_buf = NULL, *check_buf = NULL;
    MPI_Win win;
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != 3) {
        /* run this test with three processes */
        goto exit_test;
    }

    /* this works when MPIR_PARAM_CH3_ODD_EVEN_CLIQUES is set */
    dest = 2;
    origin_shm = 0;
    origin_am = 1;

    if (rank == origin_am)
        my_buf_size = AM_BUF_SIZE;
    else if (rank == origin_shm)
        my_buf_size = SHM_BUF_SIZE;

    if (rank != dest) {
        MPI_Alloc_mem(sizeof(int) * my_buf_size, MPI_INFO_NULL, &orig_buf);
        MPI_Alloc_mem(sizeof(int) * my_buf_size, MPI_INFO_NULL, &result_buf);
    }

    MPI_Win_allocate(sizeof(int) * WIN_BUF_SIZE, sizeof(int), MPI_INFO_NULL,
                     MPI_COMM_WORLD, &target_buf, &win);

    for (k = 0; k < LOOP_SIZE; k++) {

        /* init buffers */
        if (rank != dest) {
            for (i = 0; i < my_buf_size; i++) {
                orig_buf[i] = 1;
                result_buf[i] = 0;
            }
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < WIN_BUF_SIZE; i++) {
                target_buf[i] = 0;
            }
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        /* perform FOP */
        MPI_Win_lock_all(0, win);
        if (rank != dest) {
            for (i = 0; i < my_buf_size; i++) {
                MPI_Fetch_and_op(&(orig_buf[i]), &(result_buf[i]), MPI_INT, dest, 0, MPI_SUM, win);
                MPI_Win_flush(dest, win);
            }
        }
        MPI_Win_unlock_all(win);

        MPI_Barrier(MPI_COMM_WORLD);

        if (rank != dest) {
            /* check results on P0 and P2 (origin) */
            if (rank == origin_am) {
                MPI_Send(result_buf, AM_BUF_SIZE, MPI_INT, origin_shm, CHECK_TAG, MPI_COMM_WORLD);
            }
            else if (rank == origin_shm) {
                MPI_Alloc_mem(sizeof(int) * AM_BUF_SIZE, MPI_INFO_NULL, &check_buf);
                MPI_Recv(check_buf, AM_BUF_SIZE, MPI_INT, origin_am, CHECK_TAG, MPI_COMM_WORLD,
                         &status);
                for (i = 0; i < AM_BUF_SIZE; i++) {
                    for (j = 0; j < SHM_BUF_SIZE; j++) {
                        if (check_buf[i] == result_buf[j]) {
                            printf
                                ("LOOP=%d, rank=%d, FOP, both check_buf[%d] and result_buf[%d] equal to %d, expected to be different. \n",
                                 k, rank, i, j, check_buf[i]);
                            errors++;
                        }
                    }
                }
                MPI_Free_mem(check_buf);
            }
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            /* check results on P1 */
            if (target_buf[0] != AM_BUF_SIZE + SHM_BUF_SIZE) {
                printf("LOOP=%d, rank=%d, FOP, target_buf[0] = %d, expected %d. \n",
                       k, rank, target_buf[0], AM_BUF_SIZE + SHM_BUF_SIZE);
                errors++;
            }
            MPI_Win_unlock(rank, win);
        }
    }

    MPI_Win_free(&win);

    if (rank == origin_am || rank == origin_shm) {
        MPI_Free_mem(orig_buf);
        MPI_Free_mem(result_buf);
    }

  exit_test:
    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();
    return 0;
}
