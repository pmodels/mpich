
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 *
 *  This program checks if MPICH can correctly handle a huge message receive
 *  when the sender underflows by sending a much smaller message
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <mpi.h>

#define HUGE_SIZE (10*1024*1024)

int main(int argc, char *argv[])
{
    int size, rank;
    int dest;
    char *buff;

    MPI_Init(&argc, &argv);

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

    if (rank == 0)
        puts(" No Errors");

    MPI_Finalize();

    return 0;
}
