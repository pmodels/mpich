/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test that communicators have reference count semantics";
*/

#define NELM 128
#define NCOMM 1020

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, source, dest, i;
    MPI_Comm      comm;
    MPI_Comm      tmpComm[NCOMM];
    MPI_Status    status;
    MPI_Request   req;
    int           *buf=0;

    MTest_Init( &argc, &argv );

    MPI_Comm_dup( MPI_COMM_WORLD, &comm );

    /* This is similar to the datatype test, except that we post
       an irecv on a simple data buffer but use a rank-reordered communicator.
       In this case, an error in handling the reference count will most 
       likely cause the program to hang, so this should be run only
       if (a) you are confident that the code is correct or (b) 
       a timeout is set for mpiexec 
    */

    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    if (size < 2) {
	fprintf( stderr, "This test requires at least two processes." );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    source  = 0;
    dest    = size - 1;

    if (rank == dest) {
	buf = (int *)malloc( NELM * sizeof(int) );
	for (i=0; i<NELM; i++) buf[i] = -i;
	MPI_Irecv( buf, NELM, MPI_INT, source, 0, comm, &req );
	MPI_Comm_free( &comm );

	if (comm != MPI_COMM_NULL) {
	    errs++;
	    printf( "Freed comm was not set to COMM_NULL\n" );
	}

	for (i=0; i<NCOMM; i++) {
	    MPI_Comm_split( MPI_COMM_WORLD, 0, size - rank, &tmpComm[i] );
	}

	MPI_Sendrecv( 0, 0, MPI_INT, source, 1, 
		      0, 0, MPI_INT, source, 1, MPI_COMM_WORLD, &status );

	MPI_Wait( &req, &status );
	for (i=0; i<NELM; i++) {
	    if (buf[i] != i) {
		errs++;
		if (errs < 10) {
		    printf( "buf[%d] = %d, expected %d\n", i, buf[i], i );
		}
	    }
	}
	for (i=0; i<NCOMM; i++) {
	    MPI_Comm_free( &tmpComm[i] );
	}
	free( buf );
    }
    else if (rank == source) {
	buf = (int *)malloc( NELM * sizeof(int) );
	for (i=0; i<NELM; i++) buf[i] = i;

	for (i=0; i<NCOMM; i++) {
	    MPI_Comm_split( MPI_COMM_WORLD, 0, size - rank, &tmpComm[i] );
	}
	/* Synchronize with the receiver */
	MPI_Sendrecv( 0, 0, MPI_INT, dest, 1, 
		      0, 0, MPI_INT, dest, 1, MPI_COMM_WORLD, &status );
	MPI_Send( buf, NELM, MPI_INT, dest, 0, comm );
	free( buf );
    }
    else {
	for (i=0; i<NCOMM; i++) {
	    MPI_Comm_split( MPI_COMM_WORLD, 0, size - rank, &tmpComm[i] );
	}
    }

    MPI_Barrier( MPI_COMM_WORLD );

    if (rank != dest) {
	/* Clean up the communicators */
	for (i=0; i<NCOMM; i++) {
	    MPI_Comm_free( &tmpComm[i] );
	}
    }
    if (comm != MPI_COMM_NULL) {
	MPI_Comm_free( &comm );
    }
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
