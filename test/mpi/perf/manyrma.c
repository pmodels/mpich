/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test measures the performance of many rma operations to a single 
   target process.
   It uses a number of operations (put or accumulate) to different
   locations in the target window 
   This is one of the ways that RMA may be used, and is used in the 
   reference implementation of the graph500 benchmark.
*/
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COUNT 65536
#define MAX_RMA_SIZE 16
#define MAX_RUNS 10

typedef enum { SYNC_NONE=0, 
	       SYNC_ALL=-1, SYNC_FENCE=1, SYNC_LOCK=2, SYNC_PSCW=4 } sync_t;
typedef enum { RMA_NONE=0, RMA_ALL=-1, RMA_PUT=1, RMA_ACC=2, RMA_GET=4 } rma_t;
/* Note GET not yet implemented */
sync_t syncChoice = SYNC_ALL;
rma_t rmaChoice = RMA_ALL;

typedef struct {
    double startOp, endOp, endSync;
} timing;

static int verbose = 1;
static int barrierSync = 0;
static double tickThreshold = 0.0;

void PrintResults( int cnt, timing t[] );
void RunAccFence( MPI_Win win, int destRank, int cnt, int sz, timing t[] );
void RunAccLock( MPI_Win win, int destRank, int cnt, int sz, timing t[] );
void RunPutFence( MPI_Win win, int destRank, int cnt, int sz, timing t[] );
void RunPutLock( MPI_Win win, int destRank, int cnt, int sz, timing t[] );
void RunAccPSCW( MPI_Win win, int destRank, int cnt, int sz, 
		 MPI_Group exposureGroup, MPI_Group accessGroup, timing t[] );
void RunPutPSCW( MPI_Win win, int destRank, int cnt, int sz, 
		 MPI_Group exposureGroup, MPI_Group accessGroup, timing t[] );

