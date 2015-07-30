/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This program provides a simple test of send-receive performance between
   two (or more) processes.  This sometimes called head-to-head or
   ping-ping test, as both processes send at the same time.
*/

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define MAXTESTS 32
#define ERROR_MARGIN 1.0        /* FIXME: This number is pretty much randomly chosen */

static int verbose = 0;

int main(int argc, char *argv[])
{
    int wsize, wrank, partner, len, maxlen, k, reps, repsleft;
    double t1;
    MPI_Request rreq;
    char *rbuf, *sbuf;
    double times[3][MAXTESTS];

    MPI_Init(&argc, &argv);
    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    if (wsize < 2) {
        fprintf(stderr, "This program requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* Set partner based on whether rank is odd or even */
    if (wrank & 0x1) {
        partner = wrank - 1;
    }
    else if (wrank < wsize - 1) {
        partner = wrank + 1;
    }
    else
        /* Handle wsize odd */
        partner = MPI_PROC_NULL;

    /* Allocate and initialize buffers */
    maxlen = 1024 * 1024;
    rbuf = (char *) malloc(maxlen);
    sbuf = (char *) malloc(maxlen);
    if (!rbuf || !sbuf) {
        fprintf(stderr, "Could not allocate %d byte buffers\n", maxlen);
        MPI_Abort(MPI_COMM_WORLD, 2);
    }
    for (k = 0; k < maxlen; k++) {
        rbuf[k] = 0;
        sbuf[k] = 0;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test Irecv and send, head to head */
    if (wrank == 0 && verbose) {
        printf("Irecv-send\n");
        printf("len\ttime    \trate\n");
    }

    /* Send powers of 2 bytes */
    len = 1;
    for (k = 0; k < 20; k++) {
        /* We use a simple linear form for the number of tests to
         * reduce the impact of the granularity of the timer */
        reps = 50 - k;
        repsleft = reps;
        /* Make sure that both processes are ready to start */
        MPI_Sendrecv(MPI_BOTTOM, 0, MPI_BYTE, partner, 0,
                     MPI_BOTTOM, 0, MPI_BYTE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        t1 = MPI_Wtime();
        while (repsleft--) {
            MPI_Irecv(rbuf, len, MPI_BYTE, partner, k, MPI_COMM_WORLD, &rreq);
            MPI_Send(sbuf, len, MPI_BYTE, partner, k, MPI_COMM_WORLD);
            MPI_Wait(&rreq, MPI_STATUS_IGNORE);
        }
        t1 = MPI_Wtime() - t1;
        times[0][k] = t1 / reps;
        if (wrank == 0) {
            t1 = t1 / reps;
            if (t1 > 0) {
                double rate;
                rate = (len / t1) / 1.e6;
                t1 = t1 * 1.e6;
                if (verbose)
                    printf("%d\t%g\t%g\n", len, t1, len / t1);
            }
            else {
                t1 = t1 * 1.e6;
                if (verbose)
                    printf("%d\t%g\tINF\n", len, t1);
            }
            if (verbose)
                fflush(stdout);
        }

        len *= 2;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test Sendrecv, head to head */
    if (wrank == 0 && verbose) {
        printf("Sendrecv\n");
        printf("len\ttime (usec)\trate (MB/s)\n");
    }

    /* Send powers of 2 bytes */
    len = 1;
    for (k = 0; k < 20; k++) {
        /* We use a simple linear form for the number of tests to
         * reduce the impact of the granularity of the timer */
        reps = 50 - k;
        repsleft = reps;
        /* Make sure that both processes are ready to start */
        MPI_Sendrecv(MPI_BOTTOM, 0, MPI_BYTE, partner, 0,
                     MPI_BOTTOM, 0, MPI_BYTE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        t1 = MPI_Wtime();
        while (repsleft--) {
            MPI_Sendrecv(sbuf, len, MPI_BYTE, partner, k,
                         rbuf, len, MPI_BYTE, partner, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        t1 = MPI_Wtime() - t1;
        times[1][k] = t1 / reps;
        if (wrank == 0) {
            t1 = t1 / reps;
            if (t1 > 0) {
                double rate;
                rate = (len / t1) / 1.e6;
                t1 = t1 * 1.e6;
                if (verbose)
                    printf("%d\t%g\t%g\n", len, t1, len / t1);
            }
            else {
                t1 = t1 * 1.e6;
                if (verbose)
                    printf("%d\t%g\tINF\n", len, t1);
            }
            if (verbose)
                fflush(stdout);
        }

        len *= 2;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Test Send/recv, ping-pong */
    if (wrank == 0 && verbose) {
        printf("Pingpong\n");
        printf("len\ttime (usec)\trate (MB/s)\n");
    }

    /* Send powers of 2 bytes */
    len = 1;
    for (k = 0; k < 20; k++) {
        /* We use a simple linear form for the number of tests to
         * reduce the impact of the granularity of the timer */
        reps = 50 - k;
        repsleft = reps;
        /* Make sure that both processes are ready to start */
        MPI_Sendrecv(MPI_BOTTOM, 0, MPI_BYTE, partner, 0,
                     MPI_BOTTOM, 0, MPI_BYTE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        t1 = MPI_Wtime();
        while (repsleft--) {
            if (wrank & 0x1) {
                MPI_Send(sbuf, len, MPI_BYTE, partner, k, MPI_COMM_WORLD);
                MPI_Recv(rbuf, len, MPI_BYTE, partner, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else {
                MPI_Recv(rbuf, len, MPI_BYTE, partner, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(sbuf, len, MPI_BYTE, partner, k, MPI_COMM_WORLD);
            }
        }
        t1 = MPI_Wtime() - t1;
        times[2][k] = t1 / reps;
        if (wrank == 0) {
            t1 = t1 / reps;
            if (t1 > 0) {
                double rate;
                rate = (len / t1) / 1.e6;
                t1 = t1 * 1.e6;
                if (verbose)
                    printf("%d\t%g\t%g\n", len, t1, len / t1);
            }
            else {
                t1 = t1 * 1.e6;
                if (verbose)
                    printf("%d\t%g\tINF\n", len, t1);
            }
            if (verbose)
                fflush(stdout);
        }

        len *= 2;
    }


    /* At this point, we could optionally analyze the results and report
     * success or failure based on some criteria, such as near monotone
     * increases in bandwidth.  This test was created because of a
     * fall-off in performance noted in the ch3:sock device:channel */

    if (wrank == 0) {
        int nPerfErrors = 0;
        len = 1;
        for (k = 0; k < 20; k++) {
            double T0, T1, T2;
            T0 = times[0][k] * 1.e6;
            T1 = times[1][k] * 1.e6;
            T2 = times[2][k] * 1.e6;
            if (verbose)
                printf("%d\t%12.2f\t%12.2f\t%12.2f\n", len, T0, T1, T2);
            /* Lets look at long messages only */
            if (k > 10) {
                double T0Old, T1Old, T2Old;
                T0Old = times[0][k - 1] * 1.0e6;
                T1Old = times[1][k - 1] * 1.0e6;
                T2Old = times[2][k - 1] * 1.0e6;
                if (T0 > (2 + ERROR_MARGIN) * T0Old) {
                    nPerfErrors++;
                    if (verbose)
                        printf("Irecv-Send:\t%d\t%12.2f\t%12.2f\n", len, T0Old, T0);
                }
                if (T1 > (2 + ERROR_MARGIN) * T1Old) {
                    nPerfErrors++;
                    if (verbose)
                        printf("Sendrecv:\t%d\t%12.2f\t%12.2f\n", len, T1Old, T1);
                }
                if (T2 > (2 + ERROR_MARGIN) * T2Old) {
                    nPerfErrors++;
                    if (verbose)
                        printf("Pingpong:\t%d\t%12.2f\t%12.2f\n", len, T2Old, T2);
                }
            }
            len *= 2;
        }
        if (nPerfErrors > 8) {
            /* Allow for 1-2 errors for eager-rendezvous shifting
             * point and cache effects. There should be a better way
             * of doing this. */
            printf(" Found %d performance errors\n", nPerfErrors);
        }
        else {
            printf(" No Errors\n");
        }
        fflush(stdout);
    }

    free(sbuf);
    free(rbuf);

    MPI_Finalize();

    return 0;
}
