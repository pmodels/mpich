/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

#define NKEYS 3
int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI_Info info;
    char *keys[NKEYS] = { (char*)"file", (char*)"soft", (char*)"host" };
    char *values[NKEYS] = { (char*)"runfile.txt", (char*)"2:1000:4,3:1000:7", 
			    (char*)"myhost.myorg.org" };
    char value[MPI_MAX_INFO_VAL];
    int i, flag, vallen;

    MTest_Init( &argc, &argv );

    MPI_Info_create( &info );
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    for (i=0; i<NKEYS; i++) {
	MPI_Info_set( info, keys[i], values[i] );
    }

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	MPI_Info_get_valuelen( info, keys[i], &vallen, &flag );
	if (!flag) {
	    errs++;
	    printf( "get_valuelen failed for valid key %s\n", keys[i] );
	}
	MPI_Info_get( info, keys[i], MPI_MAX_INFO_VAL, value, &flag );
	if (!flag) {
	    errs++;
	    printf( "No value for key %s\n", keys[i] );
	}
	if (strcmp( value, values[i] )) {
	    errs++;
	    printf( "Incorrect value for key %s\n", keys[i] );
	}
	if (strlen(value) != vallen) {
	    errs++;
	    printf( "value_len returned %d but actual len is %d\n", 
		    vallen, (int) strlen(value) );
	}
    }

    MPI_Info_free( &info );
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
