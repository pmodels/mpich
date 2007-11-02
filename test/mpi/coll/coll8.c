/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>

int main( int argc, char **argv )
{
    int              rank, size, i;
    int              data;
    int              errors=0;
    int              result = -100;
    int              correct_result;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    data = rank;

    MPI_Reduce ( &data, &result, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    MPI_Bcast  ( &result, 1, MPI_INT, 0, MPI_COMM_WORLD );
    correct_result = 0;
    for(i=0;i<size;i++) 
      correct_result += i;
    if (result != correct_result) errors++;

    MPI_Reduce ( &data, &result, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD );
    MPI_Bcast  ( &result, 1, MPI_INT, 0, MPI_COMM_WORLD );
    if (result != 0) errors++;

    MPI_Reduce ( &data, &result, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD );
    MPI_Bcast  ( &result, 1, MPI_INT, 0, MPI_COMM_WORLD );
    if (result != (size-1)) errors++;

    MPI_Finalize();
    if (errors)
      printf( "[%d] done with ERRORS(%d)!\n", rank, errors );
    else {
	if (rank == 0) 
	    printf(" No Errors\n");
    }
    return errors;
}
