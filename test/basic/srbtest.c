/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* A very simple program that performs only a send and a receive.  This
   program can be use with logging enabled (configure with 
   --enable-g=dbg,log) and run with -mpich-dbg-class=routine to get
   a trace of the execution of the functions in MPICH2 */
#include <stdio.h>
#include "mpi.h"

#define SENDER_RANK 0
#define RECEIVER_RANK 1
int main( int argc, char *argv[] )
{
    int rank, size, val;
    MPI_Status status;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if (size < 2) {
	fprintf( stderr, "This program requires at least 2 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    if (rank == SENDER_RANK) {
	MPI_Send( &rank, 1, MPI_INT, RECEIVER_RANK, 0, MPI_COMM_WORLD );
    }
    else if (rank == RECEIVER_RANK) {
	/* This may or may not post the receive before the send arrives */
	MPI_Recv( &val, 1, MPI_INT, SENDER_RANK, 0, MPI_COMM_WORLD, &status );
    }
    /* Perform a second send/receive to allow the first pair to handle
       any connection logic */
#if 1
    if (rank == SENDER_RANK) {
	MPI_Send( &rank, 1, MPI_INT, RECEIVER_RANK, 0, MPI_COMM_WORLD );
    }
    else if (rank == RECEIVER_RANK) {
	/* This may or may not post the receive before the send arrives */
	MPI_Recv( &val, 1, MPI_INT, SENDER_RANK, 0, MPI_COMM_WORLD, &status );
    }
#endif

    MPI_Finalize( );

    return 0;
}
