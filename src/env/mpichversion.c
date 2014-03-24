/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpiimpl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: We really should consider internationalizing the output from this
   program */
/* style: allow:fprintf:1 sig:0 */
/* style: allow:printf:8 sig:0 */

/*
 * This program reports on properties of the MPICH library, such as the
 * version, device, and what patches have been applied.  This is available
 * only since MPICH 1.0.6.  
 *
 * The reason that this program doesn't directly include the info is that it
 * can be compiled and then linked with the MPICH library to discover
 * the information about the version of the library.  If built with shared
 * libraries, this will give the information about the currently installed
 * shared library.
 */

typedef enum { Version_number=0, Date=1, 
	       Patches=2, Configure_args=3, Device=4,
	       Compilers=5, LastField
             } fields;

/*D
  mpichversion - Report on the MPICH version

  Command Line Arguments:
+ -version - Show the version of MPICH
. -date    - Show the release date of this version
. -patches - Show the identifiers for any applied patches
. -configure - Show the configure arguments used to build MPICH
- -device  - Show the device for which MPICH was configured

  Using this program:
  To use this program, link it against 'libmpi.a' (use 'mpicc' or
  the whichever compiler command is used to create MPICH programs)
  D*/

int main( int argc, char *argv[] )
{
    int i, flags[10];
    
    if (argc <= 1) {
	/* Show all values */
	for (i=0; i<LastField; i++) flags[i] = 1;
    }
    else {
	/* Show only requested values */
	for (i=0; i<LastField; i++) flags[i] = 0;
	for (i=1; i<argc; i++) {
	    if (strcmp( argv[i], "-version" ) == 0 || 
		strcmp( argv[i], "--version" ) == 0 || 
		strcmp( argv[i], "-v" ) == 0) 
		flags[Version_number] = 1;
	    else if (strcmp( argv[i], "-date" ) == 0 ||
		     strcmp( argv[i], "--date" ) == 0 ||
		     strcmp( argv[i], "-D" ) == 0) 
		flags[Date] = 1;
	    else if (strcmp( argv[i], "-patches" ) == 0)
		flags[Patches] = 1;
	    else if (strcmp( argv[i], "-configure" ) == 0 ||
		     strcmp( argv[i], "--configure" ) == 0 ||
		     strcmp( argv[i], "-c" ) == 0) 
		flags[Configure_args] = 1;
	    else if (strcmp( argv[i], "-device" ) == 0 ||
		     strcmp( argv[i], "--device" ) == 0 ||
		     strcmp( argv[i], "-d" ) == 0)
		flags[Device] = 1;
	    else if (strcmp( argv[i], "-compiler" ) == 0 ||
		     strcmp( argv[i], "--compiler" ) == 0 ||
		     strcmp( argv[i], "-b" ) == 0)
		flags[Compilers] = 1;
	    else {
		fprintf( stderr, "Unrecognized argument %s\n", argv[i] );
		exit(1);
	    }
	}
    }

    /* Print out the information, one item per line */
    if (flags[Version_number]) {
	printf( "MPICH Version:    \t%s\n", MPIR_Version_string );
    }
    if (flags[Date]) {
	printf( "MPICH Release date:\t%s\n", MPIR_Version_date );
    }
    if (flags[Device]) {
	printf( "MPICH Device:    \t%s\n", MPIR_Version_device );
    }
    if (flags[Configure_args]) {
	printf( "MPICH configure: \t%s\n", MPIR_Version_configure );
    }
    if (flags[Compilers]) {
	printf( "MPICH CC: \t%s\n", MPIR_Version_CC );
	printf( "MPICH CXX: \t%s\n", MPIR_Version_CXX );
	printf( "MPICH F77: \t%s\n", MPIR_Version_F77 );
	printf( "MPICH FC: \t%s\n", MPIR_Version_FC );
    }

    return 0;
}
