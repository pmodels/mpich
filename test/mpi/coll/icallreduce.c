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
static char MTEST_Descrip[] = "Simple intercomm allreduce test";
*/

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int *sendbuf = 0, *recvbuf = 0;
    int leftGroup, i, count, rank, rsize;
    MPI_Comm comm;
    MPI_Datatype datatype;

    MTest_Init( &argc, &argv );

    datatype = MPI_INT;
    /* Get an intercommunicator */
    while (MTestGetIntercomm( &comm, &leftGroup, 4 )) {
	if (comm == MPI_COMM_NULL) continue;
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_remote_size( comm, &rsize );

	/* To improve reporting of problems about operations, we
	   change the error handler to errors return */
	MPI_Errhandler_set( comm, MPI_ERRORS_RETURN );

	for (count = 1; count < 65000; count = 2 * count) {
	    /* printf( "rank = %d(%d)\n", rank, leftGroup ); fflush(stdout); */
	    sendbuf = (int *)malloc( count * sizeof(int) );
	    recvbuf = (int *)malloc( count * sizeof(int) );
	    if (leftGroup) {
		for (i=0; i<count; i++) sendbuf[i] = i;
	    }
	    else {
		for (i=0; i<count; i++) sendbuf[i] = -i;
	    }
	    for (i=0; i<count; i++) recvbuf[i] = 0;
	    err = MPI_Allreduce( sendbuf, recvbuf, count, datatype, 
				 MPI_SUM, comm );
	    if (err) {
		errs++;
		MTestPrintError( err );
	    }
	    /* In each process should be the sum of the values from the
	       other process */
	    if (leftGroup) {
		for (i=0; i<count; i++) {
		    if (recvbuf[i] != -i * rsize) {
			errs++;
			if (errs < 10) {
			    fprintf( stderr, "recvbuf[%d] = %d\n", i, recvbuf[i] );
			}
		    }
		}
	    }
	    else {
		for (i=0; i<count; i++) {
		    if (recvbuf[i] != i * rsize) {
			errs++;
			if (errs < 10) {
			    fprintf( stderr, "recvbuf[%d] = %d\n", i, recvbuf[i] );
			}
		    }
		}
	    }
            free( sendbuf );
            free( recvbuf );
	}
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
