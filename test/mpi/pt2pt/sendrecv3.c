/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Head to head send-recv to test backoff in device when large messages are being transferred";
*/

#define  MAX_NMSGS 100
int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, source, dest, partner;
    int i, testnum; 
    double tsend;
    static int msgsizes[] = { 100, 1000, 10000, 100000, -1 };
    static int nmsgs[]    = { 100, 10,   10,    4 };
    MPI_Comm      comm;

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );
    source = 0;
    dest   = 1;
    if (size < 2) {
	printf( "This test requires at least 2 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    for (testnum=0; msgsizes[testnum] > 0; testnum++) {
	if (rank == source || rank == dest) {
	    int nmsg = nmsgs[testnum];
	    int msgSize = msgsizes[testnum];
	    MPI_Request r[MAX_NMSGS];
	    int *buf[MAX_NMSGS];

	    for (i=0; i<nmsg; i++) {
		buf[i] = (int *)malloc( msgSize );
	    }
	    partner = (rank + 1) % size;

	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, partner, 10, 
			  MPI_BOTTOM, 0, MPI_INT, partner, 10, comm, 
			  MPI_STATUS_IGNORE );
	    /* Try to fill up the outgoing message buffers */
	    for (i=0; i<nmsg; i++) {
		MPI_Isend( buf[i], msgSize, MPI_CHAR, partner, testnum, comm,
			   &r[i] );
	    }
	    for (i=0; i<nmsg; i++) {
		MPI_Recv( buf[i], msgSize, MPI_CHAR, partner, testnum, comm,
			  MPI_STATUS_IGNORE );
	    }
	    MPI_Waitall( nmsg, r, MPI_STATUSES_IGNORE );

	    /* Repeat the test, but make one of the processes sleep */
	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, partner, 10, 
			  MPI_BOTTOM, 0, MPI_INT, partner, 10, comm, 
			  MPI_STATUS_IGNORE );
	    if (rank == dest) MTestSleep( 1 );
	    /* Try to fill up the outgoing message buffers */
	    tsend = MPI_Wtime();
	    for (i=0; i<nmsg; i++) {
		MPI_Isend( buf[i], msgSize, MPI_CHAR, partner, testnum, comm,
			   &r[i] );
	    }
	    tsend = MPI_Wtime() - tsend;
	    for (i=0; i<nmsg; i++) {
		MPI_Recv( buf[i], msgSize, MPI_CHAR, partner, testnum, comm,
			  MPI_STATUS_IGNORE );
	    }
	    MPI_Waitall( nmsg, r, MPI_STATUSES_IGNORE );

	    if (tsend > 0.5) {
		printf( "Isends for %d messages of size %d took too long (%f seconds)\n", nmsg, msgSize, tsend );
		errs++;
	    }
	    MTestPrintfMsg( 1, "%d Isends for size = %d took %f seconds\n", 
			    nmsg, msgSize, tsend );

	    for (i=0; i<nmsg; i++) {
		free( buf[i] );
	    }
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
