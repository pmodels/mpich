/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

/* Make sure datatype creation is independent of data size
   Note, however, that there is no guarantee or expectation
   that the time would be constant.  In particular, some
   optimizations might take more time than others.

   The real goal of this is to ensure that the time to create
   a datatype doesn't increase strongly with the number of elements
   within the datatype, particularly for these datatypes that are
   quite simple patterns.
 */

#define SKIP 4
#define NUM_SIZES 16
#define FRACTION 1.0

/* Don't make the number of loops too high; we create so many
 * datatypes before trying to free them */
#define LOOPS 1024

int main(int argc, char *argv[])
{
    MPI_Datatype column[LOOPS], xpose[LOOPS];
    double t[NUM_SIZES], ttmp, tmin, tmax, tmean, tdiff;
    double tMeanLower, tMeanHigher;
    int size;
    int i, j, errs = 0, nrows, ncols;

    MPI_Init(&argc, &argv);

    tmean = 0;
    size = 1;
    for (i = -SKIP; i < NUM_SIZES; i++) {
        nrows = ncols = size;

        ttmp = MPI_Wtime();

        for (j = 0; j < LOOPS; j++) {
            MPI_Type_vector(nrows, 1, ncols, MPI_INT, &column[j]);
            MPI_Type_hvector(ncols, 1, sizeof(int), column[j], &xpose[j]);
            MPI_Type_commit(&xpose[j]);
        }

        if (i >= 0) {
            t[i] = MPI_Wtime() - ttmp;
            if (t[i] < 100 * MPI_Wtick()) {
                /* Time is too inaccurate to use.  Set to zero.
                 * Consider increasing the LOOPS value to make this
                 * time large enough */
                t[i] = 0;
            }
            tmean += t[i];
        }

        for (j = 0; j < LOOPS; j++) {
            MPI_Type_free(&xpose[j]);
            MPI_Type_free(&column[j]);
        }

        if (i >= 0)
            size *= 2;
    }
    tmean /= NUM_SIZES;

    /* Now, analyze the times to see that they do not grow too fast
     * as a function of size.  As that is a vague criteria, we do the
     * following as a simple test:
     * Compute the mean of the first half and the second half of the
     * data
     * Compare the two means
     * If the mean of the second half is more than FRACTION times the
     * mean of the first half, then the time may be growing too fast.
     */
    tMeanLower = tMeanHigher = 0;
    for (i = 0; i < NUM_SIZES / 2; i++)
        tMeanLower += t[i];
    tMeanLower /= (NUM_SIZES / 2);
    for (i = NUM_SIZES / 2; i < NUM_SIZES; i++)
        tMeanHigher += t[i];
    tMeanHigher /= (NUM_SIZES - NUM_SIZES / 2);
    /* A large value (even 1 or greater) is a good choice for
     * FRACTION here - the goal is to detect significant growth in
     * execution time as the size increases, and there is no MPI
     * standard requirement here to meet.
     *
     * If the times were too small, then the test also passes - the
     * goal is to find implementation problems that lead to excessive
     * time in these routines.
     */
    if (tMeanLower > 0 && tMeanHigher > (1 + FRACTION) * tMeanLower)
        errs++;

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
