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
    int rank, size, rc, errclass, errs = 0;
    int flag = 1;

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

    if (0 == rank)
        flag = 0;

    rc = MPIX_Comm_agree(MPI_COMM_WORLD, &flag);
    MPI_Error_class(rc, &errclass);
    if (errclass != MPIX_ERR_PROC_FAILED) {
        fprintf(stderr, "[%d] Expected MPIX_ERR_PROC_FAILED after agree. Received: %d\n", rank,
                errclass);
        MPI_Abort(MPI_COMM_WORLD, 1);
        errs++;
    }
    else if (0 != flag) {
        fprintf(stderr, "[%d] Expected flag to be 0. Received: %d\n", rank, flag);
        errs++;
    }

    MPIX_Comm_failure_ack(MPI_COMM_WORLD);

    if (0 == rank)
        flag = 0;
    else
        flag = 1;
    rc = MPIX_Comm_agree(MPI_COMM_WORLD, &flag);
    MPI_Error_class(rc, &errclass);
    if (MPI_SUCCESS != rc) {
        fprintf(stderr, "[%d] Expected MPI_SUCCESS after agree. Received: %d\n", rank, errclass);
        MPI_Abort(MPI_COMM_WORLD, 1);
        errs++;
    }
    else if (0 != flag) {
        fprintf(stderr, "[%d] Expected flag to be 0. Received: %d\n", rank, flag);
        MPI_Abort(MPI_COMM_WORLD, 1);
        errs++;
    }

    MPI_Finalize();

    if (0 == rank) {
        if (errs == 0)
            fprintf(stdout, " No Errors\n");
        else
            fprintf(stdout, " Found %d errors\n", errs);
    }

    return errs;
}
