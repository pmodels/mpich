/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Test of info that makes use of the extended handles */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifndef MAX_INFOS
#define MAX_INFOS 4000
#endif
#define info_list 16
/* #define DBG  */

int main( int arg, char *argv[] )
{
    MPI_Info infos[MAX_INFOS];
    char key[64], value[64];
    int  errs = 0;
    int  i, j;

    MPI_Init( 0, 0 );
    
    for (i=0; i<MAX_INFOS; i++) {
	MPI_Info_create( &infos[i] );
#ifdef DBG
	printf( "Info handle is %x\n", infos[i] );
#endif
	for (j=0; j<info_list; j++) {
	    sprintf( key, "key%d-%d", i, j );
	    sprintf( value, "value%d-%d", i, j );
#ifdef DBG
	    printf( "Creating key/value %s=%s\n", key, value );
#endif
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
    
    for (i=0; i<MAX_INFOS; i++) {
	int nkeys;
	/*printf( "info = %x\n", infos[i] );
	  print_handle( infos[i] ); printf( "\n" );*/
	MPI_Info_get_nkeys( infos[i], &nkeys );
	if (nkeys != info_list) {
	    errs++;
	    printf( "Wrong number of keys for info %d; got %d, should be %d\n",
		    i, nkeys, info_list );
	}
	for (j=0; j<nkeys; j++) {
	    char keystr[64];
	    char valstr[64];
	    int  flag;
	    MPI_Info_get_nthkey( infos[i], j, key );
	    sprintf( keystr, "key%d-%d", i, j );
	    if (strcmp( keystr, key ) != 0) {
		errs++;
		printf( "Wrong key for info %d; got %s expected %s\n", 
			i, key, keystr );
		continue;
	    }
	    MPI_Info_get( infos[i], key, 64, value, &flag );
	    if (!flag) {
		errs++;
		printf( "Get failed to return value for info %d\n", i );
		continue;
	    }
	    sprintf( valstr, "value%d-%d", i, j );
	    if (strcmp( valstr, value ) != 0) {
		errs++;
		printf( "Wrong value for info %d; got %s expected %s\n",
			i, value, valstr );
	    }
	}
    }
    for (i=0; i<MAX_INFOS; i++) {
	MPI_Info_free( &infos[i] );
    }
    if (errs) {
	printf( " Found %d errors\n", errs );
    }
    else {
	printf( " No Errors\n" );
    }
    
    MPI_Finalize( );
    return 0;
}
