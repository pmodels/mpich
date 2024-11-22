/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run coll_bcasttest
int run(const char *arg);
#endif

#define ROOT      0
#define NUM_REPS  5
#define NUM_SIZES 4

int run(const char *arg)
{
    int *buf;
    int i, rank, reps, n;
    int bVerify = 1;
    int sizes[NUM_SIZES] = { 100, 64 * 1024, 128 * 1024, 1024 * 1024 };
    int errs = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MTestArgList *head = MTestArgListCreate_arg(arg);
    if (MTestArgListGetInt_with_default(head, "noverify", 0)) {
        bVerify = 0;
    }
    MTestArgListDestroy(head);

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
                } else {
                    for (i = 0; i < sizes[n]; i++) {
                        buf[i] = -1 - (n * NUM_REPS + reps);
                    }
                }
            }
#	    ifdef DEBUG
            {
                printf("rank=%d, n=%d, reps=%d\n", rank, n, reps);
            }
#endif

            MPI_Bcast(buf, sizes[n], MPI_INT, ROOT, MPI_COMM_WORLD);

            if (bVerify) {
                errs = 0;
                for (i = 0; i < sizes[n]; i++) {
                    if (buf[i] != 1000000 * (n * NUM_REPS + reps) + i) {
                        errs++;
                        if (errs < 10) {
                            printf("Error: Rank=%d, n=%d, reps=%d, i=%d, buf[i]=%d expected=%d\n",
                                   rank, n, reps, i, buf[i], 1000000 * (n * NUM_REPS + reps) + i);
                            fflush(stdout);
                        }
                    }
                }
                if (errs >= 10) {
                    printf("Error: Rank=%d, errs = %d\n", rank, errs);
                    fflush(stdout);
                }
            }
        }
    }

    free(buf);

    return errs;
}
