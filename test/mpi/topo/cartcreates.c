/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size, rank;
    int dims[2], periods[2];
    MPI_Comm comm;

    MTest_Init( &argc, &argv );

    /* Create a new cartesian communicator in a subset of the processes */
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if (size < 2) {
	fprintf( stderr, "This test needs at least 2 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    dims[0]    = size-1;
    periods[0] = 1;
    MPI_Cart_create( MPI_COMM_WORLD, 1, dims, periods, 0, &comm );

    if (comm != MPI_COMM_NULL) {
	int csize;
	MPI_Comm_size( comm, &csize );
	if (csize != dims[0]) {
	    errs++;
	    fprintf( stderr, 
	     "Sizes is wrong in cart communicator.  Is %d, should be %d\n", 
	     csize, dims[0] ); 
	}
	MPI_Barrier( comm );

	MPI_Comm_free( &comm );
    } 
    else if (rank < dims[0]) {
	errs++;
	fprintf( stderr, "Communicator returned is null!" );
    }

    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}
