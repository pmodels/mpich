/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program checks if MPICH can correctly handle huge synchronous
 * sends
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_huge_ssend
int run(const char *arg);
#endif

#define COUNT (4*1024*1024)

int run(const char *arg)
{
    int *buff;
    int size, rank;

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

    return 0;
}
