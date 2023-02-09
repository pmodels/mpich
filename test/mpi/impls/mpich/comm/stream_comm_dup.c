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

static int test_stream_comm_dup_single(MPIX_Stream stream);
static int test_stream_comm_idup_multiplex(MPIX_Stream stream);

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

    errs += test_stream_comm_dup_single(stream);
    errs += test_stream_comm_idup_multiplex(stream);

    mpi_errno = MPIX_Stream_free(&stream);
    CHECK_MPI_ERROR;

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

static int test_stream_comm_dup_single(MPIX_Stream stream)
{
    int errs = 0;
    int mpi_errno;

    MPI_Comm stream_comm, dup_comm;
    MPIX_Stream stream_check;

    mpi_errno = MPIX_Stream_comm_create(MPI_COMM_WORLD, stream, &stream_comm);
    CHECK_MPI_ERROR;

    mpi_errno = MPI_Comm_dup(stream_comm, &dup_comm);
    CHECK_MPI_ERROR;

    MPIX_Comm_get_stream(dup_comm, 0, &stream_check);
    if (stream_check != stream) {
        printf("MPIX_Comm_get_stream did not return expected stream\n");
        errs++;
    }

    mpi_errno = MPI_Comm_free(&dup_comm);
    CHECK_MPI_ERROR;

    mpi_errno = MPI_Comm_free(&stream_comm);
    CHECK_MPI_ERROR;

  fn_exit:
    return errs;
}

static int test_stream_comm_idup_multiplex(MPIX_Stream stream)
{
    int errs = 0;
    int mpi_errno;

    MPI_Comm stream_comm, dup_comm;
    MPIX_Stream stream_check;

    MPIX_Stream local_streams[2] = { MPIX_STREAM_NULL, stream };
    mpi_errno = MPIX_Stream_comm_create_multiplex(MPI_COMM_WORLD, 2, local_streams, &stream_comm);
    CHECK_MPI_ERROR;

    MPI_Request req;
    mpi_errno = MPI_Comm_idup(stream_comm, &dup_comm, &req);
    CHECK_MPI_ERROR;
    mpi_errno = MPI_Wait(&req, MPI_STATUS_IGNORE);
    CHECK_MPI_ERROR;

    for (int i = 0; i < 2; i++) {
        MPIX_Comm_get_stream(dup_comm, i, &stream_check);
        if (stream_check != local_streams[i]) {
            printf("MPIX_Comm_get_stream (idx=%d) did not return expected stream\n", i);
            errs++;
        }
    }

    mpi_errno = MPI_Comm_free(&dup_comm);
    CHECK_MPI_ERROR;

    mpi_errno = MPI_Comm_free(&stream_comm);
    CHECK_MPI_ERROR;

  fn_exit:
    return errs;
}
