/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This simple test ignores stdin and exits.  This is used to test the
   handling of stdin by programs that do not read all of the stdin 
   data that they may have */
#include <stdio.h>
#include "mpi.h"

/* style: allow:printf:1 sig:0 */

int main( int argc, char *argv[] )
{
    int rank;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if (rank == 0) {
	printf( " No Errors\n" );
    }
    MPI_Finalize();

    return 0;
}
