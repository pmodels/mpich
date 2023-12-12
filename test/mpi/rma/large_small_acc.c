/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This code tests the case when a large ACC is issued, and then
 * several small ACCs is issued between the same origin and target.
 * The purpose of this test is to check if the ordering of ACCs
 * is guaranteed. */

#include "mpitest.h"
#include <stdint.h>

#ifdef MULTI_TESTS
#define run rma_large_small_acc
int run(const char *arg);
#endif

#define LOOP 5
#define DATA_COUNT 8192

int run(const char *arg)
{
    int rank, nprocs;
    MPI_Win win;
    uint64_t buf[DATA_COUNT], orig_buf[DATA_COUNT];
    uint64_t small_orig_buf_1 = 2, small_orig_buf_2[2] = { 3, 3 };
    int i, j, errs = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (j = 0; j < LOOP; j++) {

        errs = 0;

        for (i = 0; i < DATA_COUNT; i++) {
            buf[i] = 0;
            orig_buf[i] = 1;
        }

        MPI_Win_create(buf, sizeof(uint64_t) * DATA_COUNT, sizeof(uint64_t),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        MPI_Win_fence(0, win);

        if (rank == 0) {
            /* ACC (atomic PUT) to win_buf[0...DATA_COUNT-1] */
            MPI_Accumulate(orig_buf, DATA_COUNT, MPI_UINT64_T, 1, 0, DATA_COUNT, MPI_UINT64_T,
                           MPI_REPLACE, win);
            /* ACC (atomic PUT) to win_buf[0] */
            MPI_Accumulate(&small_orig_buf_1, 1, MPI_UINT64_T, 1, 0, 1, MPI_UINT64_T, MPI_REPLACE,
                           win);
            /* ACC (atomic PUT) to win_buf[1,2] */
            MPI_Accumulate(&small_orig_buf_2, 2, MPI_UINT64_T, 1, 1, 2, MPI_UINT64_T, MPI_REPLACE,
                           win);
        }

        MPI_Win_fence(0, win);

        if (rank == 1) {
            for (i = 0; i < DATA_COUNT; i++) {
                if (i == 0) {
                    if (buf[i] != 2) {
                        errs++;
                    }
                } else if (i == 1 || i == 2) {
                    if (buf[i] != 3) {
                        errs++;
                    }
                } else {
                    if (buf[i] != 1) {
                        errs++;
                    }
                }
            }
        }

        MPI_Win_free(&win);
    }

    return errs;
}
