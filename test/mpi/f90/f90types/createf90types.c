/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

static char MTEST_Descrip[] = "Test the routines to access the Fortran 90 datatypes from C";

/* Check the return from the routine */
static int checkType( const char str[], int p, int r, int f90kind,
		      int err, MPI_Datatype dtype )
{
    int errs = 0;
    if (dtype == MPI_DATATYPE_NULL) {
	printf( "Unable to find a real type for (p=%d,r=%d) in %s\n", 
		p, r, str );
	errs++;
    }
    if (err) {
	errs++;
	MTestPrintError( err );
    }

    if (!errs) {
	int nints, nadds, ndtypes, combiner;
	/* Check that we got the correct type */
	MPI_Type_get_envelope( dtype, &nints, &nadds, &ndtypes, &combiner );
	if (combiner != f90kind) {
	    errs++;
	    printf( "Wrong combiner type (got %d, should be %d) for %s\n", 
		    combiner, f90kind, str );
	}
	else {
	    int          parms[2];
	    MPI_Datatype outtype;
	    parms[0] = 0;
	    parms[1] = 0;

	    if (ndtypes != 0) {
		errs++;
		printf( "Section 8.6 states that the arraay_of_datatypes entry is empty for the create_f90 types\n" );
	    }
	    MPI_Type_get_contents( dtype, 2, 0, 1, parms, 0, &outtype );
	    switch (combiner) {
	    case MPI_COMBINER_F90_REAL:
	    case MPI_COMBINER_F90_COMPLEX:
		if (nints != 2) {
		    errs++;
		    printf( "Returned %d integer values, 2 expected for %s\n", 
			    nints, str );
		}
		if (parms[0] != p || parms[1] != r) {
		    errs++;
		    printf( "Returned (p=%d,r=%d); expected (p=%d,r=%d) for %s\n",
			    parms[0], parms[1], p, r, str );
		}
		break;
	    case MPI_COMBINER_F90_INTEGER:
		if (nints != 1) {
		    errs++;
		    printf( "Returned %d integer values, 1 expected for %s\n", 
			    nints, str );
		}
		if (parms[0] != p) {
		    errs++;
		    printf( "Returned (p=%d); expected (p=%d) for %s\n",
			    parms[0], p, str );
		}
		break;
	    default:
		errs++;
		printf( "Unrecognized combiner for %s\n", str );
		break;
	    }
	    
	}
    }
    return errs;
}

int main( int argc, char *argv[] )
{
    int p, r;
    int errs = 0;
    int err;
    int i, nLoop = 1;
    MPI_Datatype newtype;

    MPI_Init(0,0);

    if (argc > 1) {
	nLoop = atoi( argv[1] );
    }
    /* Set the handler to errors return, since according to the
       standard, it is invalid to provide p and/or r that are unsupported */

    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    for (i=0; i<nLoop; i++) {
	/* printf( "+" );fflush(stdout); */
	/* This should be a valid type similar to MPI_REAL */
	p = 3;
	r = 10;
	err = MPI_Type_create_f90_real( p, r, &newtype );
	errs += checkType( "REAL", p, r, MPI_COMBINER_F90_REAL, err, newtype );
	
	r = MPI_UNDEFINED;
	err = MPI_Type_create_f90_real( p, r, &newtype );
	errs += checkType( "REAL", p, r, MPI_COMBINER_F90_REAL, err, newtype );
	
	p = MPI_UNDEFINED;
	r = 10;
	err = MPI_Type_create_f90_real( p, r, &newtype );
	errs += checkType( "REAL", p, r, MPI_COMBINER_F90_REAL, err, newtype );

	/* This should be a valid type similar to MPI_COMPLEX */
	p = 3;
	r = 10;
	err = MPI_Type_create_f90_complex( p, r, &newtype );
	errs += checkType( "COMPLEX", p, r, MPI_COMBINER_F90_COMPLEX, 
			   err, newtype );
	
	r = MPI_UNDEFINED;
	err = MPI_Type_create_f90_complex( p, r, &newtype );
	errs += checkType( "COMPLEX", p, r, MPI_COMBINER_F90_COMPLEX, 
			   err, newtype );
	
	p = MPI_UNDEFINED;
	r = 10;
	err = MPI_Type_create_f90_complex( p, r, &newtype );
	errs += checkType( "COMPLEX", p, r, MPI_COMBINER_F90_COMPLEX, 
			   err, newtype );
	
	/* This should be a valid type similar to MPI_INTEGER */
	p = 3;
	err = MPI_Type_create_f90_integer( p, &newtype );
	errs += checkType( "INTEGER", p, r, MPI_COMBINER_F90_INTEGER, 
			   err, newtype );
    }

    if (errs == 0) {
	printf( " No Errors\n" );
    }
    else {
	printf( " Found %d errors\n", errs );
    }

    MPI_Finalize();
    return 0;
}
