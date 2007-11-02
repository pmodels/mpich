/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

int main(int argc, char **argv )
{
    char *p;
    int errs = 0, toterrs;
    int size, rank;

    MPI_Init( &argc, &argv );
    
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if (size != 2) {
	errs++;
	printf( "Communicator size is %d, should be 2\n", size );
    }
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    p = getenv("TMP_ENV_VAR");
    if (!p) {
	errs++;
	printf( "Did not find TMP_ENV_VAR\n" );
    }
    else if (strcmp(p,"1") != 0) {
	errs++;
	printf( "Value of TMP_ENV_VAR was %s, expected 1\n", p );
    }

    MPI_Reduce( &errs, &toterrs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    if (rank == 0) {
	if (toterrs == 0) {
	    printf( " No Errors\n" );
	}
	else {
	    printf( " Found %d errors\n", toterrs );
	}
    }

    MPI_Finalize();
    return 0;
}
