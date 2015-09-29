/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 4
#define NUM_ITER    1

#define check(X_)       \
    do {                \
        if (!(X_)) {    \
            printf("[%s:%d] -- Assertion failed: %s\n", __FILE__, __LINE__, #X_);\
            MPI_Abort(MPI_COMM_WORLD, 1); \
        }               \
    } while (0)

MPI_Comm comms[NUM_THREADS];
MTEST_THREAD_LOCK_TYPE comm_lock;
int rank, size;
int verbose = 0;
volatile int start_idup[NUM_THREADS];

MTEST_THREAD_RETURN_TYPE test_comm_dup(void *arg)
{
    int rank;
    int i, j;
    int wait;
    int tid = *(int *) arg;
    MPI_Comm_rank(comms[*(int *) arg], &rank);
    MPI_Comm comm;
    MPI_Request req;

    if (tid % 2 == 0 && rank % 2 == 0) {
        do {
            wait = 0;
            for (i = 0; i < NUM_THREADS; i++)
                wait += start_idup[i];
        } while (wait > NUM_THREADS / 2);
    }

    MPI_Comm_idup(comms[*(int *) arg], &comm, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    start_idup[tid] = 0;
    MPI_Comm_free(&comm);
    return (MTEST_THREAD_RETURN_TYPE) 0;
}


int main(int argc, char **argv)
{
    int thread_args[NUM_THREADS];
    int i, provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    check(provided == MPI_THREAD_MULTIPLE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (i = 0; i < NUM_THREADS; i++) {
        start_idup[i] = 1;
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        MTest_Start_thread(test_comm_dup, (void *) &thread_args[i]);
    }

    MTest_Join_threads();

    for (i = 0; i < NUM_THREADS; i++) {
        MPI_Comm_free(&comms[i]);
    }

    if (rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
