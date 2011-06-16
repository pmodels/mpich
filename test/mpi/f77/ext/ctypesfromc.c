/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * This file contains the C routines used in testing that all C datatypes
 * are available in Fortran and have the correct values.
 *
 * The tests follow this pattern:
 *
 *  Fortran main program
 *     calls the c routine f2ctype with each of the C types and the name of 
 *     the type.  That c routine using MPI_Type_f2c to convert the 
 *     Fortran handle to a C handle, and then compares it to the corresponding
 *     C type, which is found by looking up the C handle by name
 *
 *     C routine uses xxx_f2c routine to get C handle, checks some
 *     properties (i.e., size and rank of communicator, contents of datatype)
 *
 *     Then the Fortran main program calls a C routine that provides
 *     a handle, and the Fortran program performs similar checks.
 *
 * We also assume that a C int is a Fortran integer.  If this is not the
 * case, these tests must be modified.
 */

/* style: allow:fprintf:10 sig:0 */
#include <stdio.h>
#include "mpi.h"
#include "../../include/mpitestconf.h"
#include <string.h>

/* Create an array with all of the MPI names in it */
/* This is extracted from the test in test/mpi/types/typename.c ; only the
   C types are included. */

typedef struct mpi_names_t { MPI_Datatype dtype; const char *name; } mpi_names_t;

/* The MPI standard specifies that the names must be the MPI names,
   not the related language names (e.g., MPI_CHAR, not char) */

static mpi_names_t mpi_names[] = {
    { MPI_CHAR, "MPI_CHAR" },
    { MPI_SIGNED_CHAR, "MPI_SIGNED_CHAR" },
    { MPI_UNSIGNED_CHAR, "MPI_UNSIGNED_CHAR" },
    { MPI_WCHAR, "MPI_WCHAR" },
    { MPI_SHORT, "MPI_SHORT" },
    { MPI_UNSIGNED_SHORT, "MPI_UNSIGNED_SHORT" },
    { MPI_INT, "MPI_INT" },
    { MPI_UNSIGNED, "MPI_UNSIGNED" },
    { MPI_LONG, "MPI_LONG" },
    { MPI_UNSIGNED_LONG, "MPI_UNSIGNED_LONG" },
    { MPI_FLOAT, "MPI_FLOAT" },
    { MPI_DOUBLE, "MPI_DOUBLE" },
    { MPI_FLOAT_INT, "MPI_FLOAT_INT" },
    { MPI_DOUBLE_INT, "MPI_DOUBLE_INT" },
    { MPI_LONG_INT, "MPI_LONG_INT" },
    { MPI_SHORT_INT, "MPI_SHORT_INT" },
    { MPI_2INT, "MPI_2INT" },
    { MPI_LONG_DOUBLE, "MPI_LONG_DOUBLE" },
    { MPI_LONG_LONG_INT, "MPI_LONG_LONG_INT" }, 
    { MPI_LONG_LONG, "MPI_LONG_LONG" },
    { MPI_UNSIGNED_LONG_LONG, "MPI_UNSIGNED_LONG_LONG" }, 
    { MPI_LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT" },
    { 0, (char *)0 },  /* Sentinal used to indicate the last element */
};

/* 
   Name mapping.  All routines are created with names that are lower case
   with a single trailing underscore.  This matches many compilers.
   We use #define to change the name for Fortran compilers that do
   not use the lowercase/underscore pattern 
*/

#ifdef F77_NAME_UPPER
#define f2ctype_ F2CTYPE

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define f2ctype_ f2ctype

#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else 
#error 'Unrecognized Fortran name mapping'
#endif

/* Prototypes to keep compilers happy */
int f2ctype_( MPI_Fint *, MPI_Fint * );

/* */
int f2ctype_( MPI_Fint *fhandle, MPI_Fint *typeidx )
{
    int errs = 0;
    MPI_Datatype ctype;

    /* printf( "Testing %s\n", mpi_names[*typeidx].name ); */
    ctype = MPI_Type_f2c( *fhandle );
    if (ctype != mpi_names[*typeidx].dtype) {
	char mytypename[MPI_MAX_OBJECT_NAME];
	int mytypenamelen;
	/* An implementation is not *required* to deliver the 
	   corresponding C version of the MPI Datatype bit-for-bit.  But 
	   if *must* act like it - e.g., the datatype name must be the same */
	MPI_Type_get_name( ctype, mytypename, &mytypenamelen );
	if (strcmp( mytypename, mpi_names[*typeidx].name ) != 0) {
	    errs++;
	    printf( "C and Fortran types for %s (c name is %s) do not match f=%d, ctof=%d.\n",
		    mpi_names[*typeidx].name, mytypename, *fhandle, MPI_Type_c2f( ctype ) );
	}
    }
    
    return errs;
}
