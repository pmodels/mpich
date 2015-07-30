
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
 * This test ensures that shrink works correctly
 */
int main(int argc, char **argv)
{
    int rank, size, newsize, rc, errclass, errs = 0;
    MPI_Comm newcomm;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (size < 4) {
        fprintf(stderr, "Must run with at least 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (2 == rank)
        exit(EXIT_FAILURE);

    rc = MPIX_Comm_shrink(MPI_COMM_WORLD, &newcomm);
    if (rc) {
        MPI_Error_class(rc, &errclass);
        fprintf(stderr, "Expected MPI_SUCCESS from MPIX_Comm_shrink. Received: %d\n", errclass);
        errs++;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_size(newcomm, &newsize);
    if (newsize != size - 1)
        errs++;

    rc = MPI_Barrier(newcomm);
    if (rc) {
        MPI_Error_class(rc, &errclass);
        fprintf(stderr, "Expected MPI_SUCCESS from MPI_BARRIER. Received: %d\n", errclass);
        errs++;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_free(&newcomm);

    if (0 == rank)
        fprintf(stdout, " No Errors\n");

    MPI_Finalize();

    return 0;
}
