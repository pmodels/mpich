/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 2
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

MTEST_THREAD_RETURN_TYPE test_comm_dup(void *arg)
{
    int rank;
    int i;
    MPI_Request req;
    MPI_Comm comm, self_dup;

    MPI_Comm_rank(comms[*(int *) arg], &rank);

    for (i = 0; i < NUM_ITER; i++) {

        if (*(int *) arg == rank) {
            MTestSleep(1);
        }

        MTest_thread_lock(&comm_lock);
        if (verbose)
            printf("%d: Thread %d - COMM_IDUP %d start\n", rank, *(int *) arg, i);
        MPI_Comm_idup(MPI_COMM_SELF, &self_dup, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        if (verbose)
            printf("\t%d: Thread %d - COMM_IDUP %d finish\n", rank, *(int *) arg, i);
        MTest_thread_unlock(&comm_lock);
        MPI_Comm_free(&self_dup);

        if (verbose)
            printf("%d: Thread %d - comm_idup %d start\n", rank, *(int *) arg, i);
        MPI_Comm_idup(comms[*(int *) arg], &comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        MPI_Comm_free(&comm);
        if (verbose)
            printf("\t%d: Thread %d - comm_idup %d finish\n", rank, *(int *) arg, i);
    }
    if (verbose)
        printf("%d: Thread %d - Done.\n", rank, *(int *) arg);
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

    MTest_thread_lock_create(&comm_lock);

    for (i = 0; i < NUM_THREADS; i++) {
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
