/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    MPI_Comm comm = MPI_COMM_WORLD;
    int root = 0;

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    int is_equal;

    MPI_Reduce(&rank, &is_equal, 1, MPI_INT, MPIX_EQUAL, root, comm);
    if (rank == root && is_equal) {
        errs++;
        printf("Reduce MPIX_EQUAL on rank resulted true!\n");
    }

    MPI_Reduce(&size, &is_equal, 1, MPI_INT, MPIX_EQUAL, root, comm);
    if (rank == root && !is_equal) {
        errs++;
        printf("Reduce MPIX_EQUAL on size resulted false!\n");
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
