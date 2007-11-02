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
    int source, dest;
    int dims[2], periods[2];
    MPI_Comm comm;

    MTest_Init( &argc, &argv );
    
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    dims[0]    = size;
    periods[0] = 1;
    MPI_Cart_create( MPI_COMM_WORLD, 1, dims, periods, 0, &comm );
    MPI_Cart_shift( comm, 0, 1, &source, &dest );
    if (source != ((rank - 1 + size) % size)) {
	errs++;
	printf( "source for shift 1 is %d\n", source );
    }
    if (dest != ((rank + 1) % size)) {
	errs++;
	printf( "dest for shift 1 is %d\n", dest );
    }
    MPI_Cart_shift( comm, 0, 0, &source, &dest );
    if (source != rank) {
	errs++;
	printf( "Source for shift 0 is %d\n", source );
    }
    if (dest != rank) {
	errs++;
	printf( "Dest for shift 0 is %d\n", dest );
    }
    MPI_Cart_shift( comm, 0, -1, &source, &dest );
    if (source != ((rank + 1) % size)) {
	errs++;
	printf( "source for shift -1 is %d\n", source );
    }
    if (dest != ((rank - 1 + size) % size)) {
	errs++;
	printf( "dest for shift -1 is %d\n", dest );
    }

    /* Now, with non-periodic */
    MPI_Comm_free( &comm );
    periods[0] = 0;
    MPI_Cart_create( MPI_COMM_WORLD, 1, dims, periods, 0, &comm );
    MPI_Cart_shift( comm, 0, 1, &source, &dest );
    if ((rank > 0 && source != (rank - 1)) || 
	(rank == 0 && source != MPI_PROC_NULL)) {
	errs++;
	printf( "source for non-periodic shift 1 is %d\n", source );
    }
    if ((rank < size-1 && dest != rank + 1) || 
	((rank == size-1) && dest != MPI_PROC_NULL)) {
	errs++;
	printf( "dest for non-periodic shift 1 is %d\n", dest );
    }
    MPI_Cart_shift( comm, 0, 0, &source, &dest );
    if (source != rank) {
	errs++;
	printf( "Source for non-periodic shift 0 is %d\n", source );
    }
    if (dest != rank) {
	errs++;
	printf( "Dest for non-periodic shift 0 is %d\n", dest );
    }
    MPI_Cart_shift( comm, 0, -1, &source, &dest );
    if ((rank < size - 1 && source != rank + 1) ||
	(rank == size - 1 && source != MPI_PROC_NULL)) {
	
	errs++;
	printf( "source for non-periodic shift -1 is %d\n", source );
    }
    if ((rank > 0 && dest != rank - 1) ||
	(rank == 0 && dest != MPI_PROC_NULL)) {
	errs++;
	printf( "dest for non-periodic shift -1 is %d\n", dest );
    }
    MPI_Comm_free( &comm );
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
