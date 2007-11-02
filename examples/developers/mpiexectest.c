/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* simple test for multiple executables */
#include "mpi.h"
#include <stdio.h>
#ifdef HAVE_WINDOWS_H
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>

#define MAX_DIRNAME_SIZE 256 

int main( int argc, char *argv[], char *envp[] )
{
    int  i, myid, numprocs;
    int  namelen;
    char *p;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    char curr_wd[MAX_DIRNAME_SIZE];

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
    MPI_Comm_rank( MPI_COMM_WORLD, &myid );
    MPI_Get_processor_name( processor_name, &namelen );

    fprintf( stdout, "[%d] Process %d of %d (%s) is on %s\n",
	     myid, myid, numprocs, argv[0], processor_name );
    fflush( stdout );

    for ( i = 1; i < argc; i++ ) {
	fprintf( stdout, "[%d] argv[%d]=\"%s\"\n", myid, i, argv[i] ); 
	fflush( stdout );
    }

    getcwd( curr_wd, MAX_DIRNAME_SIZE ); 
    fprintf( stdout, "[%d] current working directory=%s\n", myid, curr_wd );

    p = getenv("PATH");
    if ( p )
	fprintf( stdout, "[%d] PATH=%s\n", myid, p );
    else
	fprintf( stdout, "[%d] PATH not set in environment\n", myid );

#ifdef PRINTENV
    /* may produce lots of output, but here if you need it */
    for ( i = 0; envp[i]; i++ )
	fprintf( stdout, "[%d] envp[%d]=%s\n", myid, i, envp[i] );
#endif

    MPI_Finalize( );
}
