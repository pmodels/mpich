/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program checks if MPICH can correctly handle huge synchronous
 * sends
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpitest.h"

#define COUNT (4*1024*1024)

int main(int argc, char *argv[])
{
    int *buff;
    int size, rank;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        fprintf(stderr, "Launch with two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    buff = malloc(COUNT * sizeof(int));

    if (rank == 0)
        MPI_Ssend(buff, COUNT, MPI_INT, 1, 0, MPI_COMM_WORLD);
    else
        MPI_Recv(buff, COUNT, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    free(buff);

    MTest_Finalize(0);

    return 0;
}
