/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test bsend message handling where \
different messages are received in different orders";
*/

/*
 * Notes on the test.
 *
 * To ensure that messages remain in the bsend buffer until received,
 * messages are sent with size MSG_SIZE (ints).  
 */

#define MSG_SIZE 17000

int main( int argc, char *argv[] )
{
    int errs = 0;
    int b1[MSG_SIZE], b2[MSG_SIZE], b3[MSG_SIZE], b4[MSG_SIZE];
    int src, dest, size, rank, i;
    MPI_Comm comm;
    MPI_Status status;

    MTest_Init( &argc, &argv );

    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    if (size < 2) {
	errs++;
	fprintf( stderr, "At least 2 processes required\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    src  = 0;
    dest = 1;

    if (rank == src) {
	int *buf, bufsize, bsize;

	bufsize = 4 * (MSG_SIZE * sizeof(int) + MPI_BSEND_OVERHEAD);
	buf = (int *)malloc( bufsize );
	if (!buf) {
	    fprintf( stderr, "Could not allocate buffer of %d bytes\n", 
		     bufsize );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
	}
	MPI_Buffer_attach( buf, bufsize );

	/* Initialize data */
	for (i=0; i<MSG_SIZE; i++) {
	    b1[i] = i;
	    b2[i] = MSG_SIZE + i;
	    b3[i] = 2 * MSG_SIZE + i;
	    b4[i] = 3 * MSG_SIZE + i;
	}
	/* Send and reset buffers after bsend returns */
	MPI_Bsend( b1, MSG_SIZE, MPI_INT, dest, 0, comm );
	for (i=0; i<MSG_SIZE; i++) b1[i] = -b1[i];
	MPI_Bsend( b2, MSG_SIZE, MPI_INT, dest, 1, comm );
	for (i=0; i<MSG_SIZE; i++) b2[i] = -b2[i];
	MPI_Bsend( b3, MSG_SIZE, MPI_INT, dest, 2, comm );
	for (i=0; i<MSG_SIZE; i++) b3[i] = -b3[i];
	MPI_Bsend( b4, MSG_SIZE, MPI_INT, dest, 3, comm );
	for (i=0; i<MSG_SIZE; i++) b4[i] = -b4[i];

	MPI_Barrier( comm );
	/* Detach waits until all messages received */
	MPI_Buffer_detach( &buf, &bsize );
    }
    else if (rank == dest) {
	
	MPI_Barrier( comm );
	MPI_Recv( b2, MSG_SIZE, MPI_INT, src, 1, comm, &status );
	MPI_Recv( b1, MSG_SIZE, MPI_INT, src, 0, comm, &status );
	MPI_Recv( b4, MSG_SIZE, MPI_INT, src, 3, comm, &status );
	MPI_Recv( b3, MSG_SIZE, MPI_INT, src, 2, comm, &status );

	/* Check received data */
	for (i=0; i<MSG_SIZE; i++) {
	    if (b1[i] != i) {
		errs++;
		if (errs < 16) printf( "b1[%d] is %d\n", i, b1[i] );
	    }
	    if (b2[i] != MSG_SIZE + i) {
		errs++;
		if (errs < 16) printf( "b2[%d] is %d\n", i, b2[i] );
	    }
	    if (b3[i] != 2 * MSG_SIZE + i) {
		errs++;
		if (errs < 16) printf( "b3[%d] is %d\n", i, b3[i] );
	    }
	    if (b4[i] != 3 * MSG_SIZE + i) {
		errs++;
		if (errs < 16) printf( "b4[%d] is %d\n", i, b4[i] );
	    }
	}
    }
    else {
	MPI_Barrier( comm );
    }


    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
