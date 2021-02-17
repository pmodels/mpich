/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Basic test for collective large APIs.
  It is only intended to test the API interface rather than stress testing the internals.
 */

#define MAX_SIZE 1024
#define MAX_PROC 20

MPI_Comm comm;
int rank, nprocs;
int errs;
int sendbuf[MAX_SIZE];
int recvbuf[MAX_SIZE];

static void init_buf(void)
{
    for (int i = 0; i < MAX_SIZE; i++) {
        sendbuf[i] = i;
    }
    for (int i = 0; i < MAX_SIZE; i++) {
        recvbuf[i] = -1;
    }
}

static void test_alltoallv(void)
{
    MPI_Count counts[MAX_PROC];
    MPI_Aint displs[MAX_PROC];
    for (int i = 0; i < nprocs; i++) {
        counts[i] = 1;
    }
    for (int i = 0; i < nprocs; i++) {
        displs[i] = i * 3;
    }
    init_buf();
    MPI_Alltoallv_c(sendbuf, counts, displs, MPI_INT, recvbuf, counts, displs, MPI_INT, comm);
    /* check results */
    for (int i = 0; i < nprocs; i++) {
        if (recvbuf[displs[i]] != rank * 3) {
            errs++;
        }
    }
}

int main(int argc, char **argv)
{
    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    test_alltoallv();

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
