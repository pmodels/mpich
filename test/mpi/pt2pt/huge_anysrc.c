/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program checks if MPICH can correctly handle many outstanding
 * large message transfers which use wildcard receives.
 */

#include "mpitest.h"
#include <memory.h>

#ifdef MULTI_TESTS
#define run pt2pt_huge_anysrc
int run(const char *arg);
#endif

#define N_TRY 32
#define BLKSIZE (10*1024*1024)

int run(const char *arg)
{
    int size, rank;
    int dest;
    int i;
    char *buff;
    MPI_Request reqs[N_TRY];

    buff = malloc(N_TRY * BLKSIZE);
    memset(buff, -1, N_TRY * BLKSIZE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    dest = size - 1;

    if (rank == 0) {
        for (i = 0; i < N_TRY; i++)
            MPI_Isend(buff + BLKSIZE * i, BLKSIZE, MPI_BYTE, dest, 0, MPI_COMM_WORLD, &reqs[i]);
        MPI_Waitall(N_TRY, reqs, MPI_STATUSES_IGNORE);
    } else if (rank == dest) {
        for (i = 0; i < N_TRY; i++)
            MPI_Irecv(buff + BLKSIZE * i, BLKSIZE, MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
                      &reqs[i]);
        MPI_Waitall(N_TRY, reqs, MPI_STATUSES_IGNORE);
    }

    free(buff);

    return 0;
}
