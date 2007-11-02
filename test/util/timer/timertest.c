/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpiimpl.h"


int main( int argc, char **argv )
{
    double t1, t2;
    double tick;
    MPID_Time_t l1,l2; 
    int    i;

    MPI_Init( &argc, &argv );
    t1 = MPI_Wtime();
    t2 = MPI_Wtime();
    fprintf( stdout, 
             "Two successive calls to MPI_Wtime gave: (%f) (%f) diff (%f)\n", 
		t1, t2, t2-t1 );
    MPID_Wtime( &l1 );
    MPID_Wtime( &l2 );
    MPID_Wtime_diff( &l1, &l2, &t1 );
    fprintf( stdout, 
             "Two successive calls to MPID_Wtime gave: (%llx) (%llx) diff (%f)\n", 
		l1, l2, t1 );
    fprintf( stdout, "Five approximations to one second:\n");
    for (i = 0; i < 5; i++)
    {
	t1 = MPI_Wtime();
	sleep(1);
	t2 = MPI_Wtime();
	fprintf( stdout, "%f seconds\n", t2 - t1 );
    } 
/*     tick = MPI_Wtick();*/
    fprintf( stdout, "MPI_Wtick gave: (%10.8f)\n", tick );

    MPI_Finalize( );

    return 0;
}
