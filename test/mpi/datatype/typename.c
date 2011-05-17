/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <string.h>

/* Create an array with all of the MPI names in it */

typedef struct mpi_names_t { MPI_Datatype dtype; const char *name; } mpi_names_t;

/* The MPI standard specifies that the names must be the MPI names,
   not the related language names (e.g., MPI_CHAR, not char) */

static mpi_names_t mpi_names[] = {
    { MPI_CHAR, "MPI_CHAR" },
    { MPI_SIGNED_CHAR, "MPI_SIGNED_CHAR" },
    { MPI_UNSIGNED_CHAR, "MPI_UNSIGNED_CHAR" },
    { MPI_BYTE, "MPI_BYTE" },
    { MPI_WCHAR, "MPI_WCHAR" },
    { MPI_SHORT, "MPI_SHORT" },
    { MPI_UNSIGNED_SHORT, "MPI_UNSIGNED_SHORT" },
    { MPI_INT, "MPI_INT" },
    { MPI_UNSIGNED, "MPI_UNSIGNED" },
    { MPI_LONG, "MPI_LONG" },
    { MPI_UNSIGNED_LONG, "MPI_UNSIGNED_LONG" },
    { MPI_FLOAT, "MPI_FLOAT" },
    { MPI_DOUBLE, "MPI_DOUBLE" },
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* these two types were added in MPI-2.2 */
    { MPI_AINT, "MPI_AINT" },
    { MPI_OFFSET, "MPI_OFFSET" },
#endif

    { MPI_PACKED, "MPI_PACKED" },
    { MPI_LB, "MPI_LB" },
    { MPI_UB, "MPI_UB" },
    { MPI_FLOAT_INT, "MPI_FLOAT_INT" },
    { MPI_DOUBLE_INT, "MPI_DOUBLE_INT" },
    { MPI_LONG_INT, "MPI_LONG_INT" },
    { MPI_SHORT_INT, "MPI_SHORT_INT" },
    { MPI_2INT, "MPI_2INT" },
    /* Fortran */
#ifdef HAVE_FORTRAN_BINDING
    { MPI_COMPLEX, "MPI_COMPLEX" },
    { MPI_DOUBLE_COMPLEX, "MPI_DOUBLE_COMPLEX" },
    { MPI_LOGICAL, "MPI_LOGICAL" },
    { MPI_REAL, "MPI_REAL" },
    { MPI_DOUBLE_PRECISION, "MPI_DOUBLE_PRECISION" },
    { MPI_INTEGER, "MPI_INTEGER" },
    { MPI_2INTEGER, "MPI_2INTEGER" },
    /* 2COMPLEX (and the 2DOUBLE_COMPLEX) were in MPI 1.0 but not later */
#ifdef HAVE_MPI_2COMPLEX
    { MPI_2COMPLEX, "MPI_2COMPLEX" },
#endif
#ifdef HAVE_MPI_2DOUBLE_COMPLEX
    /* MPI_2DOUBLE_COMPLEX is an extension - it is not part of MPI 2.1 */
    { MPI_2DOUBLE_COMPLEX, "MPI_2DOUBLE_COMPLEX" },
#endif
    { MPI_2REAL, "MPI_2REAL" },
    { MPI_2DOUBLE_PRECISION, "MPI_2DOUBLE_PRECISION" },
    { MPI_CHARACTER, "MPI_CHARACTER" },
#endif
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* these C99 types were added in MPI-2.2 */
    { MPI_INT8_T,   "MPI_INT8_T"   },
    { MPI_INT16_T,  "MPI_INT16_T"  },
    { MPI_INT32_T,  "MPI_INT32_T"  },
    { MPI_INT64_T,  "MPI_INT64_T"  },
    { MPI_UINT8_T,  "MPI_UINT8_T"  },
    { MPI_UINT16_T, "MPI_UINT16_T" },
    { MPI_UINT32_T, "MPI_UINT32_T" },
    { MPI_UINT64_T, "MPI_UINT64_T" },
    { MPI_C_BOOL, "MPI_C_BOOL" },
    { MPI_C_FLOAT_COMPLEX,  "MPI_C_FLOAT_COMPLEX"  },
    { MPI_C_DOUBLE_COMPLEX, "MPI_C_DOUBLE_COMPLEX" },
    { MPI_AINT, "MPI_AINT" },
    { MPI_OFFSET, "MPI_OFFSET" },
#endif
    /* Size-specific types */
    /* Do not move MPI_REAL4 - this is used to indicate the very first 
       optional type.  In addition, you must not add any required types
       after this type */
    /* See MPI 2.1, Section 16.2.  These are required, predefined types. 
       If the type is not available (e.g., *only* because the Fortran
       compiler does not support it), the value may be MPI_DATATYPE_NULL */
    { MPI_REAL4, "MPI_REAL4" },
    { MPI_REAL8, "MPI_REAL8" },
    { MPI_REAL16, "MPI_REAL16" },
    { MPI_COMPLEX8, "MPI_COMPLEX8" },
    { MPI_COMPLEX16, "MPI_COMPLEX16" },
    { MPI_COMPLEX32, "MPI_COMPLEX32" },
    { MPI_INTEGER1, "MPI_INTEGER1" },
    { MPI_INTEGER2, "MPI_INTEGER2" },
    { MPI_INTEGER4, "MPI_INTEGER4" },
    { MPI_INTEGER8, "MPI_INTEGER8" },
#ifdef HAVE_MPI_INTEGER16
    /* MPI_INTEGER16 is not included in most of the tables in MPI 2.1,
       and some implementations omit it.  An error will be reported, but
       this ifdef allows the test to be built and run. */
    { MPI_INTEGER16, "MPI_INTEGER16" },
#endif
    /* Semi-optional types - if the compiler doesn't support long double
       or long long, these might be MPI_DATATYPE_NULL */
    { MPI_LONG_DOUBLE, "MPI_LONG_DOUBLE" },
    { MPI_LONG_LONG_INT, "MPI_LONG_LONG_INT" }, 
    { MPI_LONG_LONG, "MPI_LONG_LONG" },
    { MPI_UNSIGNED_LONG_LONG, "MPI_UNSIGNED_LONG_LONG" }, 
    { MPI_LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT" },
#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* added in MPI-2.2 */
    { MPI_C_LONG_DOUBLE_COMPLEX, "MPI_C_LONG_DOUBLE_COMPLEX" },
#endif
    { 0, (char *)0 },  /* Sentinal used to indicate the last element */
};

int main( int argc, char **argv )
{
    char name[MPI_MAX_OBJECT_NAME];
    int namelen, i, inOptional;
    int errs = 0;

    MTest_Init( &argc, &argv );
    
    /* Sample some datatypes */
    /* See 8.4, "Naming Objects" in MPI-2.  The default name is the same
       as the datatype name */
    MPI_Type_get_name( MPI_DOUBLE, name, &namelen );
    if (strncmp( name, "MPI_DOUBLE", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected MPI_DOUBLE but got :%s:\n", name );
    }

    MPI_Type_get_name( MPI_INT, name, &namelen );
    if (strncmp( name, "MPI_INT", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected MPI_INT but got :%s:\n", name );
    }

    /* Now we try them ALL */
    inOptional = 0;
    for (i=0; mpi_names[i].name != 0; i++) {
	/* Are we in the optional types? */
	if (strcmp( mpi_names[i].name, "MPI_REAL4" ) == 0) 
	    inOptional = 1;
	/* If this optional type is not supported, skip it */
	if (inOptional && mpi_names[i].dtype == MPI_DATATYPE_NULL) continue;
	if (mpi_names[i].dtype == MPI_DATATYPE_NULL) {
	    /* Report an error because all of the standard types 
	       must be supported */
	    errs++;
	    fprintf( stderr, "MPI Datatype %s is MPI_DATATYPE_NULL\n", 
		     mpi_names[i].name );
	    continue;
	}
	MTestPrintfMsg( 10, "Checking type %s\n", mpi_names[i].name );
	name[0] = 0;
	MPI_Type_get_name( mpi_names[i].dtype, name, &namelen );
	if (strncmp( name, mpi_names[i].name, namelen )) {
	    errs++;
	    fprintf( stderr, "Expected %s but got %s\n", 
		     mpi_names[i].name, name );
	}
    }

    /* Try resetting the name */
    MPI_Type_set_name( MPI_INT, (char*)"int" );
    name[0] = 0;
    MPI_Type_get_name( MPI_INT, name, &namelen );
    if (strncmp( name, "int", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected int but got :%s:\n", name );
    }

#ifndef HAVE_MPI_INTEGER16
    errs++;
    fprintf( stderr, "MPI_INTEGER16 is not available\n" );
#endif

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
