/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"

/* Very simple test that Allreduce detects invalid (datatype,operation)
   pair */

int verbose = 0;

int main( int argc, char *argv[] )
{
    int a, b, ierr, errs=0, toterrs;
    int rank;
    char      str[MPI_MAX_ERROR_STRING+1];
    int       slen;

    MPI_Init( &argc, &argv );
    
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    
    ierr = MPI_Reduce( &a, &b, 1, MPI_BYTE, MPI_MAX, 0, MPI_COMM_WORLD );
    if (ierr == MPI_SUCCESS) {
	errs ++;
	printf( "Did not detect invalid type/op pair (byte,max) in Allreduce\n" );
    }
    else {
	if (verbose) {
	    MPI_Error_string( ierr, str, &slen );
	    printf( "Found expected error; message is: %s\n", str );
	}
    }

    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Allreduce( &errs, &toterrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (rank == 0) {
	if (toterrs == 0) {
	    printf( " No Errors\n" );
	}
	else {
	    printf( " Found %d errors\n", toterrs );
	}
    }
    MPI_Finalize( );
    return 0;
}
