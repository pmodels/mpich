/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Test of info that makes use of the extended handles, including
   inserts and deletes */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifndef MAX_INFOS
#define MAX_INFOS 4000
#endif
#define MAX_ERRORS 10
#define info_list 16
/* #define DBG  */

#ifdef DEBUG
#define DBGPRINTF(a) printf a; fflush(stdout)
#else
#define DBGPRINTF(a)
#endif

int main( int argc, char *argv[] )
{
    MPI_Info infos[MAX_INFOS];
    char key[64], value[64];
    int  errs = 0;
    int  i, j;

    MTest_Init( &argc, &argv );

    /* We create max_info items, then delete the middle third of them,
       then recreate them, then check them, then 
       delete them all.  This checks that the MPICH2 algorithm for 
       handling large numbers of items works correctly; other MPI 
       implementations should also be able to handle this */

    /* Create them all */
    for (i=0; i<MAX_INFOS; i++) {
	MPI_Info_create( &infos[i] );
	DBGPRINTF( ( "Info handle is %x\n", infos[i] ) );
	for (j=0; j<info_list; j++) {
	    sprintf( key, "key%d-%d", i, j );
	    sprintf( value, "value%d-%d", i, j );
	    DBGPRINTF( ( "Creating key/value %s=%s\n", key, value ));
	    MPI_Info_set( infos[i], key, value );
	}
#ifdef DBG
	{ int nkeys;
	MPI_Info_get_nkeys( infos[0], &nkeys );
	if (nkeys != info_list) {
	    printf( "infos[0] changed at %d info\n", i );}
	}
#endif
    }

    /* Delete the middle set */
    for (i=MAX_INFOS/3; i<(2*MAX_INFOS/3); i++) {
	MPI_Info_free( &infos[i] );
    }
    
    /* Recreate the middle set */
    for (i=MAX_INFOS/3; i<(2*MAX_INFOS/3); i++) {
	MPI_Info_create( &infos[i] );
	DBGPRINTF( ( "Info handle is %x\n", infos[i] ) );
	for (j=0; j<info_list; j++) {
	    sprintf( key, "key%d-%d", i, j );
	    sprintf( value, "value%d-%d", i, j );
	    DBGPRINTF( ( "Creating key/value %s=%s\n", key, value ));
	    MPI_Info_set( infos[i], key, value );
	}
    }

    /* Now, check that they are still valid */
    for (i=0; i<MAX_INFOS; i++) {
	int nkeys;
	/*printf( "info = %x\n", infos[i] );
	  print_handle( infos[i] ); printf( "\n" );*/
	MPI_Info_get_nkeys( infos[i], &nkeys );
	if (nkeys != info_list) {
	    errs++;
	    if (errs < MAX_ERRORS) {
		printf( "Wrong number of keys for info %d; got %d, should be %d\n",
			i, nkeys, info_list );
	    }
	}
	for (j=0; j<nkeys; j++) {
	    char keystr[64];
	    char valstr[64];
	    int  flag;
	    MPI_Info_get_nthkey( infos[i], j, key );
	    sprintf( keystr, "key%d-%d", i, j );
	    if (strcmp( keystr, key ) != 0) {
		errs++;
		if (errs < MAX_ERRORS) {
		    printf( "Wrong key for info %d; got %s expected %s\n", 
			    i, key, keystr );
		}
		continue;
	    }
	    MPI_Info_get( infos[i], key, 64, value, &flag );
	    if (!flag) {
		errs++;
		if (errs < MAX_ERRORS) {
		    printf( "Get failed to return value for info %d\n", i );
		}
		continue;
	    }
	    sprintf( valstr, "value%d-%d", i, j );
	    if (strcmp( valstr, value ) != 0) {
		errs++;
		if (errs < MAX_ERRORS) {
		    printf( "Wrong value for info %d; got %s expected %s\n",
			    i, value, valstr );
		}
	    }
	}
    }
    for (i=0; i<MAX_INFOS; i++) {
	MPI_Info_free( &infos[i] );
    }
    
    MTest_Finalize( errs );
    MPI_Finalize( );
    return 0;
}
