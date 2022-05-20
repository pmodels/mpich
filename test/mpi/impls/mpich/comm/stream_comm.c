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

static int test_stream_comm(MPIX_Stream stream);

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

    MPIX_Stream stream;
    mpi_errno = MPIX_Stream_create(MPI_INFO_NULL, &stream);
    if (mpi_errno != MPI_SUCCESS) {
        MTestPrintfMsg(1, "MPIX_Stream_create returns error: %x\n", mpi_errno);
        goto fn_exit;
    }

    /* Test stream to stream send/recv */
    errs += test_stream_comm(stream);

    /* Test stream to MPIX_STREAM_NULL send/recv */
    if (rank == 0) {
        errs += test_stream_comm(stream);
    } else {
        errs += test_stream_comm(MPIX_STREAM_NULL);
    }

    MPIX_Stream_free(&stream);

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

static int test_stream_comm(MPIX_Stream stream)
{
    int errs = 0;
    int mpi_errno;

    MPI_Comm stream_comm;
    mpi_errno = MPIX_Stream_comm_create(MPI_COMM_WORLD, stream, &stream_comm);
    CHECK_MPI_ERROR;

    int src = 0;
    int dst = 1;
    int tag = 1;
#define DATA_VAL 42

    /* test send/recv */
    if (rank == src) {
        int data = DATA_VAL;
        mpi_errno = MPI_Send(&data, 1, MPI_INT, dst, tag, stream_comm);
        CHECK_MPI_ERROR;

        data = -1;
        mpi_errno = MPI_Recv(&data, 1, MPI_INT, dst, tag, stream_comm, MPI_STATUS_IGNORE);
        CHECK_MPI_ERROR;
        if (data != DATA_VAL) {
            printf("Expect receive data = %d, got %d\n", DATA_VAL, data);
            errs++;
        }
    } else if (rank == dst) {
        int data = -1;
        mpi_errno = MPI_Recv(&data, 1, MPI_INT, src, tag, stream_comm, MPI_STATUS_IGNORE);
        CHECK_MPI_ERROR;
        if (data != DATA_VAL) {
            printf("Expect receive data = %d, got %d\n", DATA_VAL, data);
            errs++;
        }

        mpi_errno = MPI_Send(&data, 1, MPI_INT, src, tag, stream_comm);
        CHECK_MPI_ERROR;
    }

    /* test coll (MPI_Allreduce) */
    errs += MTestTestIntracomm(stream_comm);

    MPI_Comm_free(&stream_comm);

  fn_exit:
    return errs;
}
