/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"

int main(int argc, char *argv[])
{
    int i, j;
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Sendrecv(&i, 1, MPI_INT, 0, 100, &j, 1, MPI_INT, 0, 100, MPI_COMM_WORLD, &status);

    MPI_Finalize();
    return (0);
}
