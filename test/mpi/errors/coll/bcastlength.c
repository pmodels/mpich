/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"

/* Very simple test that Bcast handled mismatched lengths (while not a 
   common user error, we've seen it several times, so good handling of 
   this helps to reduce bug reports) 
*/

int verbose = 0;

int main( int argc, char *argv[] )
{
    int buf[10], ierr, errs=0, toterrs;
    int rank;
    char      str[MPI_MAX_ERROR_STRING+1];
    int       slen;

    MPI_Init( &argc, &argv );
    
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  
    if (rank == 0) {
	ierr = MPI_Bcast( buf, 1, MPI_INT, 0, MPI_COMM_WORLD );
    }
    else {
	ierr = MPI_Bcast( buf, 10, MPI_INT, 0, MPI_COMM_WORLD );
    }
    if (ierr == MPI_SUCCESS) {
	if (rank != 0) {
	    /* The root process may not detect that a too-long buffer
	       was provided by the non-root processes, but those processes
	       should detect this. */
	    errs ++;
	    printf( "Did not detect mismatched length (long) on process %d\n", 
		    rank );
	}
    }
    else {
	if (verbose) {
	    MPI_Error_string( ierr, str, &slen );
	    printf( "Found expected error; message is: %s\n", str );
	}
    }

    if (rank == 0) {
	ierr = MPI_Bcast( buf, 10, MPI_INT, 0, MPI_COMM_WORLD );
    }
    else {
	ierr = MPI_Bcast( buf, 1, MPI_INT, 0, MPI_COMM_WORLD );
    }
    if (ierr == MPI_SUCCESS) {
	if (rank != 0) {
	    /* The root process may not detect that a too-short buffer
	       was provided by the non-root processes, but those processes
	       should detect this. */
	    errs ++;
	    printf( "Did not detect mismatched length (short) on process %d\n",
		    rank );
	}
    }
    else {
	if (verbose) {
	    MPI_Error_string( ierr, str, &slen );
	    printf( "Found expected error; message is: %s\n", str );
	}
    }

    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL );

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
