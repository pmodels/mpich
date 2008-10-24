/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*  
    Check that the MPI implementation properly handles zero-dimensional
    Cartesian communicators - the original standard implies that these
    should be consistent with higher dimensional topologies and thus 
    these should work with any MPI implementation.  MPI 2.1 made this
    requirement explicit.
*/
int main( int argc, char *argv[] )
{
    int errs = 0;
    int size, rank, ndims;
    MPI_Comm comm, newcomm;

    MTest_Init( &argc, &argv );

    /* Create a new cartesian communicator in a subset of the processes */
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if (size < 2) {
	fprintf( stderr, "This test needs at least 2 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    MPI_Cart_create( MPI_COMM_WORLD, 0, NULL, NULL, 0, &comm );

    if (comm != MPI_COMM_NULL) {
	int csize;
	MPI_Comm_size( comm, &csize );
	if (csize != 1) {
	    errs++;
	    fprintf( stderr, 
	     "Sizes is wrong in cart communicator.  Is %d, should be 1\n", 
	     csize ); 
	}

	/* This function is not meaningful, but should not fail */
	MPI_Dims_create(1, 0, NULL);

	ndims = -1;
	MPI_Cartdim_get(comm, &ndims);
	if (ndims != 0) {
	    errs++;
	    fprintf(stderr, "MPI_Cartdim_get: ndims is %d, should be 0\n", ndims);
	}

	/* this function should not fail */
	MPI_Cart_get(comm, 0, NULL, NULL, NULL);

	MPI_Cart_rank(comm, NULL, &rank);
	if (rank != 0) {
	    errs++;
	    fprintf(stderr, "MPI_Cart_rank: rank is %d, should be 0\n", rank);
	}

	/* this function should not fail */
	MPI_Cart_coords(comm, 0, 0, NULL);

	MPI_Cart_sub(comm, NULL, &newcomm);
	ndims = -1;
	MPI_Cartdim_get(newcomm, &ndims);
	if (ndims != 0) {
	    errs++;
	    fprintf(stderr, 
	       "MPI_Cart_sub did not return zero-dimensional communicator\n");
	}

	MPI_Barrier( comm );

	MPI_Comm_free( &comm );
	MPI_Comm_free( &newcomm );
    } 
    else if (rank == 0) {
	errs++;
	fprintf( stderr, "Communicator returned is null!" );
    }

    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}
