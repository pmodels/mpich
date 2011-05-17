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
static char MTEST_Descrip[] = "Test MPI_Exscan (simple test)";
*/

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size;
    int sendbuf[1], recvbuf[1];
    MPI_Comm      comm;

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );
    
    sendbuf[0] = rank;
    recvbuf[0] = -2;
	    
    MPI_Exscan( sendbuf, recvbuf, 1, MPI_INT, MPI_SUM, comm );

    /* Check the results.  rank 0 has no data.  Input is
       0  1  2  3  4  5  6  7  8 ...
       Output is
       -  0  1  3  6 10 15 21 28 36
       (scan, not counting the contribution from the calling process)
    */
    if (rank > 0) {
	int result = (((rank) * (rank-1))/2);
	/* printf( "%d: %d\n", rank, result ); */
	if (recvbuf[0] != result) {
	    errs++;
	    fprintf( stderr, "Error in recvbuf = %d on %d, expected %d\n",
			 recvbuf[0], rank, result );
	}
    }
    else if (recvbuf[0] != -2) {
	errs++;
	fprintf( stderr, "Error in recvbuf on zero, is %d\n", recvbuf[0] );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
