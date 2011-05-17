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

/*
static char MTEST_Descrip[] = "Test of type_match_size";
*/

/*
 * type match size is part of the extended Fortran support, and may not
 * be present in 
 */

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int dsize;
    MPI_Datatype  newtype;

    MTest_Init( &argc, &argv );

    /* Check the most likely cases.  Note that it is an error to
       free the type returned by MPI_Type_match_size.  Also note
       that it is an error to request a size not supported by the compiler,
       so Type_match_size should generate an error in that case */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    err = MPI_Type_match_size( MPI_TYPECLASS_REAL, sizeof(float), &newtype );
    if (err) {
	errs++;
	MTestPrintErrorMsg( "Float: ", err );
    }
    else {
	err = MPI_Type_size( newtype, &dsize );
	if (err) {
	    errs++;
	    MTestPrintErrorMsg( "Float type: ", err );
	}
	else {
	    if (dsize != sizeof(float)) {
		errs++;
		printf( "Unexpected size for float (%d != %d)\n", 
			dsize, (int) sizeof(float) );
	    }
	}
    }

    err = MPI_Type_match_size( MPI_TYPECLASS_REAL, sizeof(double), &newtype );
    if (err) {
	errs++;
	MTestPrintErrorMsg( "Double: ", err );
    }
    else {
	MPI_Type_size( newtype, &dsize );
	if (dsize != sizeof(double)) {
	    errs++;
	    printf( "Unexpected size for double\n" );
	}
    }
#ifdef HAVE_LONG_DOUBLE
    err = MPI_Type_match_size( MPI_TYPECLASS_REAL, sizeof(long double), &newtype );
    if (err) {
	errs++;
	MTestPrintErrorMsg( "Long double: ", err );
    }
    else {
	MPI_Type_size( newtype, &dsize );
	if (dsize != sizeof(long double)) {
	    errs++;
	    printf( "Unexpected size for long double\n" );
	}
    }
#endif
    
    err = MPI_Type_match_size( MPI_TYPECLASS_INTEGER, sizeof(short), &newtype );
    if (err) {
	errs++;
	MTestPrintErrorMsg( "Short: ", err );
    }
    else {
	MPI_Type_size( newtype, &dsize );
	if (dsize != sizeof(short)) {
	    errs++;
	    printf( "Unexpected size for short\n" );
	}
    }

    err = MPI_Type_match_size( MPI_TYPECLASS_INTEGER, sizeof(int), &newtype );
    if (err) {
	errs++;
	MTestPrintErrorMsg( "Int: ", err );
    }
    else {
	MPI_Type_size( newtype, &dsize );
	if (dsize != sizeof(int)) {
	    errs++;
	    printf( "Unexpected size for int\n" );
	}
    }

    err = MPI_Type_match_size( MPI_TYPECLASS_INTEGER, sizeof(long), &newtype );
    if (err) {
	errs++;
	MTestPrintErrorMsg( "Long: ", err );
    }
    else {
	MPI_Type_size( newtype, &dsize );
	if (dsize != sizeof(long)) {
	    errs++;
	    printf( "Unexpected size for long\n" );
	}
    }
#ifdef HAVE_LONG_LONG
    err = MPI_Type_match_size( MPI_TYPECLASS_INTEGER, sizeof(long long), &newtype );
    if (err) {
	errs++;
	MTestPrintErrorMsg( "Long long: ", err );
    }
    else {
	MPI_Type_size( newtype, &dsize );
	if (dsize != sizeof(long long)) {
	    errs++;
	    printf( "Unexpected size for long long\n" );
	}
    }
#endif

    /* COMPLEX is a FORTRAN type.  The MPICH2 Type_match_size attempts
       to give a valid datatype, but if Fortran is not available,
       MPI_COMPLEX and MPI_DOUBLE_COMPLEX are not supported.  
       Allow this case by testing for MPI_DATATYPE_NULL */
    if (MPI_COMPLEX != MPI_DATATYPE_NULL) {
	err = MPI_Type_match_size( MPI_TYPECLASS_COMPLEX, 2*sizeof(float), &newtype );
	if (err) {
	    errs++;
	    MTestPrintErrorMsg( "Complex: ", err );
	}
	else {
	    MPI_Type_size( newtype, &dsize );
	    if (dsize != 2*sizeof(float)) {
		errs++;
		printf( "Unexpected size for complex\n" );
	    }
	}
    }

    if (MPI_COMPLEX != MPI_DATATYPE_NULL &&
	MPI_DOUBLE_COMPLEX != MPI_DATATYPE_NULL) {
	err = MPI_Type_match_size( MPI_TYPECLASS_COMPLEX, 2*sizeof(double), &newtype );
	if (err) {
	    errs++;
	    MTestPrintErrorMsg( "Double complex: ", err );
	}
	else {
	    MPI_Type_size( newtype, &dsize );
	    if (dsize != 2*sizeof(double)) {
		errs++;
		printf( "Unexpected size for double complex\n" );
	    }
	}
    }
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
