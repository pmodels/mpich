/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <abt.h>
#include "mpi.h"

#define DEFAULT_NUM_THREADS     2
#define NUM_LOOP                10
#define BUF_SIZE                16384

#define HANDLE_ERROR(ret,msg)                           \
    if (ret != ABT_SUCCESS) {                           \
        fprintf(stderr, "ERROR[%d]: %s\n", ret, msg);   \
        exit(EXIT_FAILURE);                             \
    }

int rank;
int size;
int buf_size;
int *send_buf;
int *recv_buf;
int num_loop;
int sum;
int verbose = 0;
int profiling = 1;
double t1, t2;
#define FIELD_WIDTH 16

void thread_send_func(void *arg)
{
    ABT_thread ult_self__;
    ABT_thread_self(&ult_self__);
    size_t my_id = (size_t) arg;
    if (verbose)
        printf("[rank %d, TH%lu]: send\n", rank, my_id);

    int i, j;
    for (i = 0; i < num_loop; i++) {
        for (j = 1; j < size; j++) {
            int dest = (rank + j) % size;
            send_buf[0] = rank;
            if (verbose)
                printf("[rank %d, TH%lu]: MPI_Send to %d\n", rank, my_id, dest);
            MPI_Send(send_buf, buf_size, MPI_INT, dest, 0, MPI_COMM_WORLD);
        }
    }

}

void thread_recv_func(void *arg)
{
    ABT_thread ult_self__;
    ABT_thread_self(&ult_self__);
    size_t my_id = (size_t) arg;
    if (verbose)
        printf("[rank %d, TH%lu]: receive\n", rank, my_id);

    int i, j;
    for (i = 0; i < num_loop; i++) {
        for (j = 1; j < size; j++) {
            int source = (rank + size - j) % size;
            if (verbose)
                printf("[rank %d, TH%lu]: MPI_Recv from %d\n", rank, my_id, source);
            MPI_Recv(recv_buf, buf_size, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sum += recv_buf[0];
        }
    }
}

int main(int argc, char *argv[])
{
    int i, provided;
    int ret;
    int num_threads = DEFAULT_NUM_THREADS;
    if (argc > 1)
        num_threads = atoi(argv[1]);
    if (num_threads % 2 != 0) {
        if (rank == 0)
            printf("number of user level threads should be even\n");
        exit(0);
    }
    assert(num_threads >= 0);
    num_loop = NUM_LOOP;
    if (argc > 2)
        num_loop = atoi(argv[2]);
    assert(num_loop >= 0);
    buf_size = BUF_SIZE;
    if (argc > 3)
        buf_size = atoi(argv[3]);
    assert(buf_size >= 0);

    /* Initialize */
    ret = ABT_init(*argc, *argv);
    if (ret != ABT_SUCCESS) {
        printf("Failed to initialize Argobots\n");
        return EXIT_FAILURE;
    }

    ret = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        printf("Cannot initialize with MPI_THREAD_MULTIPLE\n");
        return EXIT_FAILURE;
    }

    send_buf = (int *) malloc(buf_size * sizeof(int));
    recv_buf = (int *) malloc(buf_size * sizeof(int));

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (profiling)
        t1 = MPI_Wtime();
    ABT_xstream xstream;
    ABT_pool pools;
    ABT_thread *threads = (ABT_thread *) malloc(num_threads * sizeof(ABT_thread));

    ABT_xstream_self(&xstream);
    ABT_xstream_get_main_pools(xstream, 1, &pools);
    for (i = 0; i < num_threads; i++) {
        size_t tid = i + 1;
        if (i % 2 == 0) {
            ret = ABT_thread_create(pools,
                                    thread_send_func, (void *) tid,
                                    ABT_THREAD_ATTR_NULL, &threads[i]);
        } else {
            ret = ABT_thread_create(pools,
                                    thread_recv_func, (void *) tid,
                                    ABT_THREAD_ATTR_NULL, &threads[i]);
        }
        HANDLE_ERROR(ret, "ABT_thread_create");
    }

    /* Join and free the ULT children */
    for (i = 0; i < num_threads; i++)
        ABT_thread_free(&threads[i]);

    if (profiling) {
        t2 = MPI_Wtime();
        if (rank == 0) {
            fprintf(stdout, "%*s%*f\n", FIELD_WIDTH, "Time", FIELD_WIDTH, t2 - t1);
        }
    }

    /* Finalize */
    free(threads);
    free(send_buf);
    free(recv_buf);
    MPI_Finalize();
    ABT_finalize();
    return EXIT_SUCCESS;
}
