/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* Set verbose to 1 to see the error message */
static int verbose = 0;

int main( int argc, char *argv[] )
{
    int ierr, errs=0;
    MPI_Comm newcomm = MPI_COMM_NULL;

    MTest_Init( &argc, &argv );

    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    ierr = MPI_Comm_connect( (char*)"myhost:27", MPI_INFO_NULL, 0, 
			     MPI_COMM_WORLD, &newcomm );
    if (ierr == MPI_SUCCESS) {
	errs++;
	printf( "Comm_connect returned success with bogus port\n" );
	MPI_Comm_free( &newcomm );
    }
    else {
	if (verbose) {
	    char str[MPI_MAX_ERROR_STRING];
	    int  slen;
	    /* Check the message */
	    MPI_Error_string( ierr, str, &slen );
	    printf( "Error message is: %s\n", str );
	}
	if (newcomm != MPI_COMM_NULL) {
	    errs++;
	    printf( "Comm_connect returned a communicator even with an error\n" );
	}
    }
    fflush(stdout);

    MTest_Finalize( errs );
    MPI_Finalize();

    return 0;
}
