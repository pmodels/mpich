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

#define NKEYS 3
int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI_Info info;
    char *keys[NKEYS] = { (char*)"file", (char*)"soft", (char*)"host" };
    char *values[NKEYS] = { (char*)"runfile.txt", (char*)"2:1000:4,3:1000:7", 
			    (char*)"myhost.myorg.org" };
    char value[MPI_MAX_INFO_VAL];
    int i, flag, nkeys;

    MTest_Init( &argc, &argv );

    MPI_Info_create( &info );
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    for (i=0; i<NKEYS; i++) {
	MPI_Info_set( info, keys[i], values[i] );
    }

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) { 
	MPI_Info_get( info, keys[i], MPI_MAX_INFO_VAL, value, &flag );
	if (!flag) {
	    errs++;
	    printf( "No value for key %s\n", keys[i] );
	}
	if (strcmp( value, values[i] )) {
	    errs++;
	    printf( "Incorrect value for key %s, got %s expected %s\n", 
		    keys[i], value, values[i] );
	}
    }

    /* Now, change one value and remove another, then check again */
    MPI_Info_delete( info, keys[NKEYS-1] );
    MPI_Info_get_nkeys( info, &nkeys );
    if (nkeys != NKEYS - 1) {
	errs++;
	printf( "Deleting a key did not change the number of keys\n" );
    }

    values[0] = (char*)"backfile.txt";
    MPI_Info_set( info, keys[0], values[0] );
    for (i=0; i<NKEYS-1; i++) {
	MPI_Info_get( info, keys[i], MPI_MAX_INFO_VAL, value, &flag );
	if (!flag) {
	    errs++;
	    printf( "(after reset) No value for key %s\n", keys[i] );
	}
	if (strcmp( value, values[i] )) {
	    errs++;
	    printf( "(after reset) Incorrect value for key %s, got %s expected %s\n", 
		    keys[i], value, values[i] );
	}
    }

    MPI_Info_free( &info );
    if (info != MPI_INFO_NULL) {
	errs++;
	printf( "MPI_Info_free should set info to MPI_INFO_NULL\n" );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
