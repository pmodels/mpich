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
    int size, dims[2], periods[2], remain[2];
    int result, rank;
    MPI_Comm comm, newcomm;

    MTest_Init( &argc, &argv );

    /* First, create a 1-dim cartesian communicator */
    periods[0] = 0;
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    dims[0] = size;
    MPI_Cart_create( MPI_COMM_WORLD, 1, dims, periods, 0, &comm );
    
    /* Now, extract a communicator with no dimensions */
    remain[0] = 0;
    MPI_Cart_sub( comm, remain, &newcomm );

    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
	/* This should be congruent to MPI_COMM_SELF */
	MPI_Comm_compare( MPI_COMM_SELF, newcomm, &result );
	if (result != MPI_CONGRUENT) {
	    errs++;
	    printf( "cart sub to size 0 did not give self\n" );
	}
	MPI_Comm_free( &newcomm );
    }
    else if (newcomm != MPI_COMM_NULL) {
	errs++;
	printf( "cart sub to size 0 did not give null\n" );
    }

    /* Free the new communicator so that storage leak tests will
       be happy */
    MPI_Comm_free( &comm );
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
