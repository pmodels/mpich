/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* 
 * Check that the timer produces monotone nondecreasing times and that
 * the Tick is reasonable
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static int verbose = 0;

#define MAX_TIMER_TEST 5000

int main(int argc, char* argv[])
{
    double t1[MAX_TIMER_TEST], tick[MAX_TIMER_TEST], tickval;
    double minDiff, maxDiff, diff;
    int i, nZeros = 0;
    int errs = 0;

    MTest_Init(&argc,&argv);

    for (i=0; i<MAX_TIMER_TEST; i++) {
	t1[i] = MPI_Wtime();
    }

    for (i=0; i<MAX_TIMER_TEST; i++) {
	tick[i] = MPI_Wtick();
    }

    /* Look at the values */
    /* Look at the tick */
    tickval = MPI_Wtick();
    for (i=0; i<MAX_TIMER_TEST; i++) {
	if (tickval != tick[i]) {
	    fprintf( stderr, "Nonconstant value for MPI_Wtick: %e != %e\n",
		     tickval, tick[i] );
	    errs ++;
	}
    }

    /* Look at the timer */
    minDiff = 1.e20;
    maxDiff = -1.0;
    nZeros  = 0;
    for (i=1; i<MAX_TIMER_TEST; i++) {
	diff = t1[i] - t1[i-1];
	if (diff == 0.0) nZeros++;
	else if (diff < minDiff) minDiff = diff;
	if (diff > maxDiff) maxDiff = diff;
    }

    /* Are the time diff values and tick values consistent */
    if (verbose) {
	printf( "Tick = %e, timer range = [%e,%e]\n", tickval, minDiff, 
		maxDiff );
	if (nZeros) printf( "Wtime difference was 0 %d times\n", nZeros );
    }    

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
