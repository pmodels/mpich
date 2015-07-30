/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Test threaded overlapped collective operations.
 *
 * Create one communicator for each thread, then do collective operation
 * on the communicator. For different threads on different processes, try to
 * do the collective in a overlapped order.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 2
#define BUF_SIZE 1024

#define check(X_)       \
    do {                \
        if (!(X_)) {    \
            printf("[%s:%d] -- Assertion failed: %s\n", __FILE__, __LINE__, #X_);\
            MPI_Abort(MPI_COMM_WORLD, 1); \
        }               \
    } while (0)

MPI_Comm comms[NUM_THREADS];
int rank, size;

MTEST_THREAD_RETURN_TYPE test_iallred(void *arg)
{
    MPI_Request req;
    int tid = *(int *) arg;
    int buf[BUF_SIZE];

    if (tid == rank)
        MTestSleep(1);
    MPI_Allreduce(MPI_IN_PLACE, buf, BUF_SIZE, MPI_INT, MPI_BAND, comms[tid]);

    return (MTEST_THREAD_RETURN_TYPE) 0;
}


int main(int argc, char **argv)
{
    int thread_args[NUM_THREADS];
    int i, provided;
    int errs = 0;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    check(provided == MPI_THREAD_MULTIPLE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (i = 0; i < NUM_THREADS; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        MTest_Start_thread(test_iallred, (void *) &thread_args[i]);
    }

    errs = MTest_Join_threads();

    for (i = 0; i < NUM_THREADS; i++) {
        MPI_Comm_free(&comms[i]);
    }

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
