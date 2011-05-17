/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h" 
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"

/* 
 * Tests for lock contention, including special cases within the MPICH2 code 
 * (any MPI implementation should pass these tests; in the MPICH2 case, our
 * coverage analysis showed that the lockcontention.c test was not covering
 * all cases, and in fact, this test revealed a bug in the code).
 *
 * In all of these tests, each process writes (or accesses) the values
 * rank + i*size_of_world for NELM times.
 *
 * This test strives to avoid operations not strictly permitted by MPI RMA,
 * for example, it doesn't target the same locations with multiple put/get
 * calls in the same access epoch.
 */

#define NELM 200
#define NBLOCK 10
#define MAX_ERRS_REPORT 10

/* 
 *  Each process writes data into the rmabuf on the process with target rank
 *  trank.  The final result in rmabuf are the consecutive integers starting
 *  from 0.  Each process, however, does not write a consecutive block.  
 *  Instead, they write these locations:
 *
 *  for i=0,...,NELM-1
 *     for j=0,...,NBLOCK-1
 *         j + NBLOCK * (rank + i * wsize)
 *  
 * The value written is the location.
 *
 * In many cases, multiple RMA operations are needed.  Where these must not
 * overlap, the above pattern is replicated at NBLOCK*NELM*wsize.
 * (NBLOCK is either 1 or NBLOCK in the code below, depending on use) 
 */

static int toterrs = 0;

int testValues( int, int, int, int *, const char * );

