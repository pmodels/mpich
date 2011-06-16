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
static char MTEST_Descrip[] = "Simple intercomm scatterv test";
*/

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int *buf = 0;
    int *sendcounts;
    int *senddispls;
    int leftGroup, i, count, rank, rsize, size;
    MPI_Comm comm;
    MPI_Datatype datatype;

    MTest_Init( &argc, &argv );

    datatype = MPI_INT;
    /* Get an intercommunicator */
    while (MTestGetIntercomm( &comm, &leftGroup, 4 )) {
	if (comm == MPI_COMM_NULL) continue;
	MPI_Comm_remote_size( comm, &rsize );
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_size( comm, &size );

	/* To improve reporting of problems about operations, we
	   change the error handler to errors return */
	MPI_Comm_set_errhandler( comm, MPI_ERRORS_RETURN );

	for (count = 1; count < 65000; count = 2 * count) {
	    buf = 0;
	    sendcounts = (int *)malloc( rsize * sizeof(int) );
	    senddispls = (int *)malloc( rsize * sizeof(int) );
	    for (i=0; i<rsize; i++) {
		sendcounts[i] = count;
		senddispls[i] = count * i;
	    }
	    if (leftGroup) {
		buf = (int *)malloc( count * rsize * sizeof(int) );
		if (rank == 0) {
		    for (i=0; i<count*rsize; i++) buf[i] = i;
		}
		else {
		    for (i=0; i<count*rsize; i++) buf[i] = -1;
		}
		err = MPI_Scatterv( buf, sendcounts, senddispls, datatype, 
				    NULL, 0, datatype,
				    (rank == 0) ? MPI_ROOT : MPI_PROC_NULL,
				    comm );
		if (err) {
		    errs++;
		    MTestPrintError( err );
		}
		/* Test that no other process in this group received the 
		   scatter */
		if (rank != 0) {
		    for (i=0; i<count*rsize; i++) {
			if (buf[i] != -1) {
			    if (errs < 10) {
				fprintf( stderr, "Received data on root group!\n" );
			    }
			    errs++;
			}
		    }
		}
	    }
	    else {
		buf = (int *)malloc( count * sizeof(int) );
		/* In the right group */
		for (i=0; i<count; i++) buf[i] = -1;
		err = MPI_Scatterv( NULL, 0, 0, datatype, 
				    buf, count, datatype, 0, comm );
		if (err) {
		    errs++;
		    MTestPrintError( err );
		}
		/* Check that we have received the correct data */
		for (i=0; i<count; i++) {
		    if (buf[i] != i + rank * count) {
			if (errs < 10) 
			    fprintf( stderr, "buf[%d] = %d on %d\n", 
				     i, buf[i], rank );
			errs++;
		    }
		}
	    }
	    free( sendcounts );
	    free( senddispls );
	    free( buf );
	}
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
