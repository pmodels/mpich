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
    int provided, flag, claimed;

    /* MTest_Init( &argc, &argv ); */

    MPI_Init_thread( 0, 0, MPI_THREAD_MULTIPLE, &provided );
    
    MPI_Is_thread_main( &flag );
    if (!flag) {
	errs++;
	printf( "This thread called init_thread but Is_thread_main gave false\n" );
    }
    MPI_Query_thread( &claimed );
    if (claimed != provided) {
	errs++;
	printf( "Query thread gave thread level %d but Init_thread gave %d\n",
		claimed, provided );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
