/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 1) {
        exit(EXIT_FAILURE);
    }

    if (rank == 0) {
        printf(" No Errors\n");
        fflush(stdout);
    }

    MPI_Finalize();

    return 0;
}
