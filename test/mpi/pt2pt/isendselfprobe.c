/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_isendselfprobe
int run(const char *arg);
#endif

int run(const char *arg)
{
    int rank;
    int sendMsg = 123;
    int recvMsg = 0;
    int flag = 0;
    int count;
    MPI_Status status;
    MPI_Request request;
    int errs = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        MPI_Isend(&sendMsg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
        while (!flag) {
            MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
        }
        MPI_Get_count(&status, MPI_INT, &count);
        if (count != 1) {
            errs++;
        }
        MPI_Recv(&recvMsg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        if (recvMsg != 123) {
            errs++;
        }
        MPI_Wait(&request, &status);
    }

    return errs;
}
