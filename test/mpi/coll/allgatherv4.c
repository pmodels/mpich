/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include "mpi.h"
#include "mpitest.h"

#define START_BUF (1)
#define LARGE_BUF (8 * 1024 * 1024)
#define MAX_BUF   (128 * 1024 * 1024)
#define LOOPS 10

char * sbuf, * rbuf;
int * recvcounts, * displs;
long long tmp;
int errs = 0;

/* #define dprintf printf */
#define dprintf(...)

typedef enum {
    REGULAR,
    BCAST,
    SPIKE,
    HALF_FULL,
    LINEAR_DECREASE,
    BELL_CURVE
} test_t;

void comm_tests(MPI_Comm comm);
double run_test(long long msg_size, MPI_Comm comm, test_t test_type, double * max_time);

int main(int argc, char ** argv)
{
    int comm_size, comm_rank;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

    sbuf = (void *) malloc(MAX_BUF);
    rbuf = (void *) malloc(MAX_BUF);

    srand(time(NULL));

    recvcounts = (void *) malloc(comm_size * sizeof(int));
    displs = (void *) malloc(comm_size * sizeof(int));
    if (!recvcounts || !displs || !sbuf || !rbuf) {
        fprintf(stderr, "Unable to allocate memory\n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if (!comm_rank) {
        dprintf("Message Range: (%d, %d); System size: %d\n", START_BUF, LARGE_BUF, comm_size);
        fflush(stdout);
    }


    /* COMM_WORLD tests */
    if (!comm_rank) {
        dprintf("\n\n==========================================================\n");
        dprintf("                         MPI_COMM_WORLD\n");
        dprintf("==========================================================\n");
    }
    comm_tests(MPI_COMM_WORLD);

    /* non-COMM_WORLD tests */
    if (!comm_rank) {
        dprintf("\n\n==========================================================\n");
        dprintf("                         non-COMM_WORLD\n");
        dprintf("==========================================================\n");
    }
    MPI_Comm_split(MPI_COMM_WORLD, (comm_rank == comm_size - 1) ? 0 : 1, 0, &comm);
    if (comm_rank < comm_size - 1)
        comm_tests(comm);
    MPI_Comm_free(&comm);

    /* Randomized communicator tests */
    if (!comm_rank) {
        dprintf("\n\n==========================================================\n");
        dprintf("                         Randomized Communicator\n");
        dprintf("==========================================================\n");
    }
    MPI_Comm_split(MPI_COMM_WORLD, 0, rand(), &comm);
    comm_tests(comm);
    MPI_Comm_free(&comm);

    free(sbuf);
    free(rbuf);
    free(recvcounts);
    free(displs);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}

void comm_tests(MPI_Comm comm)
{
    int comm_size, comm_rank;
    double time, max_time;
    long long msg_size;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);

    for (msg_size = START_BUF; msg_size <= LARGE_BUF; msg_size *= 2) {
        if (!comm_rank) {
            dprintf("\n====> MSG_SIZE: %d\n", (int) msg_size);
            fflush(stdout);
        }

        time = run_test(msg_size, comm, REGULAR, &max_time);
        if (!comm_rank) {
            dprintf("REGULAR:\tAVG: %.3f\tMAX: %.3f\n", time, max_time);
            fflush(stdout);
        }

        time = run_test(msg_size, comm, BCAST, &max_time);
        if (!comm_rank) {
            dprintf("BCAST:\tAVG: %.3f\tMAX: %.3f\n", time, max_time);
            fflush(stdout);
        }

        time = run_test(msg_size, comm, SPIKE, &max_time);
        if (!comm_rank) {
            dprintf("SPIKE:\tAVG: %.3f\tMAX: %.3f\n", time, max_time);
            fflush(stdout);
        }

        time = run_test(msg_size, comm, HALF_FULL, &max_time);
        if (!comm_rank) {
            dprintf("HALF_FULL:\tAVG: %.3f\tMAX: %.3f\n", time, max_time);
            fflush(stdout);
        }

        time = run_test(msg_size, comm, LINEAR_DECREASE, &max_time);
        if (!comm_rank) {
            dprintf("LINEAR_DECREASE:\tAVG: %.3f\tMAX: %.3f\n", time, max_time);
            fflush(stdout);
        }

        time = run_test(msg_size, comm, BELL_CURVE, &max_time);
        if (!comm_rank) {
            dprintf("BELL_CURVE:\tAVG: %.3f\tMAX: %.3f\n", time, max_time);
            fflush(stdout);
        }
    }
}

double run_test(long long msg_size, MPI_Comm comm, test_t test_type, double * max_time)
{
    int i, j;
    int comm_size, comm_rank;
    double start, end;
    double total_time, avg_time;

    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);

    if (test_type == REGULAR) {
        for (i = 0; i < comm_size; i++) {
            recvcounts[i] = msg_size;
            displs[i] = 0;
        }
    }
    else if (test_type == BCAST) {
        for (i = 0; i < comm_size; i++) {
            recvcounts[i] = (!i) ? msg_size : 0;
            displs[i] = 0;
        }
    }
    else if (test_type == SPIKE) {
        for (i = 0; i < comm_size; i++) {
            recvcounts[i] = (!i) ? (msg_size / 2) : (msg_size / (2 * (comm_size - 1)));
            displs[i] = 0;
        }
    }
    else if (test_type == HALF_FULL) {
        for (i = 0; i < comm_size; i++) {
            recvcounts[i] = (i < (comm_size / 2)) ? (2 * msg_size) : 0;
            displs[i] = 0;
        }
    }
    else if (test_type == LINEAR_DECREASE) {
        for (i = 0; i < comm_size; i++) {
            tmp = 2 * msg_size * (comm_size - 1 - i) / (comm_size - 1);
            recvcounts[i] = (int) tmp;
            displs[i] = 0;

            /* If the maximum message size is too large, don't run */
            if (tmp > MAX_BUF) return 0;
        }
    }
    else if (test_type == BELL_CURVE) {
        for (i = 1; i <= comm_size; i *= 2) {
            for (j = 0; j < i; j++) {
                if (i - 1 + j >= comm_size) continue;
                tmp = msg_size * comm_size / (log(comm_size) * i);
                recvcounts[i - 1 + j] = (int) tmp;
                displs[i - 1 + j] = 0;

                /* If the maximum message size is too large, don't run */
                if (tmp > MAX_BUF) return 0;
            }
        }
    }

    MPI_Barrier(comm);
    start = 1000000.0 * MPI_Wtime();
    for (i = 0; i < LOOPS; i++) {
        MPI_Allgatherv(sbuf, recvcounts[comm_rank], MPI_CHAR,
                       rbuf, recvcounts, displs, MPI_CHAR, comm);
    }
    end = 1000000.0 * MPI_Wtime();
    MPI_Barrier(comm);

    total_time = end - start;
    MPI_Reduce(&total_time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    MPI_Reduce(&total_time, max_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

    return (avg_time / (LOOPS * comm_size));
}
