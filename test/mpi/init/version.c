/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int errs = 0;
    int majversion, subversion;

    MTest_Init( &argc, &argv );

    MPI_Get_version( &majversion, &subversion );
    if (majversion != MPI_VERSION) {
	errs++;
	printf( "Major version is %d but is %d in the mpi.h file\n", 
		majversion, MPI_VERSION );
    }
    if (subversion != MPI_SUBVERSION) {
	errs++;
	printf( "Minor version is %d but is %d in the mpi.h file\n", 
		subversion, MPI_SUBVERSION );
    }
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
