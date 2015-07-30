/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

#define ROOT      0
#define NUM_REPS  5
#define NUM_SIZES 4

int main(int argc, char **argv)
{
    int *buf;
    int i, rank, reps, n;
    int bVerify = 1;
    int sizes[NUM_SIZES] = { 100, 64 * 1024, 128 * 1024, 1024 * 1024 };
    int num_errors = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc > 1) {
        if (strcmp(argv[1], "-novalidate") == 0 || strcmp(argv[1], "-noverify") == 0)
            bVerify = 0;
    }

    buf = (int *) malloc(sizes[NUM_SIZES - 1] * sizeof(int));
    memset(buf, 0, sizes[NUM_SIZES - 1] * sizeof(int));

    for (n = 0; n < NUM_SIZES; n++) {
#ifdef DEBUG
        if (rank == ROOT) {
            printf("bcasting %d MPI_INTs %d times\n", sizes[n], NUM_REPS);
            fflush(stdout);
        }
#endif
        for (reps = 0; reps < NUM_REPS; reps++) {
            if (bVerify) {
                if (rank == ROOT) {
                    for (i = 0; i < sizes[n]; i++) {
                        buf[i] = 1000000 * (n * NUM_REPS + reps) + i;
                    }
                }
                else {
                    for (i = 0; i < sizes[n]; i++) {
                        buf[i] = -1 - (n * NUM_REPS + reps);
                    }
                }
            }

#	    ifdef DEBUG
            {
                printf("rank=%d, n=%d, reps=%d\n", rank, n, reps);
            }
#           endif

            MPI_Bcast(buf, sizes[n], MPI_INT, ROOT, MPI_COMM_WORLD);

            if (bVerify) {
                num_errors = 0;
                for (i = 0; i < sizes[n]; i++) {
                    if (buf[i] != 1000000 * (n * NUM_REPS + reps) + i) {
                        num_errors++;
                        if (num_errors < 10) {
                            printf("Error: Rank=%d, n=%d, reps=%d, i=%d, buf[i]=%d expected=%d\n",
                                   rank, n, reps, i, buf[i], 1000000 * (n * NUM_REPS + reps) + i);
                            fflush(stdout);
                        }
                    }
                }
                if (num_errors >= 10) {
                    printf("Error: Rank=%d, num_errors = %d\n", rank, num_errors);
                    fflush(stdout);
                }
            }
        }
    }

    free(buf);

    MTest_Finalize(num_errors);
    MPI_Finalize();
    return 0;
}
