/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"

/* 
 * This is a special test to check that mpiexec handles zero/non-zero 
 * return status from an application.  In this case, each process 
 * returns a different return status
 */
int main( int argc, char *argv[] )
{
    int rank;
    MPI_Init( 0, 0 );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Finalize( );
    return rank;
}
