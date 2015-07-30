/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "mpi.h"

MPI_Comm comm_all;

void error_handler(MPI_Comm * communicator, int *error_code, ...)
{
    MPI_Comm *new_comm = malloc(sizeof(MPI_Comm));

    MPIX_Comm_revoke(comm_all);
    MPIX_Comm_shrink(comm_all, new_comm);

    MPI_Comm_free(&comm_all);

    comm_all = *new_comm;
}

int main(int argc, char *argv[])
{
    int rank, size, i;
    int sum = 0, val = 1;
    int errs = 0;
    MPI_Errhandler errhandler;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 4) {
        fprintf(stderr, "Must run with at least 4 processes.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_dup(MPI_COMM_WORLD, &comm_all);

    MPI_Comm_create_errhandler(&error_handler, &errhandler);
    MPI_Comm_set_errhandler(comm_all, errhandler);

    for (i = 0; i < 10; ++i) {
        MPI_Comm_size(comm_all, &size);
        sum = 0;
        if (i == 5 && rank == 1) {
            exit(1);
        }
        else if (i != 5) {
            MPI_Allreduce(&val, &sum, 1, MPI_INT, MPI_SUM, comm_all);
            if (sum != size && rank == 0) {
                errs++;
                fprintf(stderr, "Incorrect answer: %d != %d\n", sum, size);
            }
        }
    }

    if (0 == rank && errs) {
        fprintf(stdout, " Found %d errors\n", errs);
    }
    else if (0 == rank) {
        fprintf(stdout, " No errors\n");
    }

    MPI_Comm_free(&comm_all);
    MPI_Errhandler_free(&errhandler);

    MPI_Finalize();

    return 0;
}
