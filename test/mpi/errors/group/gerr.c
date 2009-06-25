/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
 * Test whether invalid handles to group routines are detected 
 */

int verbose = 0;

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rc;
    int ranks[2];
    MPI_Group ng;
    char      str[MPI_MAX_ERROR_STRING+1];
    int       slen;

    MTest_Init( &argc, &argv );
    /* Set errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    /* Create some valid input data except for the group handle */
    ranks[0] = 0;
    rc = MPI_Group_incl( MPI_COMM_WORLD, 1, ranks, &ng );
    if (rc == MPI_SUCCESS) {
	errs ++;
	printf( "Did not detect invalid handle (comm) in group_incl\n" );
    }
    else {
	if (verbose) {
	    MPI_Error_string( rc, str, &slen );
	    printf( "Found expected error; message is: %s\n", str );
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize( );
    return 0;
}
