/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* tests send/recv of a message > 8GB.
   run with 3 processes to exercise both shared memory and TCP in Nemesis tests*/

#define CNT_MAX 270000000

int do_test(int rank, long long *cols, MPI_Datatype dtype, int send_cnt, int recv_cnt)
{
    int i;
    int errs = 0;
    int stat_cnt = 0;
    MPI_Status status;

    if (rank == 0) {
        for (i = 0; i < send_cnt; i++)
            cols[i] = i;
        MTestPrintfMsg(1, "[%d] sending...\n", rank);
        MPI_Send(cols, send_cnt, dtype, 1, 0, MPI_COMM_WORLD);
        MPI_Send(cols, send_cnt, dtype, 2, 0, MPI_COMM_WORLD);
    }
    else {
        MTestPrintfMsg(1, "[%d] receiving...\n", rank);
        for (i = 0; i < recv_cnt; i++)
            cols[i] = -1;
        MPI_Recv(cols, recv_cnt, dtype, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status,dtype,&stat_cnt);
        if (send_cnt != stat_cnt) {
            fprintf(stderr, "Output of MPI_Get_count (%d) does not match expected count (%d).\n", stat_cnt, send_cnt);
            errs++;
        }
        for (i = 0; i < send_cnt; i++) {
            if (cols[i] != i) {
                MTestPrintfMsg(1, "Rank %d, cols[i]=%lld, should be %d\n", rank, cols[i], i);
                errs++;
            }
        }
    }

    return errs;
}

int main(int argc, char *argv[])
{
    int size, rank;
    int cnt = CNT_MAX;
    long long *cols;
    int errs = 0;
    MPI_Datatype int256; /* 256-bit integer type */

    MTest_Init(&argc, &argv);

    /* need large memory */
    if (sizeof(void *) < 8) {
        MTest_Finalize(errs);
        MPI_Finalize();
        return 0;
    }

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (size != 3) {
        fprintf(stderr, "[%d] usage: mpiexec -n 3 %s\n", rank, argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    cols = malloc(32ULL * cnt);
    if (cols == NULL) {
        printf("malloc of >8GB array failed\n");
        errs++;
        MTest_Finalize(errs);
        MPI_Finalize();
        return 0;
    }

    MPI_Type_contiguous(8, MPI_INT, &int256);
    MPI_Type_commit(&int256);

    if (rank == 0)
        MTestPrintfMsg(1, "**** Phase 1 : send_cnt = recv_cnt ****\n");

    for (cnt = CNT_MAX/128; cnt <= CNT_MAX; cnt *= 2) {
        MTestPrintfMsg(1, "Testing %10zu bytes send and %10zu bytes receive\n",
                       sizeof(long long) * cnt, sizeof(long long) * cnt);
        errs += do_test(rank, cols, MPI_LONG_LONG_INT, cnt, cnt);
    }
    for (cnt = CNT_MAX/2; cnt <= CNT_MAX; cnt *= 2) {
        MTestPrintfMsg(1, "Testing %10zu bytes send and %10zu bytes receive\n",
                       sizeof(int32_t) * 8 * cnt, sizeof(int32_t) * 8 * cnt);
        errs += do_test(rank, cols, int256, cnt, cnt);
    }

    if (rank == 0)
        MTestPrintfMsg(1, "**** Phase 2 : send_cnt < recv_cnt ****\n");

    for (cnt = CNT_MAX/128; cnt <= CNT_MAX; cnt *= 2) {
        MTestPrintfMsg(1, "Testing %10zu bytes send and %10zu bytes receive\n",
                       sizeof(long long) * cnt / 2, sizeof(long long) * cnt);
        errs += do_test(rank, cols, MPI_LONG_LONG_INT, cnt/2, cnt);
    }
    for (cnt = CNT_MAX/2; cnt <= CNT_MAX; cnt *= 2) {
        MTestPrintfMsg(1, "Testing %10zu bytes send and %10zu bytes receive\n",
                       sizeof(int32_t) * 8 * cnt / 2, sizeof(int32_t) * 8 * cnt);
        errs += do_test(rank, cols, int256, cnt / 2, cnt);
    }

    MPI_Type_free(&int256);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
