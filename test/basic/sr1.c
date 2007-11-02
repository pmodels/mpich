/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * A simple code that can be used to test the basic message passing or
 * debug parts of the code.  This is less a test than a way to exercise
 * the primary code path for short message send-receives
 */
#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int myrank, size, src=0, dest=1, msgsize=1;
    int buf[20];

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &myrank );

    if (myrank == src) {
        buf[0] = 0xabcd0123;
	MPI_Send( buf, msgsize, MPI_INT, dest, 0, MPI_COMM_WORLD );
    }
    else if (myrank == dest) {
        buf[0] = 0xffffffff;
	MPI_Recv( buf, msgsize, MPI_INT, src, 0, MPI_COMM_WORLD, 
		  MPI_STATUS_IGNORE );
	if (buf[0] != 0xabcd0123) {
	    printf( "Expected %x but got %x\n", 0xabcd0123, buf[0] );
	    fflush(stdout);
	}
    }

    MPI_Finalize();

    return 0;
}
