/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* Test Ibsend and Request_free */
int main( int argc, char *argv[] )
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int dest = 1, src = 0, tag = 1;
    int s1;
    char *buf, *bbuf;
    int smsg[5], rmsg[5];
    int errs = 0, rank, size;
    int bufsize, bsize;

    MTest_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if (src >= size || dest >= size) {
	int r = src;
	if (dest > r) r = dest;
	fprintf( stderr, "This program requires %d processes\n", r-1 );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    if (rank == src) {
	MPI_Request r;

	MPI_Barrier( MPI_COMM_WORLD );

	/* According to the standard, we must use the PACK_SIZE length of each
	   message in the computation of the message buffer size */
	MPI_Pack_size( 5, MPI_INT, comm, &s1 );
	bufsize = MPI_BSEND_OVERHEAD + s1 + 2000;
	buf = (char *)malloc( bufsize );
	MPI_Buffer_attach( buf, bufsize );

	MTestPrintfMsg( 10, "About create and free Isend request\n" );
	smsg[0] = 10;
	MPI_Isend( &smsg[0], 1, MPI_INT, dest, tag, comm, &r );
	MPI_Request_free( &r );
	if (r != MPI_REQUEST_NULL) {
	    errs++;
	    fprintf( stderr, "Request not set to NULL after request free\n" );
	}
	MTestPrintfMsg( 10, "About create and free Ibsend request\n" );
	smsg[1] = 11;
	MPI_Ibsend( &smsg[1], 1, MPI_INT, dest, tag+1, comm, &r );
	MPI_Request_free( &r );
	if (r != MPI_REQUEST_NULL) {
	    errs++;
	    fprintf( stderr, "Request not set to NULL after request free\n" );
	}
	MTestPrintfMsg( 10, "About create and free Issend request\n" );
	smsg[2] = 12;
	MPI_Issend( &smsg[2], 1, MPI_INT, dest, tag+2, comm, &r );
	MPI_Request_free( &r );
	if (r != MPI_REQUEST_NULL) {
	    errs++;
	    fprintf( stderr, "Request not set to NULL after request free\n" );
	}
	MTestPrintfMsg( 10, "About create and free Irsend request\n" );
	smsg[3] = 13;
	MPI_Irsend( &smsg[3], 1, MPI_INT, dest, tag+3, comm, &r );
	MPI_Request_free( &r );
	if (r != MPI_REQUEST_NULL) {
	    errs++;
	    fprintf( stderr, "Request not set to NULL after request free\n" );
	}
	smsg[4] = 14;
	MPI_Isend( &smsg[4], 1, MPI_INT, dest, tag+4, comm, &r );
	MPI_Wait( &r, MPI_STATUS_IGNORE );

	/* We can't guarantee that messages arrive until the detach */
 	MPI_Buffer_detach( &bbuf, &bsize ); 
    }

    if (rank == dest) {
	MPI_Request r[5];
	int i;

	for (i=0; i<5; i++) {
	    MPI_Irecv( &rmsg[i], 1, MPI_INT, src, tag+i, comm, &r[i] );
	}
	if (rank != src) /* Just in case rank == src */
	    MPI_Barrier( MPI_COMM_WORLD );

	for (i=0; i<4; i++) {
	    MPI_Wait( &r[i], MPI_STATUS_IGNORE );
	    if (rmsg[i] != 10+i) {
		errs++;
		fprintf( stderr, "message %d (%d) should be %d\n", i, rmsg[i], 10+i );
	    }
	}
	/* The MPI standard says that there is no way to use MPI_Request_free
	   safely with receive requests.  A strict MPI implementation may
	   choose to consider these erroreous (an IBM MPI implementation
	   does so)  */
#ifdef USE_STRICT_MPI
	MPI_Wait( &r[4], MPI_STATUS_IGNORE );
#else
	MTestPrintfMsg( 10, "About  free Irecv request\n" );
	MPI_Request_free( &r[4] ); 
#endif
    }

    if (rank != dest && rank != src) {
	MPI_Barrier( MPI_COMM_WORLD );
    }


    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}