int main( int argc, char *argv[] )
{
    int arraysize, i, cnt, sz, maxCount, *arraybuffer;
    int wrank, wsize, destRank, srcRank;
    MPI_Win win;
    MPI_Group wgroup, accessGroup, exposureGroup;
    timing t[MAX_RUNS];
    int    maxSz = MAX_RMA_SIZE;

    MPI_Init( &argc, &argv );

    /* Determine clock accuracy */
    tickThreshold = 10.0 * MPI_Wtick();
    MPI_Allreduce( MPI_IN_PLACE, &tickThreshold, 1, MPI_DOUBLE, MPI_MAX, 
		   MPI_COMM_WORLD );

    for (i=1; i<argc; i++) {
	if (strcmp( argv[i], "-put" ) == 0) {
	    if (rmaChoice == RMA_ALL) rmaChoice = RMA_NONE;
	    rmaChoice  |= RMA_PUT;
	}
	else if (strcmp( argv[i], "-acc" ) == 0) {
	    if (rmaChoice == RMA_ALL) rmaChoice = RMA_NONE;
	    rmaChoice  |= RMA_ACC;
	}
	else if (strcmp( argv[i], "-fence" ) == 0) {
	    if (syncChoice == SYNC_ALL) syncChoice = SYNC_NONE;
	    syncChoice |= SYNC_FENCE;
	}
	else if (strcmp( argv[i], "-lock" ) == 0) {
	    if (syncChoice == SYNC_ALL) syncChoice = SYNC_NONE;
	    syncChoice |= SYNC_LOCK;
	}
	else if (strcmp( argv[i], "-pscw" ) == 0) {
	    if (syncChoice == SYNC_ALL) syncChoice = SYNC_NONE;
	    syncChoice |= SYNC_PSCW;
	}
	else if (strcmp( argv[i], "-maxsz" ) == 0) {
	    i++;
	    maxSz = atoi( argv[i] );
	}
	else if (strcmp( argv[i], "-barrier" ) == 0) {
	    barrierSync = 1;
	}
	else {
	    fprintf( stderr, "Unrecognized argument %s\n", argv[i] );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
	}
    }
    
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    destRank = wrank + 1;
    while (destRank >= wsize) destRank = destRank - wsize;
    srcRank = wrank - 1;
    if (srcRank < 0) srcRank += wsize;

    /* Create groups for PSCW */
    MPI_Comm_group( MPI_COMM_WORLD, &wgroup );
    MPI_Group_incl( wgroup, 1, &destRank, &accessGroup );
    MPI_Group_incl( wgroup, 1, &srcRank, &exposureGroup );
    MPI_Group_free( &wgroup );

    arraysize = maxSz * MAX_COUNT;
    arraybuffer = (int*)malloc( arraysize * sizeof(int) );
    if (!arraybuffer) {
	fprintf( stderr, "Unable to allocate %d words\n", arraysize );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    MPI_Win_create( arraybuffer, arraysize*sizeof(int), (int)sizeof(int),
		    MPI_INFO_NULL, MPI_COMM_WORLD, &win );

    /* FIXME: we need a test on performance consistency.
       The test needs to have both a relative growth limit and
       an absolute limit.
    */

    maxCount = MAX_COUNT;

    if ((syncChoice & SYNC_FENCE) && (rmaChoice & RMA_ACC)) {
	for (sz=1; sz<=maxSz; sz = sz + sz) {
	    if (wrank == 0) 
		printf( "Accumulate with fence, %d elements\n", sz );
	    cnt = 1;
	    while (cnt <= maxCount) {
		RunAccFence( win, destRank, cnt, sz, t );
		if (wrank == 0) {
		    PrintResults( cnt, t );
		}
		cnt = 2 * cnt;
	    }
	}
    }

    if ((syncChoice & SYNC_LOCK) && (rmaChoice & RMA_ACC)) {
	for (sz=1; sz<=maxSz; sz = sz + sz) {
	    if (wrank == 0) 
		printf( "Accumulate with lock, %d elements\n", sz );
	    cnt = 1;
	    while (cnt <= maxCount) {
		RunAccLock( win, destRank, cnt, sz, t );
		if (wrank == 0) {
		    PrintResults( cnt, t );
		}
		cnt = 2 * cnt;
	    }
	}
    }

    if ((syncChoice & SYNC_FENCE) && (rmaChoice & RMA_PUT)) {
	for (sz=1; sz<=maxSz; sz = sz + sz) {
	    if (wrank == 0) 
		printf( "Put with fence, %d elements\n", sz );
	    cnt = 1;
	    while (cnt <= maxCount) {
		RunPutFence( win, destRank, cnt, sz, t );
		if (wrank == 0) {
		    PrintResults( cnt, t );
		}
		cnt = 2 * cnt;
	    }
	}
    }

    if ((syncChoice & SYNC_LOCK) && (rmaChoice & RMA_PUT)) {
	for (sz=1; sz<=maxSz; sz = sz + sz) {
	    if (wrank == 0) 
		printf( "Put with lock, %d elements\n", sz );
	    cnt = 1;
	    while (cnt <= maxCount) {
		RunPutLock( win, destRank, cnt, sz, t );
		if (wrank == 0) {
		    PrintResults( cnt, t );
		}
		cnt = 2 * cnt;
	    }
	}
    }

    if ((syncChoice & SYNC_PSCW) && (rmaChoice & RMA_PUT)) {
	for (sz=1; sz<=maxSz; sz = sz + sz) {
	    if (wrank == 0) 
		printf( "Put with pscw, %d elements\n", sz );
	    cnt = 1;
	    while (cnt <= maxCount) {
		RunPutPSCW( win, destRank, cnt, sz, 
			    exposureGroup, accessGroup, t );
		if (wrank == 0) {
		    PrintResults( cnt, t );
		}
		cnt = 2 * cnt;
	    }
	}
    }

    if ((syncChoice & SYNC_PSCW) && (rmaChoice & RMA_ACC)) {
	for (sz=1; sz<=maxSz; sz = sz + sz) {
	    if (wrank == 0) 
		printf( "Accumulate with pscw, %d elements\n", sz );
	    cnt = 1;
	    while (cnt <= maxCount) {
		RunAccPSCW( win, destRank, cnt, sz, 
			    exposureGroup, accessGroup, t );
		if (wrank == 0) {
		    PrintResults( cnt, t );
		}
		cnt = 2 * cnt;
	    }
	}
    }

    MPI_Win_free( &win );

    MPI_Group_free( &accessGroup );
    MPI_Group_free( &exposureGroup );
    
    MPI_Finalize();
    return 0;
}


void RunAccFence( MPI_Win win, int destRank, int cnt, int sz, timing t[] )
{
    int k, i, j, one = 1;

    for (k=0; k<MAX_RUNS; k++) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_fence( 0, win );
	j = 0;
	t[k].startOp = MPI_Wtime();
	for (i=0; i<cnt; i++) {
	    MPI_Accumulate( &one, sz, MPI_INT, destRank, 
			    j, sz, MPI_INT, MPI_SUM, win );
	    j += sz;
	}
	t[k].endOp = MPI_Wtime();
	if (barrierSync) MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_fence( 0, win );
	t[k].endSync = MPI_Wtime();
    }
}

