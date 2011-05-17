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
static char MTEST_Descrip[] = "Simple intercomm alltoall test";
*/

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int *sendbuf = 0, *recvbuf = 0;
    int leftGroup, i, j, idx, count, rrank, rsize;
    MPI_Comm comm;
    MPI_Datatype datatype;

    MTest_Init( &argc, &argv );

    datatype = MPI_INT;
    while (MTestGetIntercomm( &comm, &leftGroup, 4 )) {
	if (comm == MPI_COMM_NULL) continue;
	for (count = 1; count < 66000; count = 2 * count) {
	    /* Get an intercommunicator */
	    MPI_Comm_remote_size( comm, &rsize );
	    MPI_Comm_rank( comm, &rrank );
	    sendbuf = (int *)malloc( rsize * count * sizeof(int) );
	    recvbuf = (int *)malloc( rsize * count * sizeof(int) );
	    for (i=0; i<rsize*count; i++) recvbuf[i] = -1;
	    if (leftGroup) {
		idx = 0;
		for (j=0; j<rsize; j++) {
		    for (i=0; i<count; i++) {
			sendbuf[idx++] = i + rrank;
		    }
		}
		err = MPI_Alltoall( sendbuf, count, datatype, 
				    NULL, 0, datatype, comm );
		if (err) {
		    errs++;
		    MTestPrintError( err );
		}
	    }
	    else {
		int rank, size;

		MPI_Comm_rank( comm, &rank );
		MPI_Comm_size( comm, &size );

		/* In the right group */
		err = MPI_Alltoall( NULL, 0, datatype, 
				    recvbuf, count, datatype, comm );
		if (err) {
		    errs++;
		    MTestPrintError( err );
		}
		/* Check that we have received the correct data */
		idx = 0;
		for (j=0; j<rsize; j++) {
		    for (i=0; i<count; i++) {
			if (recvbuf[idx++] != i + j) {
			    errs++;
			    if (errs < 10) 
				fprintf( stderr, "buf[%d] = %d on %d\n", 
					 i, recvbuf[i], rank );
			}
		    }
		}
	    }
	    free( recvbuf );
	    free( sendbuf );
	}
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
