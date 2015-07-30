/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>
#include "mpitest.h"

#define DEFAULT_TASKS 128
#define DEFAULT_TASK_WINDOW 2
/* #define USE_THREADS */

int comm_world_rank;
int comm_world_size;

#define CHECK_SUCCESS(status) \
{ \
    if (status != MPI_SUCCESS) { \
	fprintf(stderr, "Routine on line %d returned with error status\n", __LINE__); \
	MPI_Abort(MPI_COMM_WORLD, -1); \
    } \
}

void process_spawn(MPI_Comm * comm, int thread_id);
void process_spawn(MPI_Comm * comm, int thread_id)
{
    CHECK_SUCCESS(MPI_Comm_spawn((char *) "./taskmaster", (char **) NULL, 1, MPI_INFO_NULL, 0,
                                 MPI_COMM_WORLD, comm, NULL));
}

void process_disconnect(MPI_Comm * comm, int thread_id);
void process_disconnect(MPI_Comm * comm, int thread_id)
{
    if (comm_world_rank == 0) {
        CHECK_SUCCESS(MPI_Recv(NULL, 0, MPI_CHAR, 0, 1, *comm, MPI_STATUS_IGNORE));
        CHECK_SUCCESS(MPI_Send(NULL, 0, MPI_CHAR, 0, 1, *comm));
    }

    CHECK_SUCCESS(MPI_Comm_disconnect(comm));
}

#ifdef USE_THREADS
static void *main_thread(void *arg)
{
    MPI_Comm child_comm;
    int thread_id = *((int *) arg);

    process_spawn(&child_comm, thread_id);
    CHECK_SUCCESS(MPI_Comm_set_errhandler(child_comm, MPI_ERRORS_RETURN));
    process_disconnect(&child_comm, thread_id);

    return NULL;
}
#endif /* USE_THREADS */

int main(int argc, char *argv[])
{
    int tasks = 0, i, j;
    MPI_Comm parent;
#ifdef USE_THREADS
    int provided;
    pthread_t *threads = NULL;
#else
    MPI_Comm *child;
#endif /* USE_THREADS */
    int can_spawn, errs = 0;

#ifdef USE_THREADS
    CHECK_SUCCESS(MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided));
    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI does not provide THREAD_MULTIPLE support\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
#else
    CHECK_SUCCESS(MPI_Init(&argc, &argv));
#endif

    errs += MTestSpawnPossible(&can_spawn);

    if (!can_spawn) {
        if (errs)
            printf(" Found %d errors\n", errs);
        else
            printf(" No Errors\n");
        fflush(stdout);
        goto fn_exit;
    }

    CHECK_SUCCESS(MPI_Comm_get_parent(&parent));

    if (parent == MPI_COMM_NULL) {      /* Parent communicator */
        if (argc == 2) {
            tasks = atoi(argv[1]);
        }
        else if (argc == 1) {
            tasks = DEFAULT_TASKS;
        }
        else {
            fprintf(stderr, "Usage: %s {number_of_tasks}\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &comm_world_rank));
        CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &comm_world_size));

#ifdef USE_THREADS
        threads = (pthread_t *) malloc(tasks * sizeof(pthread_t));
        if (!threads) {
            fprintf(stderr, "Unable to allocate memory for threads\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
#else
        child = (MPI_Comm *) malloc(tasks * sizeof(MPI_Comm));
        if (!child) {
            fprintf(stderr, "Unable to allocate memory for child communicators\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
#endif /* USE_THREADS */

#ifdef USE_THREADS
        /* Create a thread for each task. Each thread will spawn a
         * child process to perform its task. */
        for (i = 0; i < tasks;) {
            for (j = 0; j < DEFAULT_TASK_WINDOW; j++)
                pthread_create(&threads[j], NULL, main_thread, &j);
            for (j = 0; j < DEFAULT_TASK_WINDOW; j++)
                pthread_join(threads[j], NULL);
            i += DEFAULT_TASK_WINDOW;
        }
#else
        /* Directly spawn a child process to perform each task */
        for (i = 0; i < tasks;) {
            for (j = 0; j < DEFAULT_TASK_WINDOW; j++)
                process_spawn(&child[j], -1);
            for (j = 0; j < DEFAULT_TASK_WINDOW; j++)
                process_disconnect(&child[j], -1);
            i += DEFAULT_TASK_WINDOW;
        }
#endif /* USE_THREADS */

        CHECK_SUCCESS(MPI_Barrier(MPI_COMM_WORLD));

        if (comm_world_rank == 0)
            printf(" No Errors\n");
    }
    else {      /* Child communicator */
        /* Do some work here and send a message to the root process in
         * the parent communicator. */
        CHECK_SUCCESS(MPI_Send(NULL, 0, MPI_CHAR, 0, 1, parent));
        CHECK_SUCCESS(MPI_Recv(NULL, 0, MPI_CHAR, 0, 1, parent, MPI_STATUS_IGNORE));
        CHECK_SUCCESS(MPI_Comm_disconnect(&parent));
    }

  fn_exit:
#ifdef USE_THREADS
    if (threads)
        free(threads);
#endif
    MPI_Finalize();

    return 0;
}
