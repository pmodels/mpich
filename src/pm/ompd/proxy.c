/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <unistd.h>
#define MAXHOSTNAMELEN 128

int main( int argc, char * argv[] )
{
    int i;
    size_t len;
    char myhostname[MAXHOSTNAMELEN];

    gethostname( myhostname, MAXHOSTNAMELEN );
    printf( "On %s:  ", myhostname );
    for ( i = 0; i < argc; i++ )
	printf( "%s ", argv[i] );
    printf( "\n" );
    
    return( 0 );
}
