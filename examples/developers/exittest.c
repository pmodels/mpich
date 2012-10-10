/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    int numprocs, myid, i;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
    MPI_Comm_rank( MPI_COMM_WORLD, &myid );
    MPI_Get_processor_name( processor_name, &namelen );

    fprintf( stdout,"Process %d of %d on %s\n",
            myid, numprocs, processor_name );
    sleep( 1 );
    fprintf( stderr, "Process %d exiting with exit code %d\n", myid, -myid );
    MPI_Finalize( );
    fprintf( stdout,"out: Process %d after finalize\n", myid); fflush(stdout);
    fprintf( stderr,"err: Process %d after finalize\n", myid); fflush(stderr);
    return( -myid );
}
