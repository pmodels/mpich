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
#define NUM_ITER    100

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

MTEST_THREAD_RETURN_TYPE test_comm_create_group(void *arg)
{
    int i;

    for (i = 0; i < NUM_ITER; i++) {
        MPI_Group world_group;
        MPI_Comm comm, self_dup;

        MPI_Comm_group(comms[*(int *) arg], &world_group);

        /* Every thread paticipates in a distinct MPI_Comm_create group,
         * distinguished by its thread-id (used as the tag).  Threads on even
         * ranks join an even comm and threads on odd ranks join the odd comm.
         */

        if (verbose)
            printf("%d: Thread %d - Comm_create_group %d start\n", rank, *(int *) arg, i);
        MPI_Comm_create_group(comms[*(int *) arg], world_group, *(int *) arg /* tag */ , &comm);
        MPI_Barrier(comm);
        MPI_Comm_free(&comm);
        if (verbose)
            printf("%d: Thread %d - Comm_create_group %d finish\n", rank, *(int *) arg, i);

        MPI_Group_free(&world_group);

        /* Threads now call a collective MPI Comm routine.  This is done on a
         * separate communicator so it can be concurrent with calls to
         * Comm_create_group.  A lock is needed to preserve MPI's threads +
         * collective semantics.
         */
        MTest_thread_lock(&comm_lock);
        if (verbose)
            printf("%d: Thread %d - Comm_dup %d start\n", rank, *(int *) arg, i);
        MPI_Comm_dup(MPI_COMM_SELF, &self_dup);
        if (verbose)
            printf("%d: Thread %d - Comm_dup %d finish\n", rank, *(int *) arg, i);
        MTest_thread_unlock(&comm_lock);

        MPI_Barrier(self_dup);
        MPI_Comm_free(&self_dup);
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

    MTest_thread_lock_create(&comm_lock);

    for (i = 0; i < NUM_THREADS; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        MTest_Start_thread(test_comm_create_group, (void *) &thread_args[i]);
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
