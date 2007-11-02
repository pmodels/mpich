/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <unistd.h>
/* MAXHOSTNAMELEN may be defined in sys/param.h (Linux, OSX) or 
   netdb.h (Solaris); a typical value is 256 */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

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
