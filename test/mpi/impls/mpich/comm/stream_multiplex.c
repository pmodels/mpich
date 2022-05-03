/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <mpi.h>

#define CHECK_MPI_ERROR \
    do { \
        if (mpi_errno != MPI_SUCCESS) { \
            MTestPrintError(mpi_errno); \
            errs++; \
            goto fn_exit; \
        } \
    } while (0)

int rank, size;

static int test_stream_send(MPIX_Stream stream, int src_idx, int dst_idx);

int main(int argc, char **argv)
{
    int errs = 0;
    int mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("This test requires at least 2 processes\n");
        errs++;
        goto fn_exit;
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

#define N 4
    MPIX_Stream streams[N + 1];
    for (int i = 0; i < N; i++) {
        mpi_errno = MPIX_Stream_create(MPI_INFO_NULL, &streams[i]);
        if (mpi_errno != MPI_SUCCESS) {
            MTestPrintfMsg(1, "MPIX_Stream_create returns error: %x\n", mpi_errno);
            goto fn_exit;
        }
    }
    streams[N] = MPIX_STREAM_NULL;

    MPI_Comm stream_comm;
    mpi_errno = MPIX_Stream_comm_create_multiplex(MPI_COMM_WORLD, N + 1, streams, &stream_comm);
    CHECK_MPI_ERROR;

    /* Test stream to stream send/recv */
    for (int i = 0; i < N + 1; i++) {
        errs += test_stream_send(stream_comm, 0, i);
        errs += test_stream_send(stream_comm, i, 0);
    }

    /* test coll (MPI_Allreduce) */
    errs += MTestTestIntracomm(stream_comm);

    mpi_errno = MPI_Comm_free(&stream_comm);
    CHECK_MPI_ERROR;

    for (int i = 0; i < N; i++) {
        mpi_errno = MPIX_Stream_free(&streams[i]);
        CHECK_MPI_ERROR;
    }

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

static int test_stream_send(MPI_Comm stream_comm, int src_idx, int dst_idx)
{
    int errs = 0;
    int mpi_errno;

    int src = 0;
    int dst = 1;
    int tag = 1;
#define DATA_VAL 42

    MTestPrintfMsg(1, "Test Multiplex Stream Send: src_idx = %d, dst_idx=%d\n", src_idx, dst_idx);
    /* test send/recv */
    if (rank == src) {
        MPI_Request reqs[2];
        int data = DATA_VAL;
        mpi_errno = MPIX_Stream_isend(&data, 1, MPI_INT, dst, tag, stream_comm,
                                      src_idx, dst_idx, &reqs[0]);
        CHECK_MPI_ERROR;

        data = -1;
        mpi_errno = MPIX_Stream_irecv(&data, 1, MPI_INT, dst, tag, stream_comm,
                                      src_idx, dst_idx, &reqs[1]);
        CHECK_MPI_ERROR;

        mpi_errno = MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
        CHECK_MPI_ERROR;

        if (data != DATA_VAL) {
            printf("Expect receive data = %d, got %d\n", DATA_VAL, data);
            errs++;
        }
    } else if (rank == dst) {
        int data = -1;
        mpi_errno = MPIX_Stream_recv(&data, 1, MPI_INT, src, tag, stream_comm,
                                     src_idx, dst_idx, MPI_STATUS_IGNORE);
        CHECK_MPI_ERROR;
        if (data != DATA_VAL) {
            printf("Expect receive data = %d, got %d\n", DATA_VAL, data);
            errs++;
        }

        mpi_errno = MPIX_Stream_send(&data, 1, MPI_INT, src, tag, stream_comm, src_idx, dst_idx);
        CHECK_MPI_ERROR;
    }


  fn_exit:
    return errs;
}
