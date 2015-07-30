/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpi.h"

int main(int argc, char **argv)
{
    int size, rank, msg, cancelled;
    MPI_Request request;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size != 2) {
        fprintf(stderr, "ERROR: must be run with 2 processes");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        msg = -1;
        /* Post, then cancel MPI_ANY_SOURCE recv */
        MPI_Irecv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        MPI_Cancel(&request);
        MPI_Wait(&request, &status);
        MPI_Test_cancelled(&status, &cancelled);
        assert(cancelled);

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Irecv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
        assert(msg == 42);
    }
    else {
        MPI_Barrier(MPI_COMM_WORLD);
        msg = 42;
        MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();
}
