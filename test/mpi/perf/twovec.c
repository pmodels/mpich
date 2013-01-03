/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

/* Make sure datatype creation is independent of data size */

#define SKIP 4
#define NUM_SIZES 16
#define FRACTION 0.2

/* Don't make the number of loops too high; we create so many
 * datatypes before trying to free them */
#define LOOPS 1024

int main(int argc, char *argv[])
{
    MPI_Datatype column[LOOPS], xpose[LOOPS];
    double t[NUM_SIZES], ttmp, tmin, tmax, tmean, tdiff;
    int size;
    int i, j, isMonotone, errs = 0, nrows, ncols, isvalid;

    MPI_Init(&argc, &argv);

    tmean = 0;
    size = 1;
    for (i = 0; i < NUM_SIZES + SKIP; i++) {
        nrows = ncols = size;

        ttmp = MPI_Wtime();

        for (j = 0; j < LOOPS; j++) {
            MPI_Type_vector(nrows, 1, ncols, MPI_INT, &column[j]);
            MPI_Type_hvector(ncols, 1, sizeof(int), column[j], &xpose[j]);
            MPI_Type_commit(&xpose[j]);
        }

        if (i >= SKIP) {
            t[i - SKIP] = MPI_Wtime() - ttmp;
            tmean += t[i - SKIP];
        }

        for (j = 0; j < LOOPS; j++) {
            MPI_Type_free(&xpose[j]);
            MPI_Type_free(&column[j]);
        }

        if (i >= SKIP)
            size *= 2;
    }
    tmean /= NUM_SIZES;

    /* Now, analyze the times to see that they are nearly independent
     * of size */
    for (i = 0; i < NUM_SIZES; i++) {
        /* The difference between the value and the mean is more than
         * a "FRACTION" of mean. */
        if (fabs(t[i] - tmean) > (FRACTION * tmean))
            errs++;
    }

    if (errs) {
        fprintf(stderr, "too much difference in performance: ");
        for (i = 0; i < NUM_SIZES; i++)
            fprintf(stderr, "%.3f ", t[i] * 1e6);
        fprintf(stderr, "\n");
    }
    else
        printf(" No Errors\n");

    MPI_Finalize();
    return 0;
}
