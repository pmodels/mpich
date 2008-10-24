/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int myrank, size, src=0, dest=1;
    int buf[20];

    MPI_Init( 0, 0 );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &myrank );

    if (myrank == src || myrank == dest) {
	int partner = src;
	if (myrank == partner) partner = dest;
        MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, partner, 0, 
  	    	      MPI_BOTTOM, 0, MPI_INT, partner,  0, MPI_COMM_WORLD, 
		      MPI_STATUS_IGNORE );
    }
    
    if (myrank == src) {
	MPI_Send( buf, 20, MPI_INT, dest, 0, MPI_COMM_WORLD );
    }
    else if (myrank == dest) {
	MPI_Recv( buf, 20, MPI_INT, src, 0, MPI_COMM_WORLD, 
		  MPI_STATUS_IGNORE );
    }

    MPI_Finalize();

    return 0;
}
