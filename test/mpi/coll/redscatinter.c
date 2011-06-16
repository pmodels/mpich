/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* 
 * Test of reduce scatter with large data on an intercommunicator
 * (needed in MPICH2 to trigger the long-data algorithm)
 *
 * Each processor contributes its rank + the index to the reduction, 
 * then receives the ith sum
 *
 * Can be called with any number of processors.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

int main( int argc, char **argv )
{
    int      err = 0;
    int      *sendbuf, *recvbuf, *recvcounts;
    int      size, rsize, rank, i, sumval;
    int      recvcount, /* Each process receives this much data */
             sendcount, /* Each process contributes this much data */
	     basecount; /* Unit of elements - basecount *rsize is recvcount, 
			   etc. */
    int      isLeftGroup;
    MPI_Comm comm;


    MTest_Init( &argc, &argv );
    comm = MPI_COMM_WORLD;

    basecount = 1024;

    while (MTestGetIntercomm( &comm, &isLeftGroup, 2 )) {
	if (comm == MPI_COMM_NULL) continue;

	MPI_Comm_remote_size( comm, &rsize );
	MPI_Comm_size( comm, &size );
	MPI_Comm_rank( comm, &rank );

	if (0) {
	    printf( "[%d] %s (%d,%d) remote %d\n", rank, 
		    isLeftGroup ? "L" : "R", 
		    rank, size, rsize );
	}

	recvcount = basecount * rsize;
	sendcount = basecount * rsize * size;

	recvcounts = (int *)malloc( size * sizeof(int) );
	if (!recvcounts) {
	    fprintf( stderr, "Could not allocate %d int for recvcounts\n", 
		     size );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
	}
	for (i=0; i<size; i++) 
	    recvcounts[i] = recvcount;
	
	sendbuf = (int *) malloc( sendcount * sizeof(int) );
	if (!sendbuf) {
	    fprintf( stderr, "Could not allocate %d ints for sendbuf\n", 
		     sendcount );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
	}

	for (i=0; i<sendcount; i++) {
	    sendbuf[i] = rank*sendcount + i;
	}
	recvbuf = (int *)malloc( recvcount * sizeof(int) );
	if (!recvbuf) {
	    fprintf( stderr, "Could not allocate %d ints for recvbuf\n", 
		     recvcount );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
	}
	for (i=0; i<recvcount; i++) {
	    recvbuf[i] = -i;
	}
	
	MPI_Reduce_scatter( sendbuf, recvbuf, recvcounts, MPI_INT, MPI_SUM, 
			    comm );

	/* Check received data */
	for (i=0; i<recvcount; i++) {
	    sumval = sendcount * (rsize * (rsize-1))/2 + 
		(i + rank * rsize * basecount) * rsize;
	    if (recvbuf[i] != sumval) {
		err++;
		if (err < 10) {
		    fprintf( stdout, "Did not get expected value for reduce scatter\n" );
		    fprintf( stdout, "[%d] %s recvbuf[%d] = %d, expected %d\n", 
			     rank, 
			     isLeftGroup ? "L" : "R", 
			     i, recvbuf[i], sumval );
		}
	    }
	}
	
	free(sendbuf);
	free(recvbuf);
	free(recvcounts);

	MTestFreeComm( &comm );
    }

    MTest_Finalize( err );

    MPI_Finalize( );

    return 0;
}
