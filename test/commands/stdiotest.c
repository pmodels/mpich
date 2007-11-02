/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int rank;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if (rank == 0) {
	fprintf( stdout, "This is stdout\n" );
	fprintf( stderr, "This is stderr\n" );
    }
    MPI_Finalize();

    return 0;
}
