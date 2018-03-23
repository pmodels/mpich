/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "win_sync.h"

int main(int argc, char *argv[])
{
    int rank, nproc;
    int errors = 0, errs = 0;
    int buf = 0, my_buf;
    MPI_Win win;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_create(&buf, sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

    /* This should fail because the window is not in any access epoch */
    CHECK_ERR(MPI_Get(&my_buf, 1, MPI_INT, 0, 0, 1, MPI_INT, win));

    MPI_Win_free(&win);

    MTest_Finalize(errors);

    return MTestReturnValue(errs);
}
