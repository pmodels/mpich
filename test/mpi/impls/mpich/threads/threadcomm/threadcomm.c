/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

static void test_allreduce(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int sum;
    MPI_Allreduce(&rank, &sum, 1, MPI_INT, MPI_SUM, comm);

    if (sum != size * (size - 1) / 2) {
        printf("[%d] test_allreduce: expect %d, got %d\n", rank, size * (size - 1) / 2, sum);
    }
}

static void test_bcast(MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int n = 0;
    if (rank == 0) {
        n = 42;
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, comm);

    if (n != 42) {
        printf("[%d] test_bcast: expect %d, got %d\n", rank, 42, n);
    }
}

static void test_sendrecv(MPI_Comm comm, int src, int dst)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    int tag = 9;

    if (rank == src) {
        int n = 42;
        MPI_Send(&n, 1, MPI_INT, dst, tag, comm);
        MTestPrintfMsg(1, "[%d] test_sendrecv: sent n=%d\n", rank, n);
    }
    if (rank == dst) {
        int ans = 0;
        MPI_Status status;
        MPI_Recv(&ans, 1, MPI_INT, src, tag, comm, &status);
        MTestPrintfMsg(1, "[%d] test_sendrecv: received ans=%d from %d, tag=%d\n", rank, ans,
                       status.MPI_SOURCE, status.MPI_TAG);
        if (ans != 42) {
            printf("[%d] test_sendrecv: expect ans = 42, got %d\n", rank, ans);
        }
    }
}

static void test_isendirecv(MPI_Comm comm, int src, int dst)
{
    int rank;
    MPI_Comm_rank(comm, &rank);

    int tag = 9;

    if (rank == src) {
        int n = 42;
        MPI_Request req;
        MPI_Isend(&n, 1, MPI_INT, dst, tag, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        MTestPrintfMsg(1, "[%d] test_isendirecv: sent n=%d\n", rank, n);
    } else if (rank == dst) {
        int ans;
        MPI_Status status;
        MPI_Request req;
        MPI_Irecv(&ans, 1, MPI_INT, src, tag, comm, &req);
        MPI_Wait(&req, &status);
        MTestPrintfMsg(1, "[%d] test_isendirecv: received ans=%d from %d, tag=%d\n", rank, ans,
                       status.MPI_SOURCE, status.MPI_TAG);
        if (ans != 42) {
            printf("[%d] test_isendirecv: expect ans = 42, got %d\n", rank, ans);
        }
    }
}

static MTEST_THREAD_RETURN_TYPE test_thread(void *arg)
{
    MPI_Comm comm = *(MPI_Comm *) arg;
    MPIX_Threadcomm_start(comm);

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    MTestPrintfMsg(1, "Rank %d / %d\n", rank, size);

    test_sendrecv(comm, 0, size - 1);
    test_isendirecv(comm, 1, 0);
    test_allreduce(comm);
    test_bcast(comm);

    MPIX_Threadcomm_finish(comm);
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int nthreads = 4;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    nthreads = MTestArgListGetInt_with_default(head, "nthreads", 4);

    int provided;
    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    assert(provided == MPI_THREAD_MULTIPLE);

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
