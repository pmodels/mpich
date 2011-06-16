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

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI_Info info1, infodup;
    int nkeys, nkeysdup, i, vallen, flag, flagdup;
    char key[MPI_MAX_INFO_KEY], keydup[MPI_MAX_INFO_KEY];
    char value[MPI_MAX_INFO_VAL], valdup[MPI_MAX_INFO_VAL];

    MTest_Init( &argc, &argv );

    MPI_Info_create( &info1 );
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    MPI_Info_set( info1, (char*)"host", (char*)"myhost.myorg.org" );
    MPI_Info_set( info1, (char*)"file", (char*)"runfile.txt" );
    MPI_Info_set( info1, (char*)"soft", (char*)"2:1000:4,3:1000:7" );

    MPI_Info_dup( info1, &infodup );

    MPI_Info_get_nkeys( infodup, &nkeysdup );
    MPI_Info_get_nkeys( info1, &nkeys );
    if (nkeys != nkeysdup) {
	errs++;
	printf( "Dup'ed info has a different number of keys; is %d should be %d\n",
		nkeysdup, nkeys );
    }
    vallen = MPI_MAX_INFO_VAL;
    for (i=0; i<nkeys; i++) {
	/* MPI requires that the keys are in the same order after the dup */
	MPI_Info_get_nthkey( info1, i, key );
	MPI_Info_get_nthkey( infodup, i, keydup );
	if (strcmp(key, keydup)) {
	    errs++;
	    printf( "keys do not match: %s should be %s\n", keydup, key );
	}

	vallen = MPI_MAX_INFO_VAL;
	MPI_Info_get( info1, key, vallen, value, &flag );
	MPI_Info_get( infodup, keydup, vallen, valdup, &flagdup );
	if (!flag || !flagdup) {
	    errs++;
	    printf( "Info get failed for key %s\n", key );
	}
	else if (strcmp( value, valdup )) {
	    errs++;
	    printf( "Info values for key %s not the same after dup\n", key );
	}
    }

    /* Change info and check that infodup does NOT have the new value 
       (ensure that lazy dups are still duped) */
    MPI_Info_set( info1, (char*)"path", (char*)"/a:/b:/c/d" );

    MPI_Info_get( infodup, (char*)"path", vallen, value, &flag );
    if (flag) {
	errs++;
	printf( "inserting path into info changed infodup\n" );
    }
    
    MPI_Info_free( &info1 );
    MPI_Info_free( &infodup );
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
