/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_SIZE 10000
#define SMALL 1
#define LARGE 10000
#define TAG   1234

#define IS_UNEXP  0x1
#define IS_ANYSRC 0x2
#define IS_ANYTAG 0x4
#define IS_SSEND  0x8
#define MAX_FLAG  0xf

static MPI_Comm comm = MPI_COMM_WORLD;
static int buf[MAX_SIZE];

int test_send(int rank, int src, int dst, int count, int flag);

int main(int argc, char *argv[])
{
    int errs = 0;
    int elems = 20;
    int rank, size;
    MPI_Request *reqs;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    if (size != 3) {
        printf("This test require at least 3 processes\n");
        goto fn_exit;
    }

    int dst_list[] = { 1, 2 };
    int cnt_list[] = { SMALL, LARGE };
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int flag = 0; flag <= MAX_FLAG; flag++) {
                errs += test_send(rank, 0, dst_list[i], cnt_list[j], flag);
            }
        }
    }

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

#define DUMP_TEST_SUBJECT \
do { \
    if (rank == src) { \
        char msg[100]; \
        sprintf(msg, "TEST send %d -> %d, count = %5d -", src, dst, count); \
        if (flag & IS_UNEXP) { \
            strcat(msg, " UNEXP"); \
        } \
        if (flag & IS_ANYSRC) { \
            strcat(msg, " ANYSRC"); \
        } \
        if (flag & IS_ANYTAG) { \
            strcat(msg, " ANYTAG"); \
        } \
        if (flag & IS_SSEND) { \
            strcat(msg, " SSEND"); \
        } \
        MTestPrintfMsg(1, "%s\n", msg); \
    } \
} while (0)


int test_send(int rank, int src, int dst, int count, int flag)
{
    int errs = 0;

    DUMP_TEST_SUBJECT;

    MPI_Request req;
    if (rank == src) {
        if (!(flag & IS_UNEXP)) {
            MPI_Barrier(comm);
        }
        if (flag & IS_SSEND) {
            MPI_Issend(buf, count, MPI_INT, dst, TAG, comm, &req);
        } else {
            MPI_Isend(buf, count, MPI_INT, dst, TAG, comm, &req);
        }
        if (flag & IS_UNEXP) {
            MPI_Barrier(comm);
        }
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else if (rank == dst) {
        if (flag & IS_UNEXP) {
            MPI_Barrier(comm);
        }

        int src_rank = src;
        if (flag & IS_ANYSRC) {
            src_rank = MPI_ANY_SOURCE;
        }
        int tag = TAG;
        if (flag & IS_ANYTAG) {
            tag = MPI_ANY_TAG;
        }

        MPI_Irecv(buf, count, MPI_INT, src_rank, tag, comm, &req);

        if (!(flag & IS_UNEXP)) {
            MPI_Barrier(comm);
        }

        MPI_Status status;
        MPI_Wait(&req, &status);
        if (status.MPI_SOURCE != src) {
            errs++;
            printf(" - expect status.MPI_SOURCE %d, got %d\n", src, status.MPI_SOURCE);
        }
        if (status.MPI_TAG != TAG) {
            errs++;
            printf(" - expect status.MPI_TAG %d, got %d\n", TAG, status.MPI_TAG);
        }
    } else {
        MPI_Barrier(comm);
    }
    return errs;
}
