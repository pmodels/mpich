/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_isendself
int run(const char *arg);
#endif

int run(const char *arg)
{
    int a[10], b[10], i;
    MPI_Status status;
    MPI_Request request;
    int rank, count;
    int errs = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 0; i < 10; i++)
        a[i] = i + 1;

    status.MPI_ERROR = 0;
    MPI_Isend(a, 0, MPI_INT, rank, 0, MPI_COMM_WORLD, &request);
    MPI_Recv(b, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    if (status.MPI_SOURCE != rank || status.MPI_TAG != 0 || status.MPI_ERROR != 0 || count != 0) {
        errs++;
        printf("1 status = %d %d %d %d\n", status.MPI_SOURCE, status.MPI_TAG,
               status.MPI_ERROR, count);
    }
    /* printf("b[0] = %d\n", b[0]); */
    MPI_Wait(&request, &status);

    MPI_Isend(NULL, 0, MPI_INT, rank, 0, MPI_COMM_WORLD, &request);
    MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    if (status.MPI_SOURCE != rank || status.MPI_TAG != 0 || status.MPI_ERROR != 0 || count != 0) {
        errs++;
        printf("2 status = %d %d %d %d\n", status.MPI_SOURCE, status.MPI_TAG,
               status.MPI_ERROR, count);
    }
    MPI_Wait(&request, &status);

    return errs;
}
