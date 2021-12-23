/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_pt2pt_large
int run(const char *arg);
#endif

/* Basic tests for large pt2pt APIs. It does not actually send large data */

#define SIZE 1024

static int rank, nproc;
static int errs;
static int sendbuf[SIZE];
static int recvbuf[SIZE];

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


int run(const char *arg)
{
    MPI_Comm comm;
    MPI_Count count = SIZE;
    int tag = 0;

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);
    if (nproc != 2) {
        printf("Run test with 2 procs!\n");
        return 1;
    }
#if MTEST_HAVE_MIN_MPI_VERSION(4,0)
    init_buf();
    if (rank == 0) {
        MPI_Send_c(sendbuf, count, MPI_INT, 1, tag, comm);
    } else if (rank == 1) {
        MPI_Recv_c(recvbuf, count, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
    }
    check_buf();
#endif

    return errs;
}
