/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Request barrier;
    int rank, i, done;
    int count = 100;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Info info;
    MPI_Info_create(&info);

    MPIX_Barrier_init(MPI_COMM_WORLD, info, &barrier);
    for (i = 0; i < count; i++) {
        MPI_Start(&barrier);
        MPI_Wait(&barrier, MPI_STATUS_IGNORE);
    }
    MPI_Request_free(&barrier);
    MPI_Info_free(&info);

    MTest_Finalize(0);
    return 0;
}
