/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define SIZE 100000
#define ITER 1000

#define ERROR_MARGIN 0.5

static int verbose = 0;

int main(int argc, char *argv[])
{
    char *sbuf, *rbuf;
    int i, j;
    double t1, t2, t, ts;
    int rank, size;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    /* Allocate memory regions to communicate */
    sbuf = (char *) malloc(SIZE);
    rbuf = (char *) malloc(size * SIZE);

    /* Touch the buffers to make sure they are allocated */
    for (i = 0; i < SIZE; i++)
        sbuf[i] = '0';
    for (i = 0; i < SIZE * size; i++)
        rbuf[i] = '0';

    /* Time when rank 0 gathers the data */
    MPI_Barrier(MPI_COMM_WORLD);
    t1 = MPI_Wtime();
    for (i = 0; i < ITER; i++) {
        MPI_Gather(sbuf, SIZE, MPI_BYTE, rbuf, SIZE, MPI_BYTE, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    t2 = MPI_Wtime();
    t = (t2 - t1) / ITER;

    /* Time when rank 1 gathers the data */
    MPI_Barrier(MPI_COMM_WORLD);
    t1 = MPI_Wtime();
    for (j = 0; j < ITER; j++) {
        MPI_Gather(sbuf, SIZE, MPI_BYTE, rbuf, SIZE, MPI_BYTE, 1, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
    }
    t2 = MPI_Wtime();
    ts = (t2 - t1) / ITER;

    if (rank == 1)
        MPI_Send(&ts, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    if (rank == 0)
        MPI_Recv(&ts, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);

    /* Print out the results */
    if (!rank) {
        if ((ts / t) > (1 + ERROR_MARGIN)) {    /* If the difference is more than 10%, it's an error */
            printf("%.3f\t%.3f\n", 1000000.0 * ts, 1000000.0 * t);
            printf("Too much difference in performance\n");
        }
        else
            printf(" No Errors\n");
    }

    MPI_Finalize();

    return 0;
}
