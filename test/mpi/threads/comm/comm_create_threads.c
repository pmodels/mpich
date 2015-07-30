/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 8
#define NUM_ITER    100

#define check(X_)       \
    do {                \
        if (!(X_)) {    \
            printf("[%s:%d] -- Assertion failed: %s\n", __FILE__, __LINE__, #X_);\
            MPI_Abort(MPI_COMM_WORLD, 1); \
        }               \
    } while (0)

MPI_Comm comms[NUM_THREADS];
int rank, size;

MTEST_THREAD_RETURN_TYPE test_comm_create(void *arg)
{
    int i;

    for (i = 0; i < NUM_ITER; i++) {
        MPI_Group world_group;
        MPI_Comm comm;

        MPI_Comm_group(comms[*(int *) arg], &world_group);

        /* Every thread paticipates in a distinct MPI_Comm_create on distinct
         * communicators.  Thus, there is no violation of MPI threads +
         * communicators semantics.
         */

        MPI_Comm_create(comms[*(int *) arg], world_group, &comm);
        MPI_Barrier(comm);
        MPI_Comm_free(&comm);

        MPI_Group_free(&world_group);

    }

    return NULL;
}


int main(int argc, char **argv)
{
    int thread_args[NUM_THREADS];
    int i, err, provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    check(provided == MPI_THREAD_MULTIPLE);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (i = 0; i < NUM_THREADS; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        MTest_Start_thread(test_comm_create, (void *) &thread_args[i]);
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
