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
    int dims[2];
    int periods[2];
    int size, rank, newrank;

    MTest_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    
    /* This defines a one dimensional cartision grid with a single point */
    periods[0] = 1;
    dims[0] = 1;

    MPI_Cart_map( MPI_COMM_WORLD, 1, dims, periods, &newrank );
    if (rank > 0) {
	if (newrank != MPI_UNDEFINED) {
	    errs++;
	    printf( "rank outside of input communicator not UNDEFINED\n" );
	}
    }
    else {
	if (rank != newrank) {
	    errs++;
	    printf( "Newrank not defined and should be 0\n" );
	}
    }


    /* As of MPI 2.1, a 0-dimensional topology is valid (its also a
       point) */
    MPI_Cart_map( MPI_COMM_WORLD, 0, dims, periods, &newrank );
    if (rank > 0) {
	if (newrank != MPI_UNDEFINED) {
	    errs++;
	    printf( "rank outside of input communicator not UNDEFINED\n" );
	}
    }
    else {
	/* rank == 0 */
	if (rank != newrank) {
	    errs++;
	    printf( "Newrank not defined and should be 0\n" );
	}
    }


    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
