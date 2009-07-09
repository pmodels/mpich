/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <pthread.h>

#define MAX_THREADS 4
/* #define LOOPS 10000 */
#define LOOPS 1000
#define WINDOW 16
/* #define MAX_MSG_SIZE 16384 */
#define MAX_MSG_SIZE 4096
#define HOP 4

/* Emulated thread private storage */
struct tp {
    int thread_id;
    int use_proc_null;
    int use_blocking_comm;
    int msg_size;
    double latency;
} tp[MAX_THREADS];

pthread_barrier_t pbarrier;
int size, rank;
char sbuf[MAX_MSG_SIZE], rbuf[MAX_MSG_SIZE];
static int verbose = 0;

void *run_test(void *arg)
{
    int thread_id = (int) arg;
    int i, j, peer;
    MPI_Status status[WINDOW];
    MPI_Request req[WINDOW];
    double start, end;

    if (tp[thread_id].use_proc_null)
        peer = MPI_PROC_NULL;
    else
        peer = (rank % 2) ? rank - 1 : rank + 1;
    pthread_barrier_wait(&pbarrier);

    start = MPI_Wtime();

    if (tp[thread_id].use_blocking_comm) {
        if ((rank % 2) == 0) {
            for (i = 0; i < LOOPS; i++)
                for (j = 0; j < WINDOW; j++)
                    MPI_Send(sbuf, tp[thread_id].msg_size, MPI_CHAR, peer, 0, MPI_COMM_WORLD);
        }
        else {
            for (i = 0; i < LOOPS; i++)
                for (j = 0; j < WINDOW; j++)
                    MPI_Recv(rbuf, tp[thread_id].msg_size, MPI_CHAR, peer, 0, MPI_COMM_WORLD,
                             &status[0]);
        }
    }
    else {
        for (i = 0; i < LOOPS; i++) {
            if ((rank % 2) == 0) {
                for (j = 0; j < WINDOW; j++)
                    MPI_Isend(sbuf, tp[thread_id].msg_size, MPI_CHAR, peer, 0, MPI_COMM_WORLD,
                              &req[j]);
            }
            else {
                for (j = 0; j < WINDOW; j++)
                    MPI_Irecv(rbuf, tp[thread_id].msg_size, MPI_CHAR, peer, 0, MPI_COMM_WORLD,
                              &req[j]);
            }
            MPI_Waitall(WINDOW, req, status);
        }
    }

    end = MPI_Wtime();
    tp[thread_id].latency = 1000000.0 * (end - start) / (LOOPS * WINDOW);

    pthread_barrier_wait(&pbarrier);
}

void loops(void)
{
    int i, nt;
    pthread_t thread[MAX_THREADS];
    void *retval[MAX_THREADS];
    double latency, mrate, avg_latency, agg_mrate;

    for (nt = 1; nt <= MAX_THREADS; nt++) {
        MPI_Barrier(MPI_COMM_WORLD);
        pthread_barrier_init(&pbarrier, NULL, nt);

        for (i = 1; i < nt; i++)
            pthread_create(&thread[i], NULL, (void*) run_test, (void *) i);
        run_test((void *) 0);
        for (i = 1; i < nt; i++)
            pthread_join(thread[i], &retval[i]);
            
        pthread_barrier_destroy(&pbarrier);

        latency = 0;
        for (i = 0; i < nt; i++)
            latency += tp[i].latency;
        latency /= nt; /* Average latency */
        mrate = nt / latency; /* Message rate */

        /* Global latency and message rate */
        MPI_Reduce(&latency, &avg_latency, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        avg_latency /= size;
        MPI_Reduce(&mrate, &agg_mrate, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if (!rank && verbose) {
            printf("Threads: %d; Latency: %.3f; Mrate: %.3f\n",
                   nt, latency, mrate);
        }
    }
}

int main(int argc, char ** argv)
{
    int pmode, i, j;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "Thread Multiple not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    /* For communication, we need an even number of processes */
    if (size % 2) {
        fprintf(stderr, "This test needs an even number of processes\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    /* PROC_NULL */
    for (i = 0; i < MAX_THREADS; i++) {
        tp[i].thread_id = i;
        tp[i].use_proc_null = 1;
        tp[i].use_blocking_comm = 1;
        tp[i].msg_size = 0;
    }
    if (!rank && verbose) {
        printf("\nUsing MPI_PROC_NULL\n");
        printf("-------------------\n");
    }
    loops();

    /* Blocking communication */
    for (j = 0; j < MAX_MSG_SIZE; j = (!j ? 1 : j * HOP)) {
        for (i = 0; i < MAX_THREADS; i++) {
            tp[i].thread_id = i;
            tp[i].use_proc_null = 0;
            tp[i].use_blocking_comm = 1;
            tp[i].msg_size = j;
        }
        if (!rank && verbose) {
            printf("\nBlocking communication with message size %6d bytes\n", j);
            printf("------------------------------------------------------\n");
        }
        loops();
    }

    /* Non-blocking communication */
    for (j = 0; j < MAX_MSG_SIZE; j = (!j ? 1 : j * HOP)) {
        for (i = 0; i < MAX_THREADS; i++) {
            tp[i].thread_id = i;
            tp[i].use_proc_null = 0;
            tp[i].use_blocking_comm = 0;
            tp[i].msg_size = j;
        }
        if (!rank && verbose) {
            printf("\nNon-blocking communication with message size %6d bytes\n", j);
            printf("----------------------------------------------------------\n");
        }
        loops();
    }

    if (rank == 0) {
	printf( " No Errors\n" );
    }

    MPI_Finalize();

    return 0;
}
