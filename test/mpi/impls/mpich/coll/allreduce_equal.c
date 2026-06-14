/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    MPI_Comm comm = MPI_COMM_WORLD;

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int is_equal;

    MPI_Allreduce(&rank, &is_equal, 1, MPI_INT, MPIX_EQUAL, comm);
    if (is_equal) {
        errs++;
        printf("Allreduce MPIX_EQUAL on rank resulted true!\n");
    }

    MPI_Allreduce(&size, &is_equal, 1, MPI_INT, MPIX_EQUAL, comm);
    if (!is_equal) {
        errs++;
        printf("Allreduce MPIX_EQUAL on size resulted false!\n");
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
