/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test is going to test the atomicity for "read-modify-write" in CAS
 * operations */

/* There are three processes involved in this test: P0 (origin_shm), P1 (origin_am),
 * and P2 (dest). P0 and P1 issues one CAS to P2 via SHM and AM respectively.
 * For P0, origin value is 1 and compare value is 0; for P1, origin value is 0 and
 * compare value is 1; for P2, initial target value is 0. The correct results can
 * only be one of the following cases:
 *
 *   (1) result value on P0: 0, result value on P1: 0, target value on P2: 1.
 *   (2) result value on P0: 0, result value on P1: 1, target value on P2: 0.
 *
 * All other results are not correct. */

#include "mpi.h"
#include <stdio.h>

#define LOOP_SIZE 10000
#define CHECK_TAG 123

int main(int argc, char *argv[])
{
    int rank, size, k;
    int errors = 0;
    int origin_shm, origin_am, dest;
    int *orig_buf = NULL, *result_buf = NULL, *compare_buf = NULL,
        *target_buf = NULL, *check_buf = NULL;
    int target_value = 0;
    MPI_Win win;

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

    if (rank != dest) {
        MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &orig_buf);
        MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &result_buf);
        MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &compare_buf);
    }

    MPI_Win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &target_buf, &win);

    for (k = 0; k < LOOP_SIZE; k++) {

        /* init buffers */
        if (rank == origin_shm) {
            orig_buf[0] = 1;
            compare_buf[0] = 0;
            result_buf[0] = 0;
        }
        else if (rank == origin_am) {
            orig_buf[0] = 0;
            compare_buf[0] = 1;
            result_buf[0] = 0;
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            target_buf[0] = 0;
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        /* perform FOP */
        MPI_Win_lock_all(0, win);
        if (rank != dest) {
            MPI_Compare_and_swap(orig_buf, compare_buf, result_buf, MPI_INT, dest, 0, win);
            MPI_Win_flush(dest, win);
        }
        MPI_Win_unlock_all(win);

        MPI_Barrier(MPI_COMM_WORLD);

        /* check results */
        if (rank != dest) {
            MPI_Gather(result_buf, 1, MPI_INT, check_buf, 1, MPI_INT, dest, MPI_COMM_WORLD);
        }
        else {
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            target_value = target_buf[0];
            MPI_Win_unlock(rank, win);

            MPI_Alloc_mem(sizeof(int) * 3, MPI_INFO_NULL, &check_buf);
            MPI_Gather(&target_value, 1, MPI_INT, check_buf, 1, MPI_INT, dest, MPI_COMM_WORLD);

            if (!(check_buf[dest] == 0 && check_buf[origin_shm] == 0 && check_buf[origin_am] == 1)
                && !(check_buf[dest] == 1 && check_buf[origin_shm] == 0 &&
                     check_buf[origin_am] == 0)) {

                printf
                    ("Wrong results: target result = %d, origin_shm result = %d, origin_am result = %d\n",
                     check_buf[dest], check_buf[origin_shm], check_buf[origin_am]);

                printf
                    ("Expected results (1): target result = 1, origin_shm result = 0, origin_am result = 0\n");
                printf
                    ("Expected results (2): target result = 0, origin_shm result = 0, origin_am result = 1\n");

                errors++;
            }

            MPI_Free_mem(check_buf);
        }
    }

    MPI_Win_free(&win);

    if (rank == origin_am || rank == origin_shm) {
        MPI_Free_mem(orig_buf);
        MPI_Free_mem(result_buf);
        MPI_Free_mem(compare_buf);
    }

  exit_test:
    if (rank == dest && errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();
    return 0;
}
