/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test is going to test atomic GET (GACC/FOP+MPI_NO_OP).
 *
 * There are totally three processes involved in this test. Both
 * rank 1 and rank 2 issue RMA operations to rank 0. Rank 2 issues
 * atomic PUT (ACC+MPI_REPLACE), whereas rank 1 issues atomic
 * GET (GACC/FOP+MPI_NO_OP). The datatype used in the test is
 * pair-type. The initial value of pair-type data in origin buffer
 * of atomic PUT is set so that the two basic values equal with
 * each other. Due to the atomicity of GET, the expected resulting
 * data should also have equivalent basic values. */

#include "mpi.h"
#include <stdio.h>

#define LOOP 100
#define DATA_SIZE 100
#define OPS_NUM 10000
#define GACC_SZ 10

typedef struct pair_struct {
    long double a;
    int b;
} pair_struct_t;

int main(int argc, char *argv[])
{
    int rank, nproc;
    int i, j, k;
    int errors = 0, curr_errors = 0;
    MPI_Win win;
    pair_struct_t *tar_buf = NULL;
    pair_struct_t *orig_buf = NULL;
    pair_struct_t *result_buf = NULL;

    /* This test needs to work with 3 processes. */

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Alloc_mem(sizeof(pair_struct_t) * DATA_SIZE, MPI_INFO_NULL, &orig_buf);
    MPI_Alloc_mem(sizeof(pair_struct_t) * DATA_SIZE, MPI_INFO_NULL, &result_buf);

    MPI_Win_allocate(sizeof(pair_struct_t) * DATA_SIZE, sizeof(pair_struct_t),
                     MPI_INFO_NULL, MPI_COMM_WORLD, &tar_buf, &win);

    for (j = 0; j < LOOP * 6; j++) {

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);

        /* initialize data */
        for (i = 0; i < DATA_SIZE; i++) {
            tar_buf[i].a = 0.0;
            tar_buf[i].b = 0;
            result_buf[i].a = 0.0;
            result_buf[i].b = 0;
        }

        MPI_Win_unlock(rank, win);

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Win_fence(0, win);

        if (rank == 2) {
            if (j < 2 * LOOP) {
                /* Work with FOP test (Test #1 to Test #2) */
                for (i = 0; i < OPS_NUM; i++) {

                    int curr_val = j * OPS_NUM + i;
                    orig_buf[0].a = (long double) (curr_val);
                    orig_buf[0].b = curr_val;

                    MPI_Accumulate(orig_buf, 1, MPI_LONG_DOUBLE_INT,
                                   0, 0, 1, MPI_LONG_DOUBLE_INT, MPI_REPLACE, win);
                }
            }
            else {
                /* Work with GACC test (Test #3 to Test #6) */
                for (i = 0; i < OPS_NUM / GACC_SZ; i++) {

                    for (k = 0; k < GACC_SZ; k++) {
                        int curr_val = j * OPS_NUM + i * GACC_SZ + k;
                        orig_buf[k].a = (long double) (curr_val);
                        orig_buf[k].b = curr_val;
                    }

                    MPI_Accumulate(orig_buf, GACC_SZ, MPI_LONG_DOUBLE_INT,
                                   0, 0, GACC_SZ, MPI_LONG_DOUBLE_INT, MPI_REPLACE, win);
                }
            }
        }
        else if (rank == 1) {
            /* equals to an atomic GET */
            if (j < LOOP) {
                for (i = 0; i < DATA_SIZE; i++) {
                    /* Test #1: FOP + MPI_NO_OP */
                    MPI_Fetch_and_op(orig_buf, &(result_buf[i]), MPI_LONG_DOUBLE_INT,
                                     0, 0, MPI_NO_OP, win);
                }
            }
            else if (j < 2 * LOOP) {
                for (i = 0; i < DATA_SIZE; i++) {
                    /* Test #2: FOP + MPI_NO_OP + NULL origin buffer address */
                    MPI_Fetch_and_op(NULL, &(result_buf[i]), MPI_LONG_DOUBLE_INT,
                                     0, 0, MPI_NO_OP, win);
                }
            }
            else if (j < 3 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #3: GACC + MPI_NO_OP */
                    MPI_Get_accumulate(orig_buf, GACC_SZ, MPI_LONG_DOUBLE_INT,
                                       &(result_buf[i * GACC_SZ]), GACC_SZ, MPI_LONG_DOUBLE_INT,
                                       0, 0, GACC_SZ, MPI_LONG_DOUBLE_INT, MPI_NO_OP, win);
                }
            }
            else if (j < 4 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #4: GACC + MPI_NO_OP + NULL origin buffer address */
                    MPI_Get_accumulate(NULL, GACC_SZ, MPI_LONG_DOUBLE_INT,
                                       &(result_buf[i * GACC_SZ]), GACC_SZ, MPI_LONG_DOUBLE_INT,
                                       0, 0, GACC_SZ, MPI_LONG_DOUBLE_INT, MPI_NO_OP, win);
                }
            }
            else if (j < 5 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #5: GACC + MPI_NO_OP + zero origin count */
                    MPI_Get_accumulate(orig_buf, 0, MPI_LONG_DOUBLE_INT,
                                       &(result_buf[i * GACC_SZ]), GACC_SZ, MPI_LONG_DOUBLE_INT,
                                       0, 0, GACC_SZ, MPI_LONG_DOUBLE_INT, MPI_NO_OP, win);
                }
            }
            else if (j < 6 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #5: GACC + MPI_NO_OP + NULL origin datatype */
                    MPI_Get_accumulate(orig_buf, GACC_SZ, MPI_DATATYPE_NULL,
                                       &(result_buf[i * GACC_SZ]), GACC_SZ, MPI_LONG_DOUBLE_INT,
                                       0, 0, GACC_SZ, MPI_LONG_DOUBLE_INT, MPI_NO_OP, win);
                }
            }
        }

        MPI_Win_fence(0, win);

        /* check results */
        if (rank == 1) {
            for (i = 0; i < DATA_SIZE; i++) {
                if (result_buf[i].a != (long double) (result_buf[i].b)) {
                    if (curr_errors < 10) {
                        printf("LOOP %d: result_buf[%d].a = %Lf, result_buf[%d].b = %d\n",
                               j, i, result_buf[i].a, i, result_buf[i].b);
                    }
                    curr_errors++;
                }
            }
        }

        if (j % LOOP == 0) {
            errors += curr_errors;
            curr_errors = 0;
        }
    }

    MPI_Win_free(&win);

    MPI_Free_mem(orig_buf);
    MPI_Free_mem(result_buf);

    if (rank == 1) {
        if (errors == 0)
            printf(" No Errors\n");
    }

    MPI_Finalize();
    return 0;
}
