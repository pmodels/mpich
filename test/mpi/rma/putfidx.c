/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/*
static char MTEST_Descrip[] = "Put with Fence for an indexed datatype";
*/

int CheckMPIErr( int err );

int main( int argc, char *argv[] )
{
    int           errs = 0, err;
    int           i, rank, size, source, dest;
    int           blksize, totsize;
    int           *recvBuf = 0, *srcBuf = 0;
    MPI_Comm      comm;
    MPI_Win       win;
    MPI_Aint      extent;
    MPI_Datatype  originType;
    int           counts[2];
    int           displs[2];

    MTest_Init( &argc, &argv );

    /* Select the communicator and datatypes */
    comm = MPI_COMM_WORLD;

    /* Create the datatype */
    /* One MPI Implementation fails this test with sufficiently large 
       values of blksize - it appears to convert this type to an 
       incorrect contiguous move */
    blksize = 2048;
    counts[0] = blksize;
    counts[1] = blksize;
    displs[0] = 0;
    displs[1] = blksize + 1;
    MPI_Type_indexed( 2, counts, displs, MPI_INT, &originType );
    MPI_Type_commit( &originType );

    totsize = 2 * blksize;

    /* Determine the sender and receiver */
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );
    source = 0;
    dest   = size - 1;
	
    recvBuf = (int *) malloc( totsize * sizeof(int) );
    srcBuf  = (int *) malloc( (totsize + 1) * sizeof(int) ) ;
    
    if (!recvBuf || !srcBuf) {
	fprintf( stderr, "Could not allocate buffers\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    
    /* Initialize the send and recv buffers */
    for (i=0; i<totsize; i++) {
	recvBuf[i] = -1;
    }
    for (i=0; i<blksize; i++) {
	srcBuf[i] = i;
	srcBuf[blksize+1+i] = blksize+i;
    }
    srcBuf[blksize] = -1;

    MPI_Type_extent( MPI_INT, &extent );
    MPI_Win_create( recvBuf, totsize * extent, extent, 
		    MPI_INFO_NULL, comm, &win );
    MPI_Win_fence( 0, win );
    if (rank == source) {
	/* To improve reporting of problems about operations, we
	   change the error handler to errors return */
	MPI_Win_set_errhandler( win, MPI_ERRORS_RETURN );

	err = MPI_Put( srcBuf, 1, originType, dest, 0, 
		       totsize, MPI_INT, win );
	errs += CheckMPIErr( err );
	err = MPI_Win_fence( 0, win );
	errs += CheckMPIErr( err );
    }
    else if (rank == dest) {
	MPI_Win_fence( 0, win );
	for (i=0; i<totsize; i++) {
	    if (recvBuf[i] != i) {
		errs++;
		if (errs < 10) {
		    printf( "recvBuf[%d] = %d should = %d\n", 
			    i, recvBuf[i], i );
		}
	    }
	}
    }
    else {
	MPI_Win_fence( 0, win );
    }
    
    MPI_Type_free( &originType );
    MPI_Win_free( &win );
    free( recvBuf );
    free( srcBuf );

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}

int CheckMPIErr( int err )
{
    int rc = 0;
    if (err != MPI_SUCCESS) { 
	MTestPrintError( err );
	rc = 1;
    }
    return rc;
}
