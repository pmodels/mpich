/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of various receive cancel calls, with multiple requests to cancel";
*/

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, source, dest;
    MPI_Comm      comm;
    MPI_Status    status;
    MPI_Request   req[4];
    static int bufsizes[4] = { 1, 100, 10000, 1000000 };
    char *bufs[4];
    int  flag, i;

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    source = 0;
    dest   = size - 1;

    if (rank == source) {
	MPI_Send( MPI_BOTTOM, 0, MPI_CHAR, dest, 1, MPI_COMM_WORLD );
    }
    else if (rank == dest) {
	/* Create 3 requests to cancel, plus one to use.  
	   Then receive one message and exit */ 
	for (i=0; i<4; i++) {
	    bufs[i] = (char *) malloc( bufsizes[i] );
	    MPI_Irecv( bufs[i], bufsizes[i], MPI_CHAR, source, 
		       i, MPI_COMM_WORLD, &req[i] );
	}
	/* Now, cancel them in a more interesting order, to ensure that the
	   queue operation work properly */
	MPI_Cancel( &req[2] );
	MPI_Wait( &req[2], &status );
	MTestPrintfMsg( 1, "Completed wait on irecv[2]\n" );
	MPI_Test_cancelled( &status, &flag );
	if (!flag) {
	    errs ++;
	    printf( "Failed to cancel a Irecv[2] request\n" );
	    fflush(stdout);
	}
	MPI_Cancel( &req[3] );
	MPI_Wait( &req[3], &status );
	MTestPrintfMsg( 1, "Completed wait on irecv[3]\n" );
	MPI_Test_cancelled( &status, &flag );
	if (!flag) {
	    errs ++;
	    printf( "Failed to cancel a Irecv[3] request\n" );
	    fflush(stdout);
	}
	MPI_Cancel( &req[0] );
	MPI_Wait( &req[0], &status );
	MTestPrintfMsg( 1, "Completed wait on irecv[0]\n" );
	MPI_Test_cancelled( &status, &flag );
	if (!flag) {
	    errs ++;
	    printf( "Failed to cancel a Irecv[0] request\n" );
	    fflush(stdout);
	}
	MPI_Wait( &req[1], &status );
	MPI_Test_cancelled( &status, &flag );
	if (flag) {
	    errs ++;
	    printf( "Incorrectly cancelled Irecv[1]\n" ); fflush(stdout);
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
