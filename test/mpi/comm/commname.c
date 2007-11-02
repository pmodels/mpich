/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
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
    MPI_Comm comm;
    int cnt, rlen;
    char name[MPI_MAX_OBJECT_NAME], nameout[MPI_MAX_OBJECT_NAME];
    MTest_Init( &argc, &argv );

    /* Check world and self firt */
    nameout[0] = 0;
    MPI_Comm_get_name( MPI_COMM_WORLD, nameout, &rlen );
    if (strcmp(nameout,"MPI_COMM_WORLD")) {
	errs++;
	printf( "Name of comm world is %s, should be MPI_COMM_WORLD\n", 
		nameout );
    }

    nameout[0] = 0;
    MPI_Comm_get_name( MPI_COMM_SELF, nameout, &rlen );
    if (strcmp(nameout,"MPI_COMM_SELF")) {
	errs++;
	printf( "Name of comm self is %s, should be MPI_COMM_SELF\n", 
		nameout );
    }

    /* Now, handle other communicators, including world/self */
    cnt = 0;
    while (MTestGetComm( &comm, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
    
	sprintf( name, "comm-%d", cnt );
	cnt++;
	MPI_Comm_set_name( comm, name );
	nameout[0] = 0;
	MPI_Comm_get_name( comm, nameout, &rlen );
	if (strcmp( name, nameout )) {
	    errs++;
	    printf( "Unexpected name, was %s but should be %s\n",
		    nameout, name );
	}
	
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
