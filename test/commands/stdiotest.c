/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        fprintf(stdout, "This is stdout\n");
        fprintf(stderr, "This is stderr\n");
    }
    MPI_Finalize();

    return 0;
}
