/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This program provides a simple test of send-receive performance between
   two (or more) processes.  This sometimes called head-to-head or
   ping-ping test, as both processes send at the same time.
*/

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpitest.h"
#include "mpithreadtest.h"

/* Alignment prevents either false-sharing on the CPU or serialization in the
 * NIC's parallel TLB engine. At least it need be cacheline size. Using page
 * size for optimum results. */
#define BUFFER_ALIGNMENT 4096

#define MESSAGE_SIZE 8
#define NUM_MESSAGES 64000
#define WINDOW_SIZE 64

#define ERROR_MARGIN 0.05       /* FIXME: a better margin? */

enum RMA_OPs {
    OP_PUT,
    OP_GET,
    OP_ACC,
    OP_GACC,
    OP_INVALID
};

int rma_op = OP_INVALID;
int world_rank;
MPI_Comm *thread_wins;
double *t_elapsed;

MTEST_THREAD_RETURN_TYPE thread_fn(void *arg);

MTEST_THREAD_RETURN_TYPE thread_fn(void *arg)
{
    int error;
    int tid;
    MPI_Win my_win;
    int win_i, win_post_i, win_posts;
    void *buf, *result_buf;
    MPI_Request requests[WINDOW_SIZE];
    MPI_Status statuses[WINDOW_SIZE];
    double t_start, t_end;

    tid = (int) (long) arg;
    my_win = thread_wins[tid];

    win_posts = NUM_MESSAGES / WINDOW_SIZE;
    assert(win_posts * WINDOW_SIZE == NUM_MESSAGES);

    /* Allocate origin buffer */
    error = posix_memalign(&buf, BUFFER_ALIGNMENT, MESSAGE_SIZE * sizeof(char));
    if (error) {
        fprintf(stderr, "Thread %d: Error in allocating origin buffer\n", tid);
    }
    /* Allocate result_buf for OP_GACC */
    error = posix_memalign(&result_buf, BUFFER_ALIGNMENT, MESSAGE_SIZE * sizeof(char));
    if (error) {
        fprintf(stderr, "Thread %d: Error in allocating result buffer\n", tid);
    }

    /* Benchmark */
    t_start = MPI_Wtime();

    for (win_post_i = 0; win_post_i < win_posts; win_post_i++) {
        MPI_Win_fence(0, my_win);
        if (world_rank == 0) {
            for (win_i = 0; win_i < WINDOW_SIZE; win_i++) {
                if (rma_op == OP_PUT) {
                    MPI_Put(buf, MESSAGE_SIZE, MPI_CHAR, 1, 0, MESSAGE_SIZE, MPI_CHAR, my_win);
                } else if (rma_op == OP_GET) {
                    MPI_Get(buf, MESSAGE_SIZE, MPI_CHAR, 1, 0, MESSAGE_SIZE, MPI_CHAR, my_win);
                } else if (rma_op == OP_ACC) {
                    MPI_Accumulate(buf, MESSAGE_SIZE, MPI_CHAR, 1, 0, MESSAGE_SIZE, MPI_CHAR,
                                   MPI_REPLACE, my_win);
                } else if (rma_op == OP_GACC) {
                    MPI_Get_accumulate(buf, MESSAGE_SIZE, MPI_CHAR,
                                       result_buf, MESSAGE_SIZE, MPI_CHAR,
                                       1, 0, MESSAGE_SIZE, MPI_CHAR, MPI_NO_OP, my_win);
                }
            }
        }
        MPI_Win_fence(0, my_win);
    }

    if (world_rank == 0) {
        t_end = MPI_Wtime();
        t_elapsed[tid] = t_end - t_start;
    }

    free(buf);
    free(result_buf);
    return 0;
}


