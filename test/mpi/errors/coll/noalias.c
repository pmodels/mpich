/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int        err, errs = 0, len;
    int        buf[1], rank;
    char       msg[MPI_MAX_ERROR_STRING];

    MTest_Init( &argc, &argv );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    buf[0] = 1;
    err = MPI_Allreduce( buf, buf, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (!err) {
	errs++;
	if (rank == 0) 
	    printf( "Did not detect aliased arguments in MPI_Allreduce\n" );
    }
    else {
	/* Check that we can get a message for this error */
	/* (This works if it does not SEGV or hang) */
	MPI_Error_string( err, msg, &len );
    }
    err = MPI_Reduce( buf, buf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    if (!err) {
	errs++;
	if (rank == 0)
	    printf( "Did not detect aliased arguments in MPI_Reduce\n" );
    }
    else {
	/* Check that we can get a message for this error */
	/* (This works if it does not SEGV or hang) */
	MPI_Error_string( err, msg, &len );
    }

    MTest_Finalize( errs );
    MPI_Finalize( );
    return 0;
}
