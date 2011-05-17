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
static char MTEST_Descrip[] = "Test MPI_BXOR operations on optional datatypes dupported by MPICH2";
*/

/*
 * This test looks at the handling of logical and for types that are not 
 * integers or are not required integers (e.g., long long).  MPICH2 allows
 * these as well.  A strict MPI test should not include this test.
 */
int main( int argc, char *argv[] )
{
    int errs = 0;
    int rc;
    int rank, size;
    MPI_Comm      comm;
    char cinbuf[3], coutbuf[3];
    signed char scinbuf[3], scoutbuf[3];
    unsigned char ucinbuf[3], ucoutbuf[3];
    short sinbuf[3], soutbuf[3];
    unsigned short usinbuf[3], usoutbuf[3];
    long linbuf[3], loutbuf[3];
    unsigned long ulinbuf[3], uloutbuf[3];
    unsigned uinbuf[3], uoutbuf[3];
    int iinbuf[3], ioutbuf[3];
    

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;
    /* Set errors return so that we can provide better information 
       should a routine reject one of the operand/datatype pairs */
    MPI_Errhandler_set( comm, MPI_ERRORS_RETURN );

    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

#ifndef USE_STRICT_MPI
    /* char */
    MTestPrintfMsg( 10, "Reduce of MPI_CHAR\n" );
    cinbuf[0] = 0xff;
    cinbuf[1] = 0;
    cinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    coutbuf[0] = 0xf;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    rc = MPI_Reduce( cinbuf, coutbuf, 3, MPI_CHAR, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_CHAR", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (coutbuf[0] != ((size % 2) ? (char)0xff : (char)0) ) {
		errs++;
		fprintf( stderr, "char BXOR(1) test failed\n" );
	    }
	    if (coutbuf[1]) {
		errs++;
		fprintf( stderr, "char BXOR(0) test failed\n" );
	    }
	    if (coutbuf[2] != ((size % 2) ? (char)0xc3 : (char)0xff)) {
		errs++;
		fprintf( stderr, "char BXOR(>) test failed\n" );
	    }
	}
    }
