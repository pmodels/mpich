/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    int rank, size, rc, ec, errs = 0;
    int flag = 1;
    MPI_Comm dup, shrunk;

    MPI_Init(&argc, &argv);
    MPI_Comm_dup(MPI_COMM_WORLD, &dup);
    MPI_Comm_rank(dup, &rank);
    MPI_Comm_size(dup, &size);
    MPI_Comm_set_errhandler(dup, MPI_ERRORS_RETURN);

    if (size < 4) {
        fprintf(stderr, "Must run with at least 4 processes\n");
        MPI_Abort(dup, 1);
    }

    if (2 == rank)
        exit(EXIT_FAILURE);

    if (MPI_SUCCESS == (rc = MPIX_Comm_agree(dup, &flag))) {
        MPI_Error_class(rc, &ec);
        fprintf(stderr, "[%d] Expected MPIX_ERR_PROC_FAILED after agree. Received: %d\n", rank, ec);
        errs++;
        MPI_Abort(dup, 1);
    }

    if (MPI_SUCCESS != (rc = MPIX_Comm_shrink(dup, &shrunk))) {
        MPI_Error_class(rc, &ec);
        fprintf(stderr, "[%d] Expected MPI_SUCCESS after shrink. Received: %d\n", rank, ec);
        errs++;
        MPI_Abort(dup, 1);
    }

    if (MPI_SUCCESS != (rc = MPIX_Comm_agree(shrunk, &flag))) {
        MPI_Error_class(rc, &ec);
        fprintf(stderr, "[%d] Expected MPI_SUCCESS after agree. Received: %d\n", rank, ec);
        errs++;
        MPI_Abort(dup, 1);
    }

    MPI_Comm_free(&shrunk);
    MPI_Comm_free(&dup);

    if (0 == rank) {
        if (errs)
            fprintf(stdout, " Found %d errors\n", errs);
        else
            fprintf(stdout, " No errors\n");
    }

    MPI_Finalize();
}
