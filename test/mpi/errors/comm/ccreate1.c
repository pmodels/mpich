/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

/* Check that Comm_create detects the case where the group is not a subset of 
   the group of the input communicator */

void abortMsg( const char *, int );

void abortMsg( const char *str, int code )
{
    char msg[MPI_MAX_ERROR_STRING];
    int class, resultLen;

    MPI_Error_class( code, &class );
    MPI_Error_string( code, msg, &resultLen );
    fprintf( stderr, "%s: errcode = %d, class = %d, msg = %s\n", 
	     str, code, class, msg );
    MPI_Abort( MPI_COMM_WORLD, code );
}

int main( int argc, char *argv[] )
{
    MPI_Comm  evencomm, lowcomm, newcomm;
    int       wrank, wsize, gsize, err, errs = 0;
    int       ranges[1][3], mygrank;
    MPI_Group wGroup, godd, ghigh, geven;

    MTest_Init( &argc, &argv );

    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );

    /* Create some communicators */
    MPI_Comm_split( MPI_COMM_WORLD, wrank % 2, wrank, &evencomm );
    MPI_Comm_split( MPI_COMM_WORLD, wrank < wsize/2, wsize-wrank, &lowcomm );
    MPI_Comm_set_errhandler( evencomm, MPI_ERRORS_RETURN );
    MPI_Comm_set_errhandler( lowcomm, MPI_ERRORS_RETURN );

    /* Create some groups */
    MPI_Comm_group( MPI_COMM_WORLD, &wGroup );

    ranges[0][0] = 2*(wsize/2)-1;
    ranges[0][1] = 1;
    ranges[0][2] = -2;
    err = MPI_Group_range_incl( wGroup, 1, ranges, &godd );
    if (err) abortMsg( "Failed to create odd group: ", err );
    err = MPI_Group_size( godd, &gsize );
    if (err) abortMsg( "Failed to get size of odd group: ", err );
    if (gsize != wsize/2) {
	fprintf( stderr, "Group godd size is %d should be %d\n", gsize, 
		 wsize/2 );
	errs++;
    }
	
    ranges[0][0] = wsize/4;
    ranges[0][1] = wsize-1;
    ranges[0][2] = 1;
    err = MPI_Group_range_incl( wGroup, 1, ranges, &ghigh );
    if (err) abortMsg( "Failed to create high group\n", err );
    ranges[0][0] = 0;
    ranges[0][1] = wsize-1;
    ranges[0][2] = 2;
    err = MPI_Group_range_incl( wGroup, 1, ranges, &geven );
    if (err) abortMsg( "Failed to create even group:", err );

    /* Check that a correct case returns success */
    if ( (wrank % 2) == 0) {
	err = MPI_Group_rank( geven, &mygrank );
	if (err) {
	    errs++;
	    fprintf( stderr, "Could not get rank from geven group\n" );
	}
	else if (mygrank == MPI_UNDEFINED) {
	    errs++;
	    fprintf( stderr, "mygrank should be %d but is undefined\n", 
		     wrank/2);
	}
	err = MPI_Comm_create( evencomm, geven, &newcomm );
	/* printf( "Created new even comm\n" ); */
	if (err != MPI_SUCCESS) {
	    errs++;
	    fprintf( stderr, "Failed to allow creation from even group\n" );
	}
	else {
	    MPI_Comm_free( &newcomm );
	}
    }

    /* Now, test that these return errors when we try to use them to create 
       groups */
    if ( (wrank % 2) == 0) {
	/* printf( "Even comm...\n" ); */
	/* evencomm is the comm of even-ranked processed in comm world */
	err = MPI_Comm_create( evencomm, godd, &newcomm );
	MPI_Group_rank( godd, &mygrank );
	if (err == MPI_SUCCESS) {
	    if (mygrank != MPI_UNDEFINED) {
		errs++;
		fprintf( stderr, "Did not detect group of odd ranks in even comm\n" );
	    }
	    MPI_Comm_free( &newcomm );
	}
    }
    if (wrank < wsize/2) {
	/* printf( "low comm...\n" ); */
	err = MPI_Comm_create( lowcomm, ghigh, &newcomm );
	MPI_Group_rank( ghigh, &mygrank );
	if (err == MPI_SUCCESS) {
	    if (mygrank != MPI_UNDEFINED) {
		errs++;
		fprintf( stderr, "Did not detect group of high ranks in low comm\n" );
	    }
	    MPI_Comm_free( &newcomm );
	}
    }
    
    MPI_Comm_free( &lowcomm );
    MPI_Comm_free( &evencomm );
    MPI_Group_free( &ghigh );
    MPI_Group_free( &godd );
    MPI_Group_free( &geven );
    MPI_Group_free( &wGroup );

    MTest_Finalize( errs );

    MPI_Finalize();
    return 0;
}