#endif /* USE_STRICT_MPI */

    /* signed char */
    MTestPrintfMsg( 10, "Reduce of MPI_SIGNED_CHAR\n" );
    scinbuf[0] = 0xff;
    scinbuf[1] = 0;
    scinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    scoutbuf[0] = 0xf;
    scoutbuf[1] = 1;
    scoutbuf[2] = 1;
    rc = MPI_Reduce( scinbuf, scoutbuf, 3, MPI_SIGNED_CHAR, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_SIGNED_CHAR", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (scoutbuf[0] != ((size % 2) ? (signed char)0xff : (signed char)0) ) {
		errs++;
		fprintf( stderr, "signed char BXOR(1) test failed\n" );
	    }
	    if (scoutbuf[1]) {
		errs++;
		fprintf( stderr, "signed char BXOR(0) test failed\n" );
	    }
	    if (scoutbuf[2] != ((size % 2) ? (signed char)0xc3 : (signed char)0xff)) {
		errs++;
		fprintf( stderr, "signed char BXOR(>) test failed\n" );
	    }
	}
    }

    /* unsigned char */
    MTestPrintfMsg( 10, "Reduce of MPI_UNSIGNED_CHAR\n" );
    ucinbuf[0] = 0xff;
    ucinbuf[1] = 0;
    ucinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    ucoutbuf[0] = 0;
    ucoutbuf[1] = 1;
    ucoutbuf[2] = 1;
    rc = MPI_Reduce( ucinbuf, ucoutbuf, 3, MPI_UNSIGNED_CHAR, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_UNSIGNED_CHAR", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (ucoutbuf[0] != ((size % 2) ? 0xff : 0)) {
		errs++;
		fprintf( stderr, "unsigned char BXOR(1) test failed\n" );
	    }
	    if (ucoutbuf[1]) {
		errs++;
		fprintf( stderr, "unsigned char BXOR(0) test failed\n" );
	    }
	    if (ucoutbuf[2] != ((size % 2) ? (unsigned char)0xc3 : (unsigned char)0xff)) {
		errs++;
		fprintf( stderr, "unsigned char BXOR(>) test failed\n" );
	    }
	}
    }

    /* bytes */
    MTestPrintfMsg( 10, "Reduce of MPI_BYTE\n" );
    cinbuf[0] = 0xff;
    cinbuf[1] = 0;
    cinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    coutbuf[0] = 0;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    rc = MPI_Reduce( cinbuf, coutbuf, 3, MPI_BYTE, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_BYTE", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (coutbuf[0] != ((size % 2) ? (char)0xff : 0)) {
		errs++;
		fprintf( stderr, "byte BXOR(1) test failed\n" );
	    }
	    if (coutbuf[1]) {
		errs++;
		fprintf( stderr, "byte BXOR(0) test failed\n" );
	    }
	    if (coutbuf[2] != ((size % 2) ? (char)0xc3 : (char)0xff)) {
		errs++;
		fprintf( stderr, "byte BXOR(>) test failed\n" );
	    }
	}
    }

    /* short */
    MTestPrintfMsg( 10, "Reduce of MPI_SHORT\n" );
    sinbuf[0] = 0xffff;
    sinbuf[1] = 0;
    sinbuf[2] = (rank > 0) ? 0x3c3c : 0xc3c3;

    soutbuf[0] = 0;
    soutbuf[1] = 1;
    soutbuf[2] = 1;
    rc = MPI_Reduce( sinbuf, soutbuf, 3, MPI_SHORT, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_SHORT", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (soutbuf[0] != ((size % 2) ? (short)0xffff : 0)) {
		errs++;
		fprintf( stderr, "short BXOR(1) test failed\n" );
	    }
	    if (soutbuf[1]) {
		errs++;
		fprintf( stderr, "short BXOR(0) test failed\n" );
	    }
	    if (soutbuf[2] != ((size % 2) ? (short)0xc3c3 : (short)0xffff)) {
		errs++;
		fprintf( stderr, "short BXOR(>) test failed\n" );
	    }
	}
    }

    /* unsigned short */
    MTestPrintfMsg( 10, "Reduce of MPI_UNSIGNED_SHORT\n" );
    usinbuf[0] = 0xffff;
    usinbuf[1] = 0;
    usinbuf[2] = (rank > 0) ? 0x3c3c : 0xc3c3;

    usoutbuf[0] = 0;
    usoutbuf[1] = 1;
    usoutbuf[2] = 1;
    rc = MPI_Reduce( usinbuf, usoutbuf, 3, MPI_UNSIGNED_SHORT, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_UNSIGNED_SHORT", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (usoutbuf[0] != ((size % 2) ? 0xffff : 0)) {
		errs++;
		fprintf( stderr, "short BXOR(1) test failed\n" );
	    }
	    if (usoutbuf[1]) {
		errs++;
		fprintf( stderr, "short BXOR(0) test failed\n" );
	    }
	    if (usoutbuf[2] != ((size % 2) ? 0xc3c3 : 0xffff)) {
		errs++;
		fprintf( stderr, "short BXOR(>) test failed\n" );
	    }
	}
    }

    /* unsigned */
    MTestPrintfMsg( 10, "Reduce of MPI_UNSIGNED\n" );
    uinbuf[0] = 0xffffffff;
    uinbuf[1] = 0;
    uinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    uoutbuf[0] = 0;
    uoutbuf[1] = 1;
    uoutbuf[2] = 1;
    rc = MPI_Reduce( uinbuf, uoutbuf, 3, MPI_UNSIGNED, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_UNSIGNED", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (uoutbuf[0] != ((size % 2) ? 0xffffffff : 0)) {
		errs++;
		fprintf( stderr, "unsigned BXOR(1) test failed\n" );
	    }
	    if (uoutbuf[1]) {
		errs++;
		fprintf( stderr, "unsigned BXOR(0) test failed\n" );
	    }
	    if (uoutbuf[2] != ((size % 2) ? 0xc3c3c3c3 : 0xffffffff)) {
		errs++;
		fprintf( stderr, "unsigned BXOR(>) test failed\n" );
	    }
	}
    }

    /* int */
    MTestPrintfMsg( 10, "Reduce of MPI_INT\n" );
    iinbuf[0] = 0xffffffff;
    iinbuf[1] = 0;
    iinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    ioutbuf[0] = 0;
    ioutbuf[1] = 1;
    ioutbuf[2] = 1;
    rc = MPI_Reduce( iinbuf, ioutbuf, 3, MPI_INT, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_INT", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (ioutbuf[0] != ((size % 2) ? 0xffffffff : 0)) {
		errs++;
		fprintf( stderr, "int BXOR(1) test failed\n" );
	    }
	    if (ioutbuf[1]) {
		errs++;
		fprintf( stderr, "int BXOR(0) test failed\n" );
	    }
	    if (ioutbuf[2] != ((size % 2) ? 0xc3c3c3c3 : 0xffffffff)) {
		errs++;
		fprintf( stderr, "int BXOR(>) test failed\n" );
	    }
	}
    }

    /* long */
    MTestPrintfMsg( 10, "Reduce of MPI_LONG\n" );
    linbuf[0] = 0xffffffff;
    linbuf[1] = 0;
    linbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    loutbuf[0] = 0;
    loutbuf[1] = 1;
    loutbuf[2] = 1;
    rc = MPI_Reduce( linbuf, loutbuf, 3, MPI_LONG, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_LONG", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (loutbuf[0] != ((size % 2) ? 0xffffffff : 0)) {
		errs++;
		fprintf( stderr, "long BXOR(1) test failed\n" );
	    }
	    if (loutbuf[1]) {
		errs++;
		fprintf( stderr, "long BXOR(0) test failed\n" );
	    }
	    if (loutbuf[2] != ((size % 2) ? 0xc3c3c3c3 : 0xffffffff)) {
		errs++;
		fprintf( stderr, "long BXOR(>) test failed\n" );
	    }
	}
    }

    /* unsigned long */
    MTestPrintfMsg( 10, "Reduce of MPI_UNSIGNED_LONG\n" );
    ulinbuf[0] = 0xffffffff;
    ulinbuf[1] = 0;
    ulinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    uloutbuf[0] = 0;
    uloutbuf[1] = 1;
    uloutbuf[2] = 1;
    rc = MPI_Reduce( ulinbuf, uloutbuf, 3, MPI_UNSIGNED_LONG, MPI_BXOR, 0, comm );
    if (rc) {
	MTestPrintErrorMsg( "MPI_BXOR and MPI_UNSIGNED_LONG", rc );
	errs++;
    }
    else {
	if (rank == 0) {
	    if (uloutbuf[0] != ((size % 2) ? 0xffffffff : 0)) {
		errs++;
		fprintf( stderr, "unsigned long BXOR(1) test failed\n" );
	    }
	    if (uloutbuf[1]) {
		errs++;
		fprintf( stderr, "unsigned long BXOR(0) test failed\n" );
	    }
	    if (uloutbuf[2] != ((size % 2) ? 0xc3c3c3c3 : 0xffffffff)) {
		errs++;
		fprintf( stderr, "unsigned long BXOR(>) test failed\n" );
	    }
	}
    }