int main(int argc, char *argv[])
{
    int size;
    int provided;
    int num_threads;
    double onethread_msg_rate, multithread_msg_rate;
    int errors;

    if (argc > 2) {
        fprintf(stderr, "Can support at most only the -nthreads argument.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE required for this test.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != 2) {
        fprintf(stderr, "please run with exactly two processes.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    errors = MTest_thread_barrier_init();
    if (errors) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MTestArgList *head = MTestArgListCreate(argc, argv);
    num_threads = MTestArgListGetInt_with_default(head, "nthreads", 4);
    const char *tmp_str = MTestArgListGetString_with_default(head, "op", "put");
    if (strcmp(tmp_str, "put") == 0) {
        rma_op = OP_PUT;
    } else if (strcmp(tmp_str, "get") == 0) {
        rma_op = OP_GET;
    } else if (strcmp(tmp_str, "acc") == 0) {
        rma_op = OP_ACC;
    } else if (strcmp(tmp_str, "gacc") == 0) {
        rma_op = OP_GACC;
    } else {
        fprintf(stderr, "Invalid op - %s\n", tmp_str);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MTestArgListDestroy(head);

    thread_wins = (MPI_Win *) malloc(sizeof(MPI_Win) * num_threads);
    t_elapsed = calloc(num_threads, sizeof(double));

    /* Create a window per thread */
    int window_size = MESSAGE_SIZE;
    if (window_size % BUFFER_ALIGNMENT) {
        window_size += (BUFFER_ALIGNMENT - window_size % BUFFER_ALIGNMENT);
    }
    for (int i = 0; i < num_threads; i++) {
        void *mybase;
        MPI_Win_allocate(window_size, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &mybase, &thread_wins[i]);
    }

    /* Run test with 1 thread */
    thread_fn((void *) 0);
    onethread_msg_rate = ((double) NUM_MESSAGES / t_elapsed[0]) / 1e6;

    /* Run test with multiple threads */
    for (int i = 1; i < num_threads; i++) {
        MTest_Start_thread(thread_fn, (void *) (long) i);
    }
    thread_fn((void *) 0);

    MTest_Join_threads();

    MTest_thread_barrier_free();

    /* Calculate message rate with multiple threads */
    if (world_rank == 0) {
        MTestPrintfMsg(1, "Number of messages: %d\n", NUM_MESSAGES);
        MTestPrintfMsg(1, "Message size: %d\n", MESSAGE_SIZE);
        MTestPrintfMsg(1, "Window size: %d\n", WINDOW_SIZE);
        MTestPrintfMsg(1, "Mmsgs/s with one thread: %-10.2f\n\n", onethread_msg_rate);
        MTestPrintfMsg(1, "%-10s\t%-10s\t%-10s\n", "Thread", "Mmsgs/s", "Error");

        multithread_msg_rate = 0;
        errors = 0;
        for (int tid = 0; tid < num_threads; tid++) {
            double my_msg_rate = ((double) NUM_MESSAGES / t_elapsed[tid]) / 1e6;
            int my_error = 0;
            if ((1 - (my_msg_rate / onethread_msg_rate)) > ERROR_MARGIN) {
                /* Erroneous */
                errors++;
                my_error = 1;
                fprintf(stderr,
                        "Thread %d message rate below threshold: %.2f / %.2f = %.2f (threshold = %.2f)\n",
                        tid, my_msg_rate, onethread_msg_rate, (my_msg_rate / onethread_msg_rate),
                        ERROR_MARGIN);
            }
            MTestPrintfMsg(1, "%-10d\t%-10.2f\t%-10d\n", tid, my_msg_rate, my_error);
            multithread_msg_rate += my_msg_rate;
        }
        MTestPrintfMsg(1, "\n%-10s\t%-10s\t%-10s\t%-10s\n", "Size", "Threads", "Mmsgs/s", "Errors");
        MTestPrintfMsg(1, "%-10d\t%-10d\t%-10.2f\t%-10d\n", MESSAGE_SIZE, num_threads,
                       multithread_msg_rate, errors);
    }

    for (int i = 0; i < num_threads; i++) {
        MPI_Win_free(&thread_wins[i]);
    }
    free(thread_wins);
    free(t_elapsed);

    MTest_Finalize(errors);

    return 0;
}