void RunAccLock( MPI_Win win, int destRank, int cnt, int sz, timing t[] )
{
    int k, i, j, one = 1;

    for (k=0; k<MAX_RUNS; k++) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_lock( MPI_LOCK_SHARED, destRank, 0, win );
	j = 0;
	t[k].startOp = MPI_Wtime();
	for (i=0; i<cnt; i++) {
	    MPI_Accumulate( &one, sz, MPI_INT, destRank, 
			    j, sz, MPI_INT, MPI_SUM, win );
	    j += sz;
	}
	t[k].endOp = MPI_Wtime();
	if (barrierSync) MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_unlock( destRank, win );
	t[k].endSync = MPI_Wtime();
    }
}

void RunPutFence( MPI_Win win, int destRank, int cnt, int sz, timing t[] )
{
    int k, i, j, one = 1;

    for (k=0; k<MAX_RUNS; k++) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_fence( 0, win );
	j = 0;
	t[k].startOp = MPI_Wtime();
	for (i=0; i<cnt; i++) {
	    MPI_Put( &one, sz, MPI_INT, destRank, 
			    j, sz, MPI_INT, win );
	    j += sz;
	}
	t[k].endOp = MPI_Wtime();
	if (barrierSync) MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_fence( 0, win );
	t[k].endSync = MPI_Wtime();
    }
}

void RunPutLock( MPI_Win win, int destRank, int cnt, int sz, timing t[] )
{
    int k, i, j, one = 1;

    for (k=0; k<MAX_RUNS; k++) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_lock( MPI_LOCK_SHARED, destRank, 0, win );
	j = 0;
	t[k].startOp = MPI_Wtime();
	for (i=0; i<cnt; i++) {
	    MPI_Put( &one, sz, MPI_INT, destRank, j, sz, MPI_INT, win );
	    j += sz;
	}
	t[k].endOp = MPI_Wtime();
	if (barrierSync) MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_unlock( destRank, win );
	t[k].endSync = MPI_Wtime();
    }
}

void RunPutPSCW( MPI_Win win, int destRank, int cnt, int sz, 
		 MPI_Group exposureGroup, MPI_Group accessGroup, timing t[] )
{
    int k, i, j, one = 1;

    for (k=0; k<MAX_RUNS; k++) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_post( exposureGroup, 0, win );
	MPI_Win_start( accessGroup, 0, win );
	j = 0;
	t[k].startOp = MPI_Wtime();
	for (i=0; i<cnt; i++) {
	    MPI_Put( &one, sz, MPI_INT, destRank, j, sz, MPI_INT, win );
	    j += sz;
	}
	t[k].endOp = MPI_Wtime();
	if (barrierSync) MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_complete( win );
	MPI_Win_wait( win );
	t[k].endSync = MPI_Wtime();
    }
}

void RunAccPSCW( MPI_Win win, int destRank, int cnt, int sz, 
		 MPI_Group exposureGroup, MPI_Group accessGroup, timing t[] )
{
    int k, i, j, one = 1;

    for (k=0; k<MAX_RUNS; k++) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_post( exposureGroup, 0, win );
	MPI_Win_start( accessGroup, 0, win );
	j = 0;
	t[k].startOp = MPI_Wtime();
	for (i=0; i<cnt; i++) {
	    MPI_Accumulate( &one, sz, MPI_INT, destRank, 
			    j, sz, MPI_INT, MPI_SUM, win );
	    j += sz;
	}
	t[k].endOp = MPI_Wtime();
	if (barrierSync) MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_complete( win );
	MPI_Win_wait( win );
	t[k].endSync = MPI_Wtime();
    }
}

void PrintResults( int cnt, timing t[] )
{
    int k;
    double d1=0, d2=0;
    double minD1 = 1e10, minD2 = 1e10;
    double tOp, tSync;
    for (k=0; k<MAX_RUNS; k++) {
	tOp   = t[k].endOp - t[k].startOp;
	tSync = t[k].endSync - t[k].endOp;
	d1    += tOp;
	d2    += tSync;
	if (tOp < minD1)   minD1 = tOp;
	if (tSync < minD2) minD2 = tSync;
    }
    if (verbose) {
	long rate = 0;
	/* Use the minimum times because they are more stable - if timing
	   accuracy is an issue, use the min over multiple trials */
	d1 = minD1;
	d2 = minD2;
	/* d1 = d1 / MAX_RUNS; d2 = d2 / MAX_RUNS); */
	if (d2 > 0) rate = (long)(cnt) / d2;
	printf( "%d\t%e\t%e\t%e\t%e\t%ld\n", cnt, 
		d1, d2, 
		d1 / cnt, d2 / cnt, rate );
    }
}
