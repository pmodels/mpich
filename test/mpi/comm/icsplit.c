/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
 * This program tests that MPI_Comm_split applies to intercommunicators;
 * this is an extension added in MPI-2
 */

int TestIntercomm( MPI_Comm );

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size, isLeft;
    MPI_Comm intercomm, newcomm;

    MTest_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if (size < 4) {
	printf( "This test requires at least 4 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    while (MTestGetIntercomm( &intercomm, &isLeft, 2 )) {
	int key, color;

        if (intercomm == MPI_COMM_NULL) continue;

	/* Split this intercomm.  The new intercomms contain the 
	   processes that had odd (resp even) rank in their local group
	   in the original intercomm */
	MTestPrintfMsg( 1, "Created intercomm %s\n", MTestGetIntercommName() );
	MPI_Comm_rank( intercomm, &key );
	color = (key % 2);
	MPI_Comm_split( intercomm, color, key, &newcomm );
	/* Make sure that the new communicator has the appropriate pieces */
	if (newcomm != MPI_COMM_NULL) {
	    int orig_rsize, orig_size, new_rsize, new_size;
	    int predicted_size, flag, commok=1;

	    MPI_Comm_test_inter( intercomm, &flag );
	    if (!flag) {
		errs++;
		printf( "Output communicator is not an intercomm\n" );
		commok = 0;
	    }

	    MPI_Comm_remote_size( intercomm, &orig_rsize );
	    MPI_Comm_remote_size( newcomm, &new_rsize );
	    MPI_Comm_size( intercomm, &orig_size );
	    MPI_Comm_size( newcomm, &new_size );
	    /* The local size is 1/2 the original size, +1 if the 
	       size was odd and the color was even.  More precisely,
	       let n be the orig_size.  Then
	                        color 0     color 1
	       orig size even    n/2         n/2
	       orig size odd     (n+1)/2     n/2

	       However, since these are integer valued, if n is even,
	       then (n+1)/2 = n/2, so this table is much simpler:
	                        color 0     color 1
	       orig size even    (n+1)/2     n/2
	       orig size odd     (n+1)/2     n/2
	       
	    */
	    predicted_size = (orig_size + !color) / 2; 
	    if (predicted_size != new_size) {
		errs++;
		printf( "Predicted size = %d but found %d for %s (%d,%d)\n",
			predicted_size, new_size, MTestGetIntercommName(),
			orig_size, orig_rsize );
		commok = 0;
	    }
	    predicted_size = (orig_rsize + !color) / 2;
	    if (predicted_size != new_rsize) {
		errs++;
		printf( "Predicted remote size = %d but found %d for %s (%d,%d)\n",
			predicted_size, new_rsize, MTestGetIntercommName(), 
			orig_size, orig_rsize );
		commok = 0;
	    }
	    /* ... more to do */
	    if (commok) {
		errs += TestIntercomm( newcomm );
	    }
	}
	else {
	    int orig_rsize;
	    /* If the newcomm is null, then this means that remote group
	       for this color is of size zero (since all processes in this 
	       test have been given colors other than MPI_UNDEFINED).
	       Confirm that here */
	    /* FIXME: ToDo */
	    MPI_Comm_remote_size( intercomm, &orig_rsize );
	    if (orig_rsize == 1) {
		if (color == 0) {
		    errs++;
		    printf( "Returned null intercomm when non-null expected\n" );
		}
	    }
	}
	if (newcomm != MPI_COMM_NULL) 
	    MPI_Comm_free( &newcomm );
	MPI_Comm_free( &intercomm );
    }
    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}

/* FIXME: This is copied from iccreate.  It should be in one place */
int TestIntercomm( MPI_Comm comm )
{
    int local_size, remote_size, rank, **bufs, *bufmem, rbuf[2], j;
    int errs = 0, wrank, nsize;
    char commname[MPI_MAX_OBJECT_NAME+1];
    MPI_Request *reqs;

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( comm, &local_size );
    MPI_Comm_remote_size( comm, &remote_size );
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_get_name( comm, commname, &nsize );

    MTestPrintfMsg( 1, "Testing communication on intercomm %s\n", commname );
    
    reqs = (MPI_Request *)malloc( remote_size * sizeof(MPI_Request) );
    if (!reqs) {
	printf( "[%d] Unable to allocated %d requests for testing intercomm %s\n", 
		wrank, remote_size, commname );
	errs++;
	return errs;
    }
    bufs = (int **) malloc( remote_size * sizeof(int *) );
    if (!bufs) {
	printf( "[%d] Unable to allocated %d int pointers for testing intercomm %s\n", 
		wrank, remote_size, commname );
	errs++;
	return errs;
    }
    bufmem = (int *) malloc( remote_size * 2 * sizeof(int) );
    if (!bufmem) {
	printf( "[%d] Unable to allocated %d int data for testing intercomm %s\n", 
		wrank, 2*remote_size, commname );
	errs++;
	return errs;
    }

    /* Each process sends a message containing its own rank and the
       rank of the destination with a nonblocking send.  Because we're using
       nonblocking sends, we need to use different buffers for each isend */
    for (j=0; j<remote_size; j++) {
	bufs[j]    = &bufmem[2*j];
	bufs[j][0] = rank;
	bufs[j][1] = j;
	MPI_Isend( bufs[j], 2, MPI_INT, j, 0, comm, &reqs[j] );
    }

    for (j=0; j<remote_size; j++) {
	MPI_Recv( rbuf, 2, MPI_INT, j, 0, comm, MPI_STATUS_IGNORE );
	if (rbuf[0] != j) {
	    printf( "[%d] Expected rank %d but saw %d in %s\n", 
		    wrank, j, rbuf[0], commname );
	    errs++;
	}
	if (rbuf[1] != rank) {
	    printf( "[%d] Expected target rank %d but saw %d from %d in %s\n", 
		    wrank, rank, rbuf[1], j, commname );
	    errs++;
	}
    }
    if (errs) 
	fflush(stdout);
    MPI_Waitall( remote_size, reqs, MPI_STATUSES_IGNORE );

    free( reqs );
    free( bufs );
    free( bufmem );

    return errs;
}
