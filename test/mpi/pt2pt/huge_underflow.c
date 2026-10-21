/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program checks if MPICH can correctly handle a huge message
 * receive when the sender underflows by sending a much smaller
 * message
 */

#include "mpitest.h"
#include <memory.h>

#ifdef MULTI_TESTS
#define run pt2pt_huge_underflow
int run(const char *arg);
#endif

#define HUGE_SIZE (10*1024*1024)

int run(const char *arg)
{
    int size, rank;
    int dest;
    char *buff;

    buff = malloc(HUGE_SIZE);
    buff[0] = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    dest = size - 1;

    /* Try testing underflow to make sure things work if we try to send 1 byte
     * when receiving a huge message */
    if (rank == 0) {
        MPI_Send(buff, 1, MPI_BYTE, dest, 0, MPI_COMM_WORLD);
    } else if (rank == dest) {
        MPI_Recv(buff, HUGE_SIZE, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    free(buff);

    MPI_Barrier(MPI_COMM_WORLD);

    return 0;
}
