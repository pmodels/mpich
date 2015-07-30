/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <assert.h>
#include "mpi.h"
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Request request;
    int size, rank;
    int one = 1, two = 2, isum, sum;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    assert(size == 2);
    MPI_Iallreduce(&one, &isum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &request);
    MPI_Allreduce(&two, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Wait(&request, MPI_STATUS_IGNORE);

    assert(isum == 2);
    assert(sum == 4);
    if (rank == 0)
        printf(" No errors\n");

    MPI_Finalize();
    return 0;
}
