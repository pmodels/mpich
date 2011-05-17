/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test MPI_SUM operations on optional datatypes dupported by MPICH2";
*/

typedef struct { double r, i; } d_complex;

/*
 * This test looks at the handling of logical and for types that are not 
 * integers or are not required integers (e.g., long long).  MPICH2 allows
 * these as well.  A strict MPI test should not include this test.
 */
int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size;
    MPI_Comm      comm;
    char cinbuf[3], coutbuf[3];
    signed char scinbuf[3], scoutbuf[3];
    unsigned char ucinbuf[3], ucoutbuf[3];
    d_complex dinbuf[3], doutbuf[3];

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

#ifndef USE_STRICT_MPI
    /* char */
    MTestPrintfMsg( 10, "Reduce of MPI_CHAR\n" );
    cinbuf[0] = 1;
    cinbuf[1] = 0;
    cinbuf[2] = (rank > 0);

    coutbuf[0] = 0;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    MPI_Reduce( cinbuf, coutbuf, 3, MPI_CHAR, MPI_SUM, 0, comm );
    if (rank == 0) {
	if (size < 128 && coutbuf[0] != size) {
	    errs++;
	    fprintf( stderr, "char SUM(1) test failed\n" );
	}
	if (size < 128 && coutbuf[1] != 0) {
	    errs++;
	    fprintf( stderr, "char SUM(0) test failed\n" );
	}
	if (size < 128 && coutbuf[2] != size - 1) {
	    errs++;
	    fprintf( stderr, "char SUM(>) test failed\n" );
	}
    }
#endif /* USE_MPI_STRICT */

    /* signed char */
    MTestPrintfMsg( 10, "Reduce of MPI_SIGNED_CHAR\n" );
    scinbuf[0] = 1;
    scinbuf[1] = 0;
    scinbuf[2] = (rank > 0);

    scoutbuf[0] = 0;
    scoutbuf[1] = 1;
    scoutbuf[2] = 1;
    MPI_Reduce( scinbuf, scoutbuf, 3, MPI_SIGNED_CHAR, MPI_SUM, 0, comm );
    if (rank == 0) {
	if (size < 128 && scoutbuf[0] != size) {
	    errs++;
	    fprintf( stderr, "signed char SUM(1) test failed\n" );
	}
	if (size < 128 && scoutbuf[1] != 0) {
	    errs++;
	    fprintf( stderr, "signed char SUM(0) test failed\n" );
	}
	if (size < 128 && scoutbuf[2] != size - 1) {
	    errs++;
	    fprintf( stderr, "signed char SUM(>) test failed\n" );
	}
    }

    /* unsigned char */
    MTestPrintfMsg( 10, "Reduce of MPI_UNSIGNED_CHAR\n" );
    ucinbuf[0] = 1;
    ucinbuf[1] = 0;
    ucinbuf[2] = (rank > 0);

    ucoutbuf[0] = 0;
    ucoutbuf[1] = 1;
    ucoutbuf[2] = 1;
    MPI_Reduce( ucinbuf, ucoutbuf, 3, MPI_UNSIGNED_CHAR, MPI_SUM, 0, comm );
    if (rank == 0) {
	if (size < 128 && ucoutbuf[0] != size) {
	    errs++;
	    fprintf( stderr, "unsigned char SUM(1) test failed\n" );
	}
	if (size < 128 && ucoutbuf[1]) {
	    errs++;
	    fprintf( stderr, "unsigned char SUM(0) test failed\n" );
	}
	if (size < 128 && ucoutbuf[2] != size - 1) {
	    errs++;
	    fprintf( stderr, "unsigned char SUM(>) test failed\n" );
	}
    }

#ifndef USE_STRICT_MPI
    /* For some reason, complex is not allowed for sum and prod */
    if (MPI_DOUBLE_COMPLEX != MPI_DATATYPE_NULL) {
	MTestPrintfMsg( 10, "Reduce of MPI_DOUBLE_COMPLEX\n" );
	/* double complex; may be null if we do not have Fortran support */
	dinbuf[0].r = 1;
	dinbuf[1].r = 0;
	dinbuf[2].r = (rank > 0);
	dinbuf[0].i = -1;
	dinbuf[1].i = 0;
	dinbuf[2].i = -(rank > 0);

	doutbuf[0].r = 0;
	doutbuf[1].r = 1;
	doutbuf[2].r = 1;
	doutbuf[0].i = 0;
	doutbuf[1].i = 1;
	doutbuf[2].i = 1;
	MPI_Reduce( dinbuf, doutbuf, 3, MPI_DOUBLE_COMPLEX, MPI_SUM, 0, comm );
	if (rank == 0) {
	    if (doutbuf[0].r != size || doutbuf[0].i != -size) {
		errs++;
		fprintf( stderr, "double complex SUM(1) test failed\n" );
	    }
	    if (doutbuf[1].r != 0 || doutbuf[1].i != 0) {
		errs++;
		fprintf( stderr, "double complex SUM(0) test failed\n" );
	    }
	    if (doutbuf[2].r != size - 1 || doutbuf[2].i != 1 - size) {
		errs++;
		fprintf( stderr, "double complex SUM(>) test failed\n" );
	    }
	}
    }
#endif /* USE_STRICT_MPI */

#ifdef HAVE_LONG_DOUBLE
    { long double ldinbuf[3], ldoutbuf[3];
    /* long double */
    ldinbuf[0] = 1;
    ldinbuf[1] = 0;
    ldinbuf[2] = (rank > 0);

    ldoutbuf[0] = 0;
    ldoutbuf[1] = 1;
    ldoutbuf[2] = 1;
    if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL) {
	MTestPrintfMsg( 10, "Reduce of MPI_LONG_DOUBLE\n" );
	MPI_Reduce( ldinbuf, ldoutbuf, 3, MPI_LONG_DOUBLE, MPI_SUM, 0, comm );
	if (rank == 0) {
	    if (ldoutbuf[0] != size) {
		errs++;
		fprintf( stderr, "long double SUM(1) test failed\n" );
	    }
	    if (ldoutbuf[1] != 0.0) {
		errs++;
		fprintf( stderr, "long double SUM(0) test failed\n" );
	    }
	    if (ldoutbuf[2] != size - 1) {
		errs++;
		fprintf( stderr, "long double SUM(>) test failed\n" );
	    }
	}
    }
    }
#endif

#ifdef HAVE_LONG_LONG
    {
	long long llinbuf[3], lloutbuf[3];
    /* long long */
    llinbuf[0] = 1;
    llinbuf[1] = 0;
    llinbuf[2] = (rank > 0);

    lloutbuf[0] = 0;
    lloutbuf[1] = 1;
    lloutbuf[2] = 1;
    if (MPI_LONG_LONG != MPI_DATATYPE_NULL) {
	MTestPrintfMsg( 10, "Reduce of MPI_LONG_LONG\n" );
	MPI_Reduce( llinbuf, lloutbuf, 3, MPI_LONG_LONG, MPI_SUM, 0, comm );
	if (rank == 0) {
	    if (lloutbuf[0] != size) {
		errs++;
		fprintf( stderr, "long long SUM(1) test failed\n" );
	    }
	    if (lloutbuf[1] != 0) {
		errs++;
		fprintf( stderr, "long long SUM(0) test failed\n" );
	    }
	    if (lloutbuf[2] != size - 1) {
		errs++;
		fprintf( stderr, "long long SUM(>) test failed\n" );
	    }
	}
    }
    }
#endif

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