int main(int argc, char *argv[]) 
{ 
    int rank, wsize, i, j, cnt;
    int *rmabuf, *localbuf, *localbuf2, *vals;
    MPI_Win win;
    int trank = 0;
    int windowsize;

    MTest_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&wsize); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

    if (wsize < 2) {
        fprintf(stderr, "Run this program with at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    windowsize = (2*NBLOCK + 2) * NELM * wsize;
    rmabuf     = (int *)malloc( windowsize * sizeof(int) );
    localbuf   = (int *)malloc( NELM * sizeof(int) );
    localbuf2  = (int *)malloc( NELM * NBLOCK * sizeof(int) );
    vals       = (int *)malloc( NELM*sizeof(int) );

    /* 
     * Initialize the buffers
     */
    for (i=0; i<NELM; i++) {
      localbuf[i] = rank + i*wsize;
    }
    cnt = 0;
    for (i=0; i<NELM; i++) {
	for (j=0; j<NBLOCK; j++) {
	    localbuf2[cnt++] = j + NBLOCK * (rank + i*wsize);
	}
    }
    for (i=0; i<windowsize; i++) {
      rmabuf[i] = -1;
    }

    /* Create the window */
    MPI_Win_create(rmabuf, windowsize*sizeof(int), sizeof(int), MPI_INFO_NULL, 
		   MPI_COMM_WORLD, &win); 

    /* Multiple puts, with contention at trank */
    MPI_Barrier( MPI_COMM_WORLD );
    for (i=0; i<NELM; i++) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	MPI_Put( &localbuf[i], 1, MPI_INT, trank, 
		 rank + i*wsize, 1, MPI_INT, win );
	MPI_Put( &localbuf[i], 1, MPI_INT, trank, 
		 rank + (i+NELM)*wsize, 1, MPI_INT, win );
	MPI_Win_unlock( trank, win );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == trank) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	toterrs += testValues( 1, NELM, wsize, rmabuf, "Multiple puts (1)" );
	toterrs += testValues( 1, NELM, wsize, rmabuf + wsize*NELM,
			       "Multiple puts (2)" );
	MPI_Win_unlock( trank, win );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    /* Reinit the rmabuf */
    for (i=0; i<windowsize; i++) {
      rmabuf[i] = -1;
    }
    MPI_Barrier( MPI_COMM_WORLD );

    /* Single put with contention */
    trank = 0;
    for (i=0; i<NELM; i++) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	MPI_Put( &localbuf[i], 1, MPI_INT, trank, rank + i*wsize, 1, MPI_INT, 
		 win );
	MPI_Win_unlock( trank, win );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == trank) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	toterrs += testValues( 1, NELM, wsize, rmabuf, "Single put" );
	MPI_Win_unlock( trank, win );
    }

    /* Reinit the rmabuf */
    for (i=0; i<windowsize; i++) {
	rmabuf[i] = -1;
    }
    /* Longer puts with contention at trank */
    MPI_Barrier( MPI_COMM_WORLD );
    for (i=0; i<NELM; i++) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	if (rank != trank) {
	    MPI_Put( &localbuf2[i*NBLOCK], NBLOCK, MPI_INT, trank, 
		     NELM * wsize + NBLOCK*(rank+i*wsize), NBLOCK, 
		     MPI_INT, win );
	    MPI_Put( &localbuf2[i*NBLOCK], NBLOCK, MPI_INT, trank, 
		     NELM * wsize + NBLOCK*(rank+(i+NELM)*wsize), NBLOCK, 
		     MPI_INT, win );
	}
	MPI_Put( &localbuf[i], 1, MPI_INT, trank, rank+i*wsize, 1, MPI_INT, 
		 win );
	MPI_Win_unlock( trank, win );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == trank) {
	/* For simplicity in testing, set the values that rank==trank
	   would have set. */
	for (i=0; i<NELM; i++) {
	    for (j=0; j<NBLOCK; j++) {
		rmabuf[NELM*wsize + NBLOCK*(trank+i*wsize) + j] = 
		    j + NBLOCK*(trank +i*wsize);
		rmabuf[NELM*wsize + NBLOCK*(trank+(i+NELM)*wsize) + j] = 
		    j + NBLOCK*(trank + i*wsize);
	    }
	}
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	toterrs += testValues( 1, NELM, wsize, rmabuf, "Long puts (1)" );
	toterrs += testValues( NBLOCK, NELM, wsize, rmabuf + NELM * wsize,
			       "Long puts(2)" );
	toterrs += testValues( NBLOCK, NELM, wsize, 
			       rmabuf + NELM * wsize * (1 + NBLOCK),
			       "Long puts(3)" );
	MPI_Win_unlock( trank, win );
    }
    
    /* Reinit the rmabuf */
    for (i=0; i<windowsize; i++) {
	rmabuf[i] = -1;
    }
    for (i=0; i< NELM; i++) 
	vals[i] = -2;
    
    /* Put mixed with Get */
    MPI_Barrier( MPI_COMM_WORLD );
    for (i=0; i<NELM; i++) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	if (rank != trank) {
	    MPI_Put( &localbuf2[i], NBLOCK, MPI_INT, trank, 
		     NELM*wsize + NBLOCK*(rank + i*wsize), NBLOCK, MPI_INT, 
		     win );
	    MPI_Put( &localbuf[i], 1, MPI_INT, trank, 
		     rank + i*wsize, 1, MPI_INT, win );
	}
	else {
	    MPI_Get( &vals[i], 1, MPI_INT, trank, i, 1, MPI_INT, win );
	}
	MPI_Win_unlock( trank, win );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == trank) {
	/* Just test the Get */
	for (i=0; i<wsize; i++) {
	    if (i == trank) {
		if (vals[i] != -1) {
		    toterrs++;
		    if (toterrs < MAX_ERRS_REPORT) {
			printf( "put/get: vals[%d] = %d, expected -1\n",
				i, vals[i] );
		    }
		}
	    }
	    else if (vals[i] != i && vals[i] != -1) {
		toterrs++;
		if (toterrs < MAX_ERRS_REPORT) {
		    printf( "put/get: vals[%d] = %d, expected -1 or %d\n",
			    i, vals[i], i );
		}
	    }
	}
    }

    /* Contention only with get */
    for (i=0; i<windowsize; i++) {
	rmabuf[i] = -i;
    }
    for (i=0; i<NELM; i++)
	vals[i] = -2;

    MPI_Barrier( MPI_COMM_WORLD );
    for (i=0; i<NELM; i++) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	MPI_Get( &vals[i], 1, MPI_INT, trank, i, 1, MPI_INT, win );
	MPI_Win_unlock( trank, win );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == trank) {
	for (i=0; i<NELM; i++) {
	    if (vals[i] != -i) {
		toterrs++;
		if (toterrs < MAX_ERRS_REPORT) {
		    printf( "single get: vals[%d] = %d, expected %d\n",
			    i, vals[i], -i );
		}
	    }
	}
    }

    /* Contention with accumulate */
    MPI_Barrier( MPI_COMM_WORLD );
    for (i=0; i<NELM*wsize; i++) {
	rmabuf[i] = 0;
    }
    MPI_Barrier( MPI_COMM_WORLD );
    for (i=0; i<NELM; i++) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	MPI_Accumulate( &localbuf[i], 1, MPI_INT, trank, rank+i*wsize, 
			1, MPI_INT, MPI_SUM, win );
	MPI_Accumulate( &localbuf[i], 1, MPI_INT, trank, rank+i*wsize, 
			1, MPI_INT, MPI_SUM, win );
	MPI_Win_unlock( trank, win );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == trank) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, trank, 0, win );
	for (i=0; i<NELM * wsize; i++) {
	    if (rmabuf[i] != 2*i) {
		toterrs++;
		if (toterrs < MAX_ERRS_REPORT) {
		    printf( "2 accumulate: rmabuf[%d] = %d, expected %d\n",
			    i, rmabuf[i], 2*i );
		}
	    }
	}
	MPI_Win_unlock( trank, win );
    }

    MPI_Win_free( &win );

    free( rmabuf );
    free( localbuf );
    free( localbuf2 );
    free( vals );
   
    MTest_Finalize(toterrs);
    MPI_Finalize(); 
    return 0; 
} 

/* Test the values in the rmabuf against the expected values.  Return the 
   number of errors */
int testValues( int nb, int nelm, int wsize, int *rmabuf, const char *msg )
{
    int i, errs = 0;
		
    for (i=0; i<nb * nelm * wsize; i++) {
	if (rmabuf[i] != i) {
	    if (toterrs + errs < MAX_ERRS_REPORT) {
		printf( "%s:rmabuf[%d] = %d expected %d\n", 
			msg, i, rmabuf[i], i );
	    }
	    errs++;
	}
    }

    return errs;
}
