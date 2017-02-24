/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 *
 *  This program checks if MPICH can correctly handle many outstanding large
 *  message transfers which use wildcard receives.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <mpi.h>

#define N_TRY 32
#define BLKSIZE (10*1024*1024)

int main(int argc, char *argv[])
{
    int size, rank;
    int dest;
    int i;
    char *buff;
    MPI_Request reqs[N_TRY];

    MPI_Init(&argc, &argv);

    buff = malloc(N_TRY * BLKSIZE);
    memset(buff, -1, N_TRY * BLKSIZE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    dest = size - 1;

    if (rank == 0) {
        for (i = 0; i < N_TRY; i++)
            MPI_Isend(buff + BLKSIZE*i, BLKSIZE, MPI_BYTE, dest, 0, MPI_COMM_WORLD, &reqs[i]);
        MPI_Waitall(N_TRY, reqs, MPI_STATUSES_IGNORE);
    }
    else if (rank == dest) {
        for (i = 0; i < N_TRY; i++)
            MPI_Irecv(buff + BLKSIZE*i, BLKSIZE, MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &reqs[i]);
        MPI_Waitall(N_TRY, reqs, MPI_STATUSES_IGNORE);
    }

    free(buff);

    if (rank == 0)
        puts(" No Errors");

    MPI_Finalize();

    return 0;
}
