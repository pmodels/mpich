/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <mpi.h>
#include "mpitest.h"

#define GET_BUF_SIZE 2147450880
#define GET_SIGN_BUF_SIZE 16384 /* last MPI_Get data size */
#define ACC_BUF_SIZE 32768
#define SIGNATURE 0x77777777
#define ITERATION 10
#define WIN_BUF_VALUE 0xaaaaaaaa

/* This program tries to mimic the case where P0 issues rdma-get to remote
 * P1, P2 issues AM-based accumulate to P0, P0 and P2 call MPI_Win_flush.
 * MPI_Get will increase OFI internal rma counter of P0; during the call of
 * MPIDI_OFI_PROGRESS, P0 may receive AM-based accumulate message from P2 and
 * increase rma counter; however, after MPIDI_OFI_PROGRESS, issued_cntr (tcount) is
 * not updated timely. This can cause incorrect exit from MPI_Win_flush with
 * incomplete issued MPI_Get.
 * In this test, we delay the handling of rdma-get on P1 by sleeping for 1 second,
 * so it would allow accumulate issued by P2 to complete first and trigger issue_cntr
 * update bug.
 * The test assumes P0 is on node 0, and all others are on other nodes, so at least
 * two nodes and three processes are required.
 */

int main(int argc, char *argv[])
{
    int rank, nprocs, target;
    int *win_ptr, *acc_buf, *get_buf;
    int acc_buf_cnt, get_buf_cnt;
    int *get_sign_buf;
    MPI_Win win;
    int get_cnt = 10;
    int acc_cnt = 10;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs < 3) {
        errs++;
        printf("expected number of processes are at least 3\n");
        goto exit;
    }

    if (rank == 0) {    /* rank 0 issues GET to rank 1 */
        target = 1;
    } else if (rank == 2) {     /* rank 2 issues ACC to rank 0 */
        target = 0;
    }

    if (rank < 3) {
        MPI_Win_allocate(GET_BUF_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, (void **) &win_ptr, &win);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
        get_buf_cnt = GET_BUF_SIZE / sizeof(int);
        for (int i = 0; i < get_buf_cnt; ++i) {
            win_ptr[i] = WIN_BUF_VALUE;
        }
        MPI_Win_unlock(rank, win);
        MPI_Barrier(MPI_COMM_WORLD);

        acc_buf = (int *) malloc(ACC_BUF_SIZE);
        memset(acc_buf, 0x01, ACC_BUF_SIZE);

        get_buf = (int *) malloc(GET_BUF_SIZE);
        memset(get_buf, 0, GET_BUF_SIZE);

        get_sign_buf = (int *) malloc(GET_SIGN_BUF_SIZE);
        memset(get_sign_buf, 0, GET_SIGN_BUF_SIZE);

        MPI_Win_lock_all(MPI_MODE_NOCHECK, win);

        for (int i = 0; i < ITERATION; ++i) {
            if (rank == 0) {
                *get_sign_buf = SIGNATURE;
                for (int j = 0; j < get_cnt; ++j)
                    MPI_Get(get_buf, GET_BUF_SIZE, MPI_CHAR, target, 0,
                            GET_BUF_SIZE, MPI_CHAR, win);
                MPI_Get(get_sign_buf, GET_SIGN_BUF_SIZE, MPI_CHAR, target,
                        0, GET_SIGN_BUF_SIZE, MPI_CHAR, win);
                MPI_Win_flush(target, win);
            } else if (rank == 2) {
                for (int j = 0; j < acc_cnt; ++j)
                    MPI_Accumulate(acc_buf, ACC_BUF_SIZE, MPI_CHAR, target, 0, ACC_BUF_SIZE,
                                   MPI_CHAR, MPI_SUM, win);
                MPI_Win_flush(target, win);
            } else {
                /* delay response to MPI_Get issued by rank 0 */
                sleep(1);
            }

            if (rank == 0) {
                /* if the last MPI_Get is not complete but process returns from MPI_Win_flush, signature
                 * will not be updated to WIN_BUF_VALUE, and the error is detected. */
                if (*get_sign_buf != WIN_BUF_VALUE) {
                    errs++;
                    printf("Iteration[%d] - expected buffer signature = 0x%x, got 0x%x\n", i,
                           WIN_BUF_VALUE, *get_sign_buf);
                }
            }
        }

        MPI_Win_unlock_all(win);

        free(acc_buf);
        free(get_buf);
        free(get_sign_buf);
    } else {
        MPI_Win_allocate(0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, (void **) &win_ptr, &win);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Win_free(&win);

  exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
