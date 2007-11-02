/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

#define BUFSIZE 2000
int main( int argc, char *argv[] )
{
    MPI_Status status;
    MPI_Request request;
    int a[10], b[10];
    int buf[BUFSIZE], *bptr, bl, i, j, rank, size;
    int errs = 0;

    MTest_Init( 0, 0 );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Buffer_attach( buf, BUFSIZE );

    for (j=0; j<10; j++) {
	MPI_Bsend_init( a, 10, MPI_INT, 0, 27+j, MPI_COMM_WORLD, &request );
	for (i=0; i<10; i++) {
	    a[i] = (rank + 10 * j) * size + i;
	}
	MPI_Start( &request );
	MPI_Wait( &request, &status );
	MPI_Request_free( &request );
    }
    if (rank == 0) {

	for (i=0; i<size; i++) {
	    for (j=0; j<10; j++) {
		int k;
		status.MPI_TAG = -10;
		status.MPI_SOURCE = -20;
		MPI_Recv( b, 10, MPI_INT, i, 27+j, MPI_COMM_WORLD, &status );
    
		if (status.MPI_TAG != 27+j) {
		    errs++;
		    printf( "Wrong tag = %d\n", status.MPI_TAG );
		}
		if (status.MPI_SOURCE != i) {
		    errs++;
		    printf( "Wrong source = %d\n", status.MPI_SOURCE );
		}
		for (k=0; k<10; k++) {
		    if (b[k] != (i + 10 * j) * size + k) {
			errs++;
			printf( "received b[%d] = %d from %d tag %d\n",
				k, b[k], i, 27+j );
		    }
		}
	    }
	}
    }
    MPI_Buffer_detach( &bptr, &bl );
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
