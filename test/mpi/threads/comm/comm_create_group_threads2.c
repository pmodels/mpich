/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test test MPI_Comm_create_group tests calling by multiple threads
   in parallel, using the same communicator but different tags */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 4
#define NUM_ITER    100

#define check(X_)       \
    do {                \
        if (!(X_)) {    \
            printf("[%s:%d] -- Assertion failed: %s\n", __FILE__, __LINE__, #X_);\
            MPI_Abort(MPI_COMM_WORLD, 1); \
        }               \
    } while (0)

MPI_Group global_group;

int rank, size;
int verbose = 0;

MTEST_THREAD_RETURN_TYPE test_comm_create_group(void *arg)
{
    int i;
    MPI_Group half_group, full_group;
    int range[1][3];
    MPI_Comm comm;

    range[0][0] = 0;
    range[0][1] = size / 2;
    range[0][2] = 1;

    for (i = 0; i < NUM_ITER; i++) {

        MPI_Comm_group(MPI_COMM_WORLD, &full_group);
        MPI_Group_range_incl(full_group, 1, range, &half_group);
        /* Every thread paticipates in a distinct MPI_Comm_create_group,
         * distinguished by its thread-id (used as the tag).
         */
        if (verbose)
            printf("%d: Thread %d - Comm_create_group %d start\n", rank, *(int *) arg, i);
        if(rank <= size / 2){
            MPI_Comm_create_group(MPI_COMM_WORLD, half_group, *(int *) arg /* tag */ , &comm);
            MPI_Barrier(comm);
            MPI_Comm_free(&comm);
        }
        if (verbose)
            printf("%d: Thread %d - Comm_create_group %d finish\n", rank, *(int *) arg, i);
        MPI_Group_free(&half_group);
        MPI_Group_free(&full_group);

        /* Repeat the test, using the same group */
        if (verbose)
            printf("%d: Thread %d - Comm_create_group %d start\n", rank, *(int *) arg, i);
        MPI_Comm_create_group(MPI_COMM_WORLD, global_group, *(int *) arg /* tag */ , &comm);
        MPI_Barrier(comm);
        MPI_Comm_free(&comm);
        if (verbose)
            printf("%d: Thread %d - Comm_create_group %d finish\n", rank, *(int *) arg, i);

    }

    if (verbose)
        printf("%d: Thread %d - Done.\n", rank, *(int *) arg);
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

    MPI_Comm_group(MPI_COMM_WORLD, &global_group);

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        MTest_Start_thread(test_comm_create_group, (void *) &thread_args[i]);
    }

    err = MTest_Join_threads();

    MPI_Group_free(&global_group);

    MTest_Finalize(err);

    MPI_Finalize();

    return 0;
}
