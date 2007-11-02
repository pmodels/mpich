/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"

/*
 * Test handling of truncated messages, including short and rendezvous
 */

static int testShort = 1;
static int ShortLen  = 2;
static int testMid   = 1;
static int MidLen    = 64;
static int testLong  = 1;
static int LongLen   = 100000;

int checkTruncError( int err, const char *msg );
int checkOk( int err, const char *msg );

int main( int argc, char *argv[] )
{
    MPI_Status status;
    int        err, errs = 0, source, dest, rank, size;
    int        *buf = 0;

    MTest_Init( &argc, &argv );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    source = 0;
    dest   = size - 1;

    buf = (int *)malloc( LongLen * sizeof(int) );
    if (!buf) {
	fprintf( stderr, "Unable to allocate communication buffer of size %d\n",
		 LongLen );
	MPI_Abort( MPI_COMM_WORLD, 1 ) ;
    }

    if (testShort) {
        if (rank == source) {
	    err = MPI_Send( buf, ShortLen, MPI_INT, dest, 0, MPI_COMM_WORLD );
	    errs += checkOk( err, "short" );
	}
	else if (rank == dest) {
	    err = MPI_Recv( buf, ShortLen-1, MPI_INT, source, 0, 
			    MPI_COMM_WORLD, &status );
	    errs += checkTruncError( err, "short" );
	}
	/* Try a message that is arrives after the receive is posted */
	if (rank == source) {
	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, dest, 1, 
			  MPI_BOTTOM, 0, MPI_INT, dest, 1, MPI_COMM_WORLD, 
			  MPI_STATUS_IGNORE );
	    MPI_Send( buf, ShortLen, MPI_INT, dest, 2, MPI_COMM_WORLD );
	}
	else if (rank == dest) {
	    MPI_Request req;
	    err = MPI_Irecv( buf, ShortLen - 1, MPI_INT, source, 2, 
			     MPI_COMM_WORLD, &req );
	    errs += checkOk( err, "irecv-short" );
	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, source, 1, 
			  MPI_BOTTOM, 0, MPI_INT, source, 1, MPI_COMM_WORLD, 
			  MPI_STATUS_IGNORE );
	    err = MPI_Wait( &req, &status );
	    errs += checkTruncError( err, "irecv-short" );
	}
    }
    if (testMid) {
        if (rank == source) {
	    err = MPI_Send( buf, MidLen, MPI_INT, dest, 0, MPI_COMM_WORLD );
	    errs += checkOk( err, "medium" );
	}
	else if (rank == dest) {
	    err = MPI_Recv( buf, MidLen-1, MPI_INT, source, 0, 
			    MPI_COMM_WORLD, &status );
	    errs += checkTruncError( err, "medium" );
	}
    }
    if (testLong) {
        if (rank == source) {
	    err = MPI_Send( buf, LongLen, MPI_INT, dest, 0, MPI_COMM_WORLD );
	    errs += checkOk( err, "long" );
	}
	else if (rank == dest) {
	    err = MPI_Recv( buf, LongLen-1, MPI_INT, source, 0, 
			    MPI_COMM_WORLD, &status );
	    errs += checkTruncError( err, "long" );
	}
    }

    free( buf );
    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}


int checkTruncError( int err, const char *msg )
{
    char errMsg[MPI_MAX_ERROR_STRING];
    int errs = 0, msgLen, errclass;

    if (!err) {
	errs ++;
	fprintf( stderr, 
	 "MPI_Recv (%s) returned MPI_SUCCESS instead of truncated message\n", 
		 msg );
	fflush( stderr );
    }
    else {
	MPI_Error_class( err, &errclass );
	if (errclass != MPI_ERR_TRUNCATE) {
	    errs ++;
	    MPI_Error_string( err, errMsg, &msgLen );
	    fprintf( stderr, 
		     "MPI_Recv (%s) returned unexpected error message: %s\n", 
		     msg, errMsg );
	    fflush( stderr );
	}
    }
    return errs;
}

int checkOk( int err, const char *msg )
{
    char errMsg[MPI_MAX_ERROR_STRING];
    int errs = 0, msgLen;
    if (err) {
	errs ++;
	MPI_Error_string( err, errMsg, &msgLen );
	fprintf( stderr, "MPI_Send(%s) failed with %s\n", msg, errMsg );
	fflush( stderr );
    }

    return errs;
}
