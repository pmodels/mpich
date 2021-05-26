/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
        MPI_Abort(MPI_COMM_WORLD, 1);

    while (1);

    MPI_Finalize();
    return 0;
}
