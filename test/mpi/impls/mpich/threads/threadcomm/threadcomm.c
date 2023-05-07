/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

static int do_restart = 0;
static int do_commdup = 0;

#define MAX_COUNT 10000
enum op {
    DO_SEND,
    DO_ISEND,
    DO_RECV,
    DO_IRECV,
};

static void test_sendrecv(MPI_Comm comm, int src, int dst, int count,
                          int send_op, int recv_op, bool check_status);

static MTEST_THREAD_RETURN_TYPE test_thread(void *arg)
{
    MPI_Comm comm = *(MPI_Comm *) arg;

    for (int rep = 0; rep < 1 + do_restart; rep++) {
        MPIX_Threadcomm_start(comm);

        MPI_Comm use_comm = comm;
        if (do_commdup) {
            MPI_Comm_dup(comm, &use_comm);
        }

        int rank, size;
        MPI_Comm_size(use_comm, &size);
        MPI_Comm_rank(use_comm, &rank);
        MTestPrintfMsg(1, "Rank %d / %d\n", rank, size);

        for (int src = 0; src < size; src++) {
            for (int dst = 0; dst < size; dst++) {
                for (int count = 1; count < MAX_COUNT; count *= 8) {
                    test_sendrecv(use_comm, src, dst, count, DO_SEND, DO_RECV, true);
                    test_sendrecv(use_comm, src, dst, count, DO_ISEND, DO_IRECV, true);
                    test_sendrecv(use_comm, src, dst, count, DO_SEND, DO_RECV, false);
                    test_sendrecv(use_comm, src, dst, count, DO_ISEND, DO_IRECV, false);
                }
            }
        }

        if (do_commdup) {
            MPI_Comm_free(&use_comm);
        }
        MPIX_Threadcomm_finish(comm);
    }
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int nthreads = 4;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    nthreads = MTestArgListGetInt_with_default(head, "nthreads", 4);
    do_restart = MTestArgListGetInt_with_default(head, "restart", 0);
    do_commdup = MTestArgListGetInt_with_default(head, "commdup", 0);
    MTestArgListDestroy(head);

    MTest_Init(NULL, NULL);

    MTestPrintfMsg(1, "Test Threadcomm: nthreads=%d, restart=%d, commdup=%d\n",
                   nthreads, do_restart, do_commdup);

    MPI_Comm comm;
    MPIX_Threadcomm_init(MPI_COMM_WORLD, nthreads, &comm);
    for (int i = 0; i < nthreads; i++) {
        MTest_Start_thread(test_thread, &comm);
    }
    MTest_Join_threads();
    MPIX_Threadcomm_free(&comm);

    MTest_Finalize(errs);
    return errs;
}

static void test_sendrecv(MPI_Comm comm, int src, int dst, int count,
                          int send_op, int recv_op, bool check_status)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    int tag = 9;
    int buf[MAX_COUNT];

    if (src == dst) {
        return;
    }

    MTestPrintfMsg(1, "test_sendrecv: %d -> %d, count=%d, send_op=%d, recv_op=%d, status:%d\n",
                   src, dst, count, send_op, recv_op, check_status);

    if (rank == src) {
        for (int i = 0; i < count; i++) {
            buf[i] = i;
        }
        if (send_op == DO_SEND) {
            MPI_Send(buf, count, MPI_INT, dst, tag, comm);
        } else if (send_op == DO_ISEND) {
            MPI_Request req;
            MPI_Isend(buf, count, MPI_INT, dst, tag, comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }
    }
    if (rank == dst) {
        MPI_Status static_status;
        MPI_Status *status = &static_status;
        if (!check_status) {
            status = MPI_STATUS_IGNORE;
        }

        if (recv_op == DO_RECV) {
            MPI_Recv(buf, count, MPI_INT, src, tag, comm, status);
        } else if (recv_op == DO_IRECV) {
            MPI_Request req;
            MPI_Irecv(buf, count, MPI_INT, src, tag, comm, &req);
            MPI_Wait(&req, status);
        }

        int errs = 0;
        for (int i = 0; i < count; i++) {
            if (buf[i] != i) {
                errs++;
            }
        }
        if (errs > 0) {
            printf("sendrecv %d -> %d, data error %d / %d\n", src, dst, errs, count);
        }

        if (check_status) {
            if (status->MPI_SOURCE != src) {
                printf("expect status->MPI_SOURCE = %d, got %d\n", src, status->MPI_SOURCE);
            }
            if (status->MPI_TAG != tag) {
                printf("expect status->MPI_TAG = %d, got %d\n", tag, status->MPI_TAG);
            }
        }
    }
}
