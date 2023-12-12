/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run pt2pt_cancelanysrc
int run(const char *arg);
#endif

int run(const char *arg)
{
    int size, rank, msg, cancelled;
    MPI_Request request;
    MPI_Status status;

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
    } else {
        MPI_Barrier(MPI_COMM_WORLD);
        msg = 42;
        MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    return 0;
}
