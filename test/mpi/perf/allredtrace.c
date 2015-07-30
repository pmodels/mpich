/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by University of Illinois
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This code is intended to test the trace overhead when using an
 * MPI tracing package.  To perform the test, follow these steps:
 *
 * 1) Run with the versbose mode selected to determine the delay argument
 *    to use in subsequent tests:
 *      mpiexec -n 4096 allredtrace -v
 *    Assume that the computed delay count is 6237; that value is used in
 *    the following.
 *
 * 2) Run with an explicit delay count, without tracing enabled:
 *      mpiexec -n 4096 allredtrace -delaycount 6237
 *
 * 3) Build allredtrace with tracing enabled, then run:
 *      mpiexec -n 4096 allredtrace -delaycount 6237
 *
 * Compare the total times.  The tracing version should take slightly
 * longer but no more than, for example, 15%.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verbose = 0;
static int lCount = 0;
void Delay(int);
void SetupDelay(double);

int main(int argc, char *argv[])
{
    double usecPerCall = 100;
    double t, t1, tsum;
    int i, nLoop = 100;
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Process arguments.  We allow the delay count to be set from the
     * command line to ensure reproducibility */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-delaycount") == 0) {
            i++;
            lCount = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        }
        else {
            fprintf(stderr, "Unrecognized argument %s\n", argv[i]);
            exit(1);
        }
    }

    if (lCount == 0) {
        SetupDelay(usecPerCall);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    t = MPI_Wtime();
    for (i = 0; i < nLoop; i++) {
        MPI_Allreduce(&t1, &tsum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        Delay(lCount);
    }
    t = MPI_Wtime() - t;
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        printf("For delay count %d, time is %e\n", lCount, t);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();

    return 0;
}

void SetupDelay(double usec)
{
    double t, tick;
    double sec = 1.0e-6 * usec;
    int nLoop, i, direction;


    /* Compute the number of times to run the tests to get an accurate
     * number given the timer resolution. */
    nLoop = 1;
    tick = 100 * MPI_Wtick();
    do {
        nLoop = 2 * nLoop;
        t = MPI_Wtime();
        for (i = 0; i < nLoop; i++) {
            MPI_Wtime();
        }
        t = MPI_Wtime() - t;
    }
    while (t < tick && nLoop < 100000);

    if (verbose)
        printf("nLoop = %d\n", nLoop);

    /* Start with an estimated count */
    lCount = 128;
    direction = 0;
    while (1) {
        t = MPI_Wtime();
        for (i = 0; i < nLoop; i++) {
            Delay(lCount);
        }
        t = MPI_Wtime() - t;
        t = t / nLoop;
        if (verbose)
            printf("lCount = %d, time = %e\n", lCount, t);
        if (t > 10 * tick)
            nLoop = nLoop / 2;

        /* Compare measured delay */
        if (t > 2 * sec) {
            lCount = lCount / 2;
            if (direction == 1)
                break;
            direction = -1;
        }
        else if (t < sec / 2) {
            lCount = lCount * 2;
            if (direction == -1)
                break;
            direction = 1;
        }
        else if (t < sec) {
            /* sec/2 <= t < sec , so estimate the lCount to hit sec */
            lCount = (sec / t) * lCount;
        }
        else
            break;
    }

    if (verbose)
        printf("lCount = %d, t = %e\n", lCount, t);

    /* Should coordinate with the other processes - take the max? */
}

volatile double delayCounter = 0;
void Delay(int count)
{
    int i;

    delayCounter = 0.0;
    for (i = 0; i < count; i++) {
        delayCounter += 2.73;
    }
}
