/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
 * This program tests the allocation (and deallocation) of contexts.
 * 
 */
int main( int argc, char **argv )
{
    int errs = 0;
    int i, j, err;
    MPI_Comm newcomm1, newcomm2[200];

    MTest_Init( &argc, &argv );

    /* Get a separate communicator to duplicate */
    MPI_Comm_dup( MPI_COMM_WORLD, &newcomm1 );

    MPI_Errhandler_set( newcomm1, MPI_ERRORS_RETURN );
    /* Allocate many communicators in batches, then free them */
    for (i=0; i<1000; i++) {
	for (j=0; j<200; j++) {
	    err = MPI_Comm_dup( newcomm1, &newcomm2[j] );
	    if (err) {
		errs++;
		if (errs < 10) {
		    fprintf( stderr, "Failed to duplicate communicator for (%d,%d)\n", i, j );
		    MTestPrintError( err );
		}
	    }
	}
	for (j=0; j<200; j++) {
	    err = MPI_Comm_free( &newcomm2[j] );
	    if (err) {
		errs++;
		if (errs < 10) {
		    fprintf( stderr, "Failed to free %d,%d\n", i, j );
		    MTestPrintError( err );
		}
	    }
	}
    }
    err = MPI_Comm_free( &newcomm1 );
    if (err) {
	errs++;
	fprintf( stderr, "Failed to free newcomm1\n" );
	MTestPrintError( err );
    }
      
    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}
