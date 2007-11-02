/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
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
    { MPI_LONG_DOUBLE, "MPI_LONG_DOUBLE" },
/*    { MPI_LONG_LONG_INT, "MPI_LONG_LONG_INT" }, */
    { MPI_LONG_LONG, "MPI_LONG_LONG" },
    { MPI_UNSIGNED_LONG_LONG, "MPI_UNSIGNED_LONG_LONG" }, 
    { MPI_PACKED, "MPI_PACKED" },
    { MPI_LB, "MPI_LB" },
    { MPI_UB, "MPI_UB" },
    { MPI_FLOAT_INT, "MPI_FLOAT_INT" },
    { MPI_DOUBLE_INT, "MPI_DOUBLE_INT" },
    { MPI_LONG_INT, "MPI_LONG_INT" },
    { MPI_SHORT_INT, "MPI_SHORT_INT" },
    { MPI_2INT, "MPI_2INT" },
    { MPI_LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT" },
    /* Fortran */
    { MPI_COMPLEX, "MPI_COMPLEX" },
    { MPI_DOUBLE_COMPLEX, "MPI_DOUBLE_COMPLEX" },
    { MPI_LOGICAL, "MPI_LOGICAL" },
    { MPI_REAL, "MPI_REAL" },
    { MPI_DOUBLE_PRECISION, "MPI_DOUBLE_PRECISION" },
    { MPI_INTEGER, "MPI_INTEGER" },
    { MPI_2INTEGER, "MPI_2INTEGER" },
    { MPI_2COMPLEX, "MPI_2COMPLEX" },
    { MPI_2DOUBLE_COMPLEX, "MPI_2DOUBLE_COMPLEX" },
    { MPI_2REAL, "MPI_2REAL" },
    { MPI_2DOUBLE_PRECISION, "MPI_2DOUBLE_PRECISION" },
    { MPI_CHARACTER, "MPI_CHARACTER" },
    /* Size-specific types */
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
    { MPI_INTEGER16, "MPI_INTEGER16" },
    { 0, (char *)0 },  /* Sentinal used to indicate the last element */
};

int main( int argc, char **argv )
{
    char name[MPI_MAX_OBJECT_NAME];
    int namelen, i;
    int errs = 0;

    MPI_Init(0,0);
    
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
    for (i=0; mpi_names[i].name != 0; i++) {
	/* The size-specific types may be DATATYPE_NULL */
	if (mpi_names[i].dtype == MPI_DATATYPE_NULL) continue;
	name[0] = 0;
	MPI_Type_get_name( mpi_names[i].dtype, name, &namelen );
	if (strncmp( name, mpi_names[i].name, namelen )) {
	    errs++;
	    fprintf( stderr, "Expected %s but got %s\n", 
		     mpi_names[i].name, name );
	}
    }

    /* Try resetting the name */
    MPI_Type_set_name( MPI_INT, "int" );
    name[0] = 0;
    MPI_Type_get_name( MPI_INT, name, &namelen );
    if (strncmp( name, "int", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected int but got :%s:\n", name );
    }


    if (errs) {
	fprintf( stderr, "Found %d errors\n", errs );
    }
    else {
	printf( " No Errors\n" );
    }
    MPI_Finalize();
    return 0;
}
