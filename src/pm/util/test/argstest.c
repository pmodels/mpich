/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "process.h"

/* 
   Test the argument parsing and processing routines.  See the Makefile
   for typical test choices 
*/
int main( int argc, char *argv[] )
{
    MPIE_ProcessInit();
    MPIE_Args( argc, argv, &pUniv, 0, 0 );
    MPIE_PrintProcessUniverse( stdout, &pUniv );
    return 0;
}

int mpiexec_usage( const char *str )
{
    fprintf( stderr, "Usage: %s\n", str );
    return 0;
}
