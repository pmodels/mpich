/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* 
 * Test for detecting that a Keyval created for one MPI Object cannot be used
 * to set an attribute on a different type of object 
 */

int main( int argc, char *argv[] )
{
    int          comm_keyval, win_keyval, type_keyval;
    int          comm_aval;
    int          err, errs = 0;
    int          buf, flag;
    MPI_Win      win;
    void         *rval;
    MPI_Datatype dtype;

    MTest_Init( &argc, &argv );

    MPI_Comm_create_keyval( MPI_COMM_NULL_COPY_FN,
			    MPI_COMM_NULL_DELETE_FN,
			    &comm_keyval, 0 );
    MPI_Win_create_keyval( MPI_WIN_NULL_COPY_FN,
			   MPI_WIN_NULL_DELETE_FN,
			   &win_keyval, 0 );
    MPI_Type_create_keyval( MPI_TYPE_NULL_COPY_FN, 
			    MPI_TYPE_NULL_DELETE_FN, 
			    &type_keyval, 0 );
    MPI_Type_contiguous( 4, MPI_DOUBLE, &dtype );
    MPI_Win_create( &buf, sizeof(int), sizeof(int), MPI_INFO_NULL, 
		    MPI_COMM_WORLD, &win );

    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    err = MPI_Comm_set_attr( MPI_COMM_WORLD, win_keyval, &comm_aval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Comm_set_attr accepted win keyval\n" );
    }
    err = MPI_Comm_set_attr( MPI_COMM_WORLD, type_keyval, &comm_aval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Comm_set_attr accepted type keyval\n" );
    }
    err = MPI_Type_set_attr( dtype, win_keyval, &comm_aval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Type_set_attr accepted win keyval\n" );
    }
    err = MPI_Type_set_attr( dtype, comm_keyval, &comm_aval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Comm_set_attr accepted type keyval\n" );
    }
    err = MPI_Win_set_attr( win, comm_keyval, &comm_aval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Win_set_attr accepted comm keyval\n" );
    }
    err = MPI_Win_set_attr( win, type_keyval, &comm_aval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Win_set_attr accepted type keyval\n" );
    }

    err = MPI_Comm_get_attr( MPI_COMM_WORLD, win_keyval, &rval, &flag );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Comm_get_attr accepted win keyval\n" );
    }
    err = MPI_Comm_get_attr( MPI_COMM_WORLD, type_keyval, &rval, &flag );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Comm_get_attr accepted type keyval\n" );
    }

    err = MPI_Comm_free_keyval( &win_keyval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Comm_free_keyval accepted win keyval\n" );
    }
    err = MPI_Comm_free_keyval( &type_keyval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Comm_free_keyval accepted type keyval\n" );
    }
    if (win_keyval != MPI_KEYVAL_INVALID) {
	err = MPI_Type_free_keyval( &win_keyval );
	if (err == MPI_SUCCESS) {
	    errs++;
	    fprintf( stderr, "Type_free_keyval accepted win keyval\n" );
	}
    }
    err = MPI_Type_free_keyval( &comm_keyval );
    if (err == MPI_SUCCESS) {
	errs++;
	fprintf( stderr, "Type_free_keyval accepted comm keyval\n" );
    }
    if (type_keyval != MPI_KEYVAL_INVALID) {
	err = MPI_Win_free_keyval( &type_keyval );
	if (err == MPI_SUCCESS) {
	    errs++;
	    fprintf( stderr, "Win_free_keyval accepted type keyval\n" );
	}
    }
    if (comm_keyval != MPI_KEYVAL_INVALID) {
	err = MPI_Win_free_keyval( &comm_keyval );
	if (err == MPI_SUCCESS) {
	    errs++;
	    fprintf( stderr, "Win_free_keyval accepted comm keyval\n" );
	}
    }

    /* Now, free for real */
    if (comm_keyval != MPI_KEYVAL_INVALID) {
	err = MPI_Comm_free_keyval( &comm_keyval );
	if (err != MPI_SUCCESS) {
	    errs++;
	    fprintf( stderr, "Could not free comm keyval\n" );
	}
    }
    if (type_keyval != MPI_KEYVAL_INVALID) {
	err = MPI_Type_free_keyval( &type_keyval );
	if (err != MPI_SUCCESS) {
	    errs++;
	    fprintf( stderr, "Could not free type keyval\n" );
	}
    }
    if (win_keyval != MPI_KEYVAL_INVALID) {
	err = MPI_Win_free_keyval( &win_keyval );
	if (err != MPI_SUCCESS) {
	    errs++;
	    fprintf( stderr, "Could not free win keyval\n" );
	}
    }

    MPI_Win_free( &win );
    MPI_Type_free( &dtype );

  
    MTest_Finalize( errs );
    MPI_Finalize();

    return 0;
}
