/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include "mpitest.h"

/* Basic tests for large pt2pt APIs. It does not actually send large data */

#define SIZE 1024

int rank, nproc;
int errs;
int sendbuf[SIZE];
int recvbuf[SIZE];

static void init_buf(void)
{
    if (rank == 0) {
        for (int i = 0; i < SIZE; i++) {
            sendbuf[i] = i;
        }
    } else if (rank == 1) {
        for (int i = 0; i < SIZE; i++) {
            recvbuf[i] = -1;
        }
    }
}

static void check_buf(void)
{
    if (rank == 1) {
        for (int i = 0; i < SIZE; i++) {
            if (recvbuf[i] != i) {
                errs++;
            }
        }
    }
}


int main(int argc, char *argv[])
{
    MPI_Comm comm;
    MPI_Count count = SIZE;
    int tag = 0;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);
    if (nproc != 2) {
        printf("Run test with 2 procs!\n");
        return 1;
    }

    init_buf();
    if (rank == 0) {
        MPI_Send_c(sendbuf, count, MPI_INT, 1, tag, comm);
    } else if (rank == 1) {
        MPI_Recv_c(recvbuf, count, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
    }
    check_buf();

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
