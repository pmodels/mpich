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


int main(int argc, char **argv)
{
    int errs = 0;
    int mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPIX_Stream stream;
    mpi_errno = MPIX_Stream_create(MPI_INFO_NULL, &stream);
    if (mpi_errno != MPI_SUCCESS) {
        MTestPrintfMsg(1, "MPIX_Stream_create returns error: %x\n", mpi_errno);
        goto fn_exit;
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);
    MPIX_Stream_free(&stream);

    /* test repeatedly allocate and deallocate a stream */
    for (int i = 0; i < 100; i++) {
        mpi_errno = MPIX_Stream_create(MPI_INFO_NULL, &stream);
        CHECK_MPI_ERROR;
        mpi_errno = MPIX_Stream_free(&stream);
        CHECK_MPI_ERROR;
    }

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
