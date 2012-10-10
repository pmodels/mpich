/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpi.h"

/* This is a program to enable testing and demonstration of the debugger
   interface, particularly in terms of showing message queues.  To use
   this, run with a few processes and attach with the debugger when the
   program stops.  You can change the variable "hold" to 0 to allow the
   program to complete. */
int main( int argc, char *argv[] )
{
    int wsize, wrank;
    int source, dest, i;
    int buf1[10], buf2[10], buf3[10];
    MPI_Request r[3];
    volatile int hold = 1; 
    MPI_Comm dupcomm;
    MPI_Status status;

    MPI_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    
    /* Set the source and dest in a ring */
    source = (wrank + 1) % wsize;
    dest   = (wrank + wsize - 1) % wsize;

    MPI_Comm_dup( MPI_COMM_WORLD, &dupcomm );
    MPI_Comm_set_name( dupcomm, "Dup of comm world" );

    for (i=0; i<3; i++) {
	MPI_Irecv( MPI_BOTTOM, 0, MPI_INT, source, i + 100, MPI_COMM_WORLD, 
		   &r[i] );
    }

    MPI_Send( buf2, 8, MPI_INT, dest, 1, MPI_COMM_WORLD );
    MPI_Send( buf3, 4, MPI_INT, dest, 2, dupcomm );
    
    while (hold) ;

    MPI_Recv( buf1, 10, MPI_INT, source, 1, MPI_COMM_WORLD, &status );
    MPI_Recv( buf1, 10, MPI_INT, source, 1, dupcomm, &status );

    for (i=0; i<3; i++) {
	MPI_Cancel( &r[i] );
    }

    MPI_Comm_free( &dupcomm );
    
    
    MPI_Finalize();
    
    return 0;
}
