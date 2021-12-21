/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run coll_coll_large
int run(const char *arg);
#endif

/*
  Basic test for collective large APIs.
  It is only intended to test the API interface rather than stress testing the internals.
 */

#define MAX_SIZE 1024
#define MAX_PROC 20

static MPI_Comm comm;
static int rank, nprocs;
static int errs;
static int sendbuf[MAX_SIZE];
static int recvbuf[MAX_SIZE];

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

int run(const char *arg)
{
    comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    test_alltoallv();

    return errs;
}
