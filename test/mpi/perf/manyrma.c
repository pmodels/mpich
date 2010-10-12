/* This test measures the performance of many rma operations to a single 
   target process.
   It uses a number of operations (put, get, or accumulate) to different
   locations in the target window */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_COUNT 65536
#define MAX_RUNS 10

static int verbose = 1;

int main( int argc, char *argv[] )
{
    int arraysize, i, k, cnt, maxCount, *arraybuffer;
    int one = 1;
    int wrank, wsize, destRank;
    MPI_Win win;
    double t1[MAX_RUNS], t2[MAX_RUNS], t3[MAX_RUNS];

    MPI_Init( &argc, &argv );
    
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    destRank = wrank + 1;
    while (destRank >= wsize) destRank = destRank - wsize;
    arraysize = MAX_COUNT;
    arraybuffer = (int*)malloc( arraysize * sizeof(int) );
    if (!arraybuffer) {
	fprintf( stderr, "Unable to allocate %d words\n", arraysize );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    MPI_Win_create( arraybuffer, arraysize*sizeof(int), (int)sizeof(int),
		    MPI_INFO_NULL, MPI_COMM_WORLD, &win );

    maxCount = MAX_COUNT;
    maxCount = 33000;
    cnt = 1;
    while (cnt <= maxCount) {
	for (k=0; k<MAX_RUNS; k++) {
	    MPI_Win_fence( 0, win );
	    MPI_Barrier( MPI_COMM_WORLD );
	    t1[k] = MPI_Wtime();
	    for (i=0; i<cnt; i++) {
		MPI_Accumulate( &one, 1, MPI_INT, destRank, 
				i, 1, MPI_INT, MPI_SUM, win );
	    }
	    t2[k] = MPI_Wtime();
	    MPI_Win_fence( 0, win );
	    t3[k] = MPI_Wtime();
	}
	if (wrank == 0) {
	    double d1=0, d2=0;
	    for (k=0; k<MAX_RUNS; k++) {
		d1 += t2[k] - t1[k];
		d2 += t3[k] - t2[k];
	    }
	    if (verbose) {
		printf( "%d\t%e\t%e\t%e\t%e\n", cnt, 
			d1 / MAX_RUNS, d2 / MAX_RUNS, 
			d1 / (MAX_RUNS * cnt), d2 / (MAX_RUNS * cnt) );
	    }
	    /* FIXME: we need a test on performance consistency.
	       The test needs to have both a relative growth limit and
	       an absolute limit.
	     */
	}
	
	cnt = 2 * cnt;
    }

    MPI_Win_free( &win );
    
    MPI_Finalize();
    return 0;
}