#ifdef HAVE_LONG_LONG
    {
	long long llinbuf[3], lloutbuf[3];
    /* long long */
    llinbuf[0] = 0xffffffff;
    llinbuf[1] = 0;
    llinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    lloutbuf[0] = 0;
    lloutbuf[1] = 1;
    lloutbuf[2] = 1;
    if (MPI_LONG_LONG != MPI_DATATYPE_NULL) {
	MTestPrintfMsg( 10, "Reduce of MPI_LONG_LONG\n" );
	rc = MPI_Reduce( llinbuf, lloutbuf, 3, MPI_LONG_LONG, MPI_BXOR, 0, comm );
	if (rc) {
	    MTestPrintErrorMsg( "MPI_BXOR and MPI_LONG_LONG", rc );
	    errs++;
	}
	else {
	    if (rank == 0) {
		if (lloutbuf[0] != ((size % 2) ? 0xffffffff : 0)) {
		    errs++;
		    fprintf( stderr, "long long BXOR(1) test failed\n" );
		}
		if (lloutbuf[1]) {
		    errs++;
		    fprintf( stderr, "long long BXOR(0) test failed\n" );
		}
		if (lloutbuf[2] != ((size % 2) ? 0xc3c3c3c3 : 0xffffffff)) {
		    errs++;
		    fprintf( stderr, "long long BXOR(>) test failed\n" );
		}
	    }
	}
    }
    }
#endif

    MPI_Errhandler_set( comm, MPI_ERRORS_ARE_FATAL );
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
