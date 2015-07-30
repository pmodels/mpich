/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include "mpithreadtest.h"
#include <stdio.h>
#include <stdlib.h>

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

int size, rank;
char sbuf[MAX_MSG_SIZE], rbuf[MAX_MSG_SIZE];
static int verbose = 0;
static volatile int num_threads;
static MTEST_THREAD_LOCK_TYPE num_threads_lock;

#define ABORT_MSG(msg_) do { printf("%s", (msg_)); MPI_Abort(MPI_COMM_WORLD, 1); } while (0)

MTEST_THREAD_RETURN_TYPE run_test(void *arg);
MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    int thread_id = (int) (long) arg;
    int i, j, peer;
    MPI_Status status[WINDOW];
    MPI_Request req[WINDOW];
    double start, end;
    int err;
    int local_num_threads = -1;

    if (tp[thread_id].use_proc_null)
        peer = MPI_PROC_NULL;
    else
        peer = (rank % 2) ? rank - 1 : rank + 1;

    err = MTest_thread_lock(&num_threads_lock);
    if (err)
        ABORT_MSG("unable to acquire lock, aborting\n");
    local_num_threads = num_threads;
    err = MTest_thread_unlock(&num_threads_lock);
    if (err)
        ABORT_MSG("unable to release lock, aborting\n");

    MTest_thread_barrier(num_threads);

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

    MTest_thread_barrier(num_threads);
    return MTEST_THREAD_RETVAL_IGN;
}

void loops(void);
void loops(void)
{
    int i, nt;
    double latency, mrate, avg_latency, agg_mrate;
    int err;

    err = MTest_thread_lock_create(&num_threads_lock);
    if (err)
        ABORT_MSG("unable to create lock, aborting\n");

    for (nt = 1; nt <= MAX_THREADS; nt++) {
        err = MTest_thread_lock(&num_threads_lock);
        if (err)
            ABORT_MSG("unable to acquire lock, aborting\n");

        num_threads = 1;
        MPI_Barrier(MPI_COMM_WORLD);
        MTest_thread_barrier_init();

        for (i = 1; i < nt; i++) {
            err = MTest_Start_thread(run_test, (void *) (long) i);
            if (err) {
                /* attempt to continue with fewer threads, we may be on a
                 * thread-constrained platform like BG/P in DUAL mode */
                break;
            }
            ++num_threads;
        }

        err = MTest_thread_unlock(&num_threads_lock);
        if (err)
            ABORT_MSG("unable to release lock, aborting\n");

        if (nt > 1 && num_threads <= 1) {
            ABORT_MSG("unable to create any additional threads, aborting\n");
        }

        run_test((void *) 0);   /* we are thread 0 */
        err = MTest_Join_threads();
        if (err) {
            printf("error joining threads, err=%d", err);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MTest_thread_barrier_free();

        latency = 0;
        for (i = 0; i < num_threads; i++)
            latency += tp[i].latency;
        latency /= num_threads; /* Average latency */
        mrate = num_threads / latency;  /* Message rate */

        /* Global latency and message rate */
        MPI_Reduce(&latency, &avg_latency, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        avg_latency /= size;
        MPI_Reduce(&mrate, &agg_mrate, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if (!rank && verbose) {
            printf("Threads: %d; Latency: %.3f; Mrate: %.3f\n", num_threads, latency, mrate);
        }
    }

    err = MTest_thread_lock_free(&num_threads_lock);
    if (err)
        ABORT_MSG("unable to free lock, aborting\n");
}

int main(int argc, char **argv)
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
        printf(" No Errors\n");
    }

    MPI_Finalize();

    return 0;
}
