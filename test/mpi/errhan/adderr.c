/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* Create NCLASSES new classes, each with 5 codes (160 total) */
#define NCLASSES 32
#define NCODES   5

int main( int argc, char *argv[] )
{
    int errs = 0;
    char string[MPI_MAX_ERROR_STRING], outstring[MPI_MAX_ERROR_STRING];
    int newclass[NCLASSES], newcode[NCLASSES][NCODES];
    int i, j, slen, outclass;

    MTest_Init( &argc, &argv );

    /* Initialize the new codes */
    for (i=0; i<NCLASSES; i++) {
	MPI_Add_error_class( &newclass[i] );
	for (j=0; j<NCODES; j++) {
	    MPI_Add_error_code( newclass[i], &newcode[i][j] );
	    sprintf( string, "code for class %d code %d\n", i, j );
	    MPI_Add_error_string( newcode[i][j], string );
	}
    }

    /* check the values */
    for (i=0; i<NCLASSES; i++) {
	MPI_Error_class( newclass[i], &outclass );
	if (outclass != newclass[i]) {
	    errs++;
	    printf( "Error class %d is not a valid error code %x %x\n", i,
		    outclass, newclass[i]);
	}
	for (j=0; j<NCODES; j++) {
	    MPI_Error_class( newcode[i][j], &outclass );
	    if (outclass != newclass[i]) {
		errs++;
		printf( "Class of code for %d is not correct %x %x\n", j,
			outclass, newclass[i] );
	    }
	    MPI_Error_string( newcode[i][j], outstring, &slen );
	    sprintf( string, "code for class %d code %d\n", i, j );
	    if (strcmp( outstring, string )) {
		errs++;
		printf( "Error string is :%s: but should be :%s:\n",
			outstring, string );
	    }
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
