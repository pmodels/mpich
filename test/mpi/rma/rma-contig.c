/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <mpi.h>

#define MAX_DATA_SIZE   (1024*128*16)
#define MAX_NUM_ITERATIONS (8192*4)
#define MIN_NUM_ITERATIONS 8
#define NUM_WARMUP_ITER 1

const int verbose = 0;
static int rank;

void run_test(int lock_mode, int lock_assert)
{
    int nproc, test_iter, target_rank, data_size;
    int *buf, *win_buf;
    MPI_Win win;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (rank == 0 && verbose) {
        printf("Starting one-sided contiguous performance test with %d processes\n", nproc);

        printf("Synchronization mode: ");

        switch (lock_mode) {
        case MPI_LOCK_EXCLUSIVE:
            printf("Exclusive lock");
            break;
        case MPI_LOCK_SHARED:
            printf("Shared lock");
            break;
        default:
            printf("Unknown lock");
            break;
        }

        if (lock_assert & MPI_MODE_NOCHECK)
            printf(", MPI_MODE_NOCHECK");

        printf("\n");
    }

    MPI_Alloc_mem(MAX_DATA_SIZE, MPI_INFO_NULL, &buf);
    MPI_Alloc_mem(MAX_DATA_SIZE, MPI_INFO_NULL, &win_buf);
    memset(buf, rank, MAX_DATA_SIZE);
    memset(win_buf, rank, MAX_DATA_SIZE);
    MPI_Win_create(win_buf, MAX_DATA_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    if (rank == 0 && verbose)
        printf("%12s %12s %12s %12s %12s %12s %12s %12s\n", "Trg. Rank", "Xfer Size",
               "Get (usec)", "Put (usec)", "Acc (usec)",
               "Get (MiB/s)", "Put (MiB/s)", "Acc (MiB/s)");

    for (target_rank = 0; rank == 0 && target_rank < nproc; target_rank++) {
        for (data_size = sizeof(double); data_size <= MAX_DATA_SIZE; data_size *= 2) {
            double t_get, t_put, t_acc;
            int num_iter = MAX_NUM_ITERATIONS;

            /* Scale the number of iterations by log_2 of the data size, so
             * that we run each test for a reasonable amount of time. */
            {
                int t = data_size, my_log2 = 0;
                while (t >>= 1)
                    my_log2++;
                if (my_log2)
                    num_iter = (num_iter / my_log2 < MIN_NUM_ITERATIONS) ?
                        MIN_NUM_ITERATIONS : num_iter / my_log2;
            }

            for (test_iter = 0; test_iter < num_iter + NUM_WARMUP_ITER; test_iter++) {
                if (test_iter == NUM_WARMUP_ITER)
                    t_get = MPI_Wtime();

                MPI_Win_lock(lock_mode, target_rank, lock_assert, win);
                MPI_Get(buf, data_size, MPI_BYTE, target_rank, 0, data_size, MPI_BYTE, win);
                MPI_Win_unlock(target_rank, win);
            }
            t_get = (MPI_Wtime() - t_get) / num_iter;

            for (test_iter = 0; test_iter < num_iter + NUM_WARMUP_ITER; test_iter++) {
                if (test_iter == NUM_WARMUP_ITER)
                    t_put = MPI_Wtime();

                MPI_Win_lock(lock_mode, target_rank, lock_assert, win);
                MPI_Put(buf, data_size, MPI_BYTE, target_rank, 0, data_size, MPI_BYTE, win);
                MPI_Win_unlock(target_rank, win);
            }
            t_put = (MPI_Wtime() - t_put) / num_iter;

            for (test_iter = 0; test_iter < num_iter + NUM_WARMUP_ITER; test_iter++) {
                if (test_iter == NUM_WARMUP_ITER)
                    t_acc = MPI_Wtime();

                MPI_Win_lock(lock_mode, target_rank, lock_assert, win);
                MPI_Accumulate(buf, data_size / sizeof(int), MPI_INT, target_rank,
                               0, data_size / sizeof(int), MPI_INT, MPI_SUM, win);
                MPI_Win_unlock(target_rank, win);
            }
            t_acc = (MPI_Wtime() - t_acc) / num_iter;

            if (rank == 0 && verbose)
                printf("%12d %12d %12.3f %12.3f %12.3f %12.3f %12.3f %12.3f\n", target_rank,
                       data_size, t_get * 1.0e6, t_put * 1.0e6, t_acc * 1.0e6,
                       data_size / (1024.0 * 1024.0) / t_get, data_size / (1024.0 * 1024.0) / t_put,
                       data_size / (1024.0 * 1024.0) / t_acc);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_free(&win);
    MPI_Free_mem(win_buf);
    MPI_Free_mem(buf);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    run_test(MPI_LOCK_EXCLUSIVE, 0);
    run_test(MPI_LOCK_EXCLUSIVE, MPI_MODE_NOCHECK);
    run_test(MPI_LOCK_SHARED, 0);
    run_test(MPI_LOCK_SHARED, MPI_MODE_NOCHECK);

    MPI_Finalize();

    if (rank == 0)
        printf(" No Errors\n");

    return 0;
}
