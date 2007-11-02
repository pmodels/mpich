/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: ic1.c,v 1.2 2002/10/29 18:04:17 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * A simple test of the intercomm create routine, with a communication test
 */
#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    MPI_Comm intercomm;
    int      remote_rank, rank, size, errs = 0, toterr;
    volatile int trigger;

    MPI_Init( &argc, &argv );

    trigger = 1;
/*    while (trigger) ; */

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if (size < 2) {
	printf( "Size must be at least 2\n" );
	MPI_Abort( MPI_COMM_WORLD, 0 );
    }

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    /* Make an intercomm of the first two elements of comm_world */
    if (rank < 2) {
	int lrank = rank, rrank = -1;
	MPI_Status status;

	remote_rank = 1 - rank;
	MPI_Intercomm_create( MPI_COMM_SELF, 0,
			      MPI_COMM_WORLD, remote_rank, 27, 
			      &intercomm );

	/* Now, communicate between them */
	MPI_Sendrecv( &lrank, 1, MPI_INT, 0, 13, 
		      &rrank, 1, MPI_INT, 0, 13, intercomm, &status );

	if (rrank != remote_rank) {
	    errs++;
	    printf( "%d Expected %d but received %d\n", 
		    rank, remote_rank, rrank );
	}

	MPI_Comm_free( &intercomm );
    }
    
    /* The next test should create an intercomm with groups of different
       sizes FIXME */

    MPI_Allreduce( &errs, &toterr, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (rank == 0) {
	if (toterr == 0) {
	    printf( " No Errors\n" );
	}
	else {
	    printf (" Found %d errors\n", toterr );
	}
    }
    
    MPI_Finalize();
    
    return 0;
}
