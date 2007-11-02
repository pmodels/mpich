/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* Check that freed communicators are detected */
int main( int argc, char *argv[] )
{
    MPI_Comm dup, dupcopy;
    int errs = 0;
    int ierr;
    
    MTest_Init( &argc, &argv );

    MPI_Comm_dup( MPI_COMM_WORLD, &dup );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    dupcopy = dup;
    MPI_Comm_free( &dupcopy );
    ierr = MPI_Barrier( dup );
    if (ierr == MPI_SUCCESS) {
	errs ++;
	printf( "Returned wrong code (SUCCESS) in barrier\n" );
    }

    {
	int in, *input = &in;
	int out, *output = &out;
	ierr = MPI_Allgather(input, 1, MPI_INT, output, 1, MPI_INT, dup);
    }
    if (ierr == MPI_SUCCESS) {
	errs ++;
	printf( "Returned wrong code (SUCCESS) in allgather\n" );
    }

    MTest_Finalize( errs );

    MPI_Finalize();
    return 0;
}
