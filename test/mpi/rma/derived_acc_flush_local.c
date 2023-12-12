/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This code tests the case when one process issues large number
 * of MPI_Accumulate operations (with large derived datatype) and
 * issues a MPI_Win_flush_local at end. */

/* FIXME: we should merge this into a comprehensive test for RMA
 * operations + MPI_Win_flush_local. */

#include <time.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_derived_acc_flush_local
int run(const char *arg);
#endif

#define DATA_SIZE 1000000
#define COUNT 5000
#define BLOCKLENGTH (DATA_SIZE/COUNT)
#define STRIDE BLOCKLENGTH
#define OPS_NUM 500

int run(const char *arg)
{
    int rank, nproc;
    int i;
    MPI_Win win;
    int *tar_buf = NULL;
    int *orig_buf = NULL;
    MPI_Datatype derived_dtp;
    int errs = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nproc < 3) {
        fprintf(stderr, "Run this program with at least 3 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Alloc_mem(sizeof(int) * DATA_SIZE, MPI_INFO_NULL, &orig_buf);
    MPI_Alloc_mem(sizeof(int) * DATA_SIZE, MPI_INFO_NULL, &tar_buf);

    for (i = 0; i < DATA_SIZE; i++) {
        orig_buf[i] = 1;
        tar_buf[i] = 0;
    }

    MPI_Type_vector(COUNT, BLOCKLENGTH - 1, STRIDE, MPI_INT, &derived_dtp);
    MPI_Type_commit(&derived_dtp);

    MPI_Win_create(tar_buf, sizeof(int) * DATA_SIZE, sizeof(int),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    /***** test between rank 0 and rank 1 *****/

    if (rank == 1) {
        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);

        for (i = 0; i < OPS_NUM; i++) {
            MPI_Accumulate(orig_buf, 1, derived_dtp,
                           0, 0, DATA_SIZE - COUNT, MPI_INT, MPI_SUM, win);
            MPI_Win_flush_local(0, win);
        }

        MPI_Win_unlock(0, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* check results */
    if (rank == 0) {
        for (i = 0; i < DATA_SIZE - COUNT; i++) {
            if (tar_buf[i] != OPS_NUM) {
                if (errs < 10) {
                    printf("tar_buf[%d] = %d, expected %d\n", i, tar_buf[i], OPS_NUM);
                }
                errs++;
            }
        }
    }

    for (i = 0; i < DATA_SIZE; i++) {
        tar_buf[i] = 0;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /***** test between rank 0 and rank 2 *****/

    if (rank == 2) {
        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);

        for (i = 0; i < OPS_NUM; i++) {
            MPI_Accumulate(orig_buf, 1, derived_dtp,
                           0, 0, DATA_SIZE - COUNT, MPI_INT, MPI_SUM, win);
            MPI_Win_flush_local(0, win);
        }

        MPI_Win_unlock(0, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* check results */
    if (rank == 0) {
        for (i = 0; i < DATA_SIZE - COUNT; i++) {
            if (tar_buf[i] != OPS_NUM) {
                if (errs < 10) {
                    printf("tar_buf[%d] = %d, expected %d\n", i, tar_buf[i], OPS_NUM);
                }
                errs++;
            }
        }
    }

    MPI_Win_free(&win);

    MPI_Type_free(&derived_dtp);

    MPI_Free_mem(orig_buf);
    MPI_Free_mem(tar_buf);

    return errs;
}
