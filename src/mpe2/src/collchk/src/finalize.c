/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include <stdio.h>
#include "mpi.h"

int MPI_Finalize( void )
{
    int  rank;

    PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if ( rank == 0 )
        fprintf( stdout, "Done with MPI Collective and Datatype Checking!\n" );
    return PMPI_Finalize();
}
