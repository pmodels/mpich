/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This code tests the case when origin process issues 10 ACC
 * operations for each data size to the target process, and each
 * operation is followed by a MPI_Win_flush_local. */

/* FIXME: we should merge this into a comprehensive test for RMA
 * operations + MPI_Win_flush_local. */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_DATA_SIZE (262144)
#define MAX_DATA_SIZE (8 * 262144)
#define OPS_NUM 10
#define LOOP 500

int main(int argc, char *argv[])
{
    int rank, nproc, i, j;
    MPI_Win win;
    int *tar_buf = NULL;
    int *orig_buf = NULL;
    int data_size;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Alloc_mem(MAX_DATA_SIZE, MPI_INFO_NULL, &orig_buf);
    MPI_Alloc_mem(MAX_DATA_SIZE, MPI_INFO_NULL, &tar_buf);

    /* run this test for LOOP times */

    for (j = 0; j < LOOP; j++) {

        MPI_Win_create(tar_buf, MAX_DATA_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        MPI_Win_lock_all(0, win);

        if (rank != 0) {
            for (data_size = MIN_DATA_SIZE; data_size <= MAX_DATA_SIZE; data_size *= 2) {
                for (i = 0; i < OPS_NUM; i++) {
                    MPI_Accumulate(orig_buf, data_size, MPI_BYTE,
                                   0, 0, data_size, MPI_BYTE, MPI_SUM, win);
                    MPI_Win_flush_local(0, win);
                }
                MPI_Win_flush(0, win);
            }
        }

        MPI_Win_unlock_all(win);

        MPI_Win_free(&win);
    }

    if (rank == 0)
        printf(" No Errors\n");

    MPI_Free_mem(orig_buf);
    MPI_Free_mem(tar_buf);

    MPI_Finalize();

    return 0;
}
