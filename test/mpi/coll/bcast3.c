/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of broadcast with various roots and datatypes and sizes that are not powers of two";
*/

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, root;
    int minsize = 2, count; 
    MPI_Comm      comm;
    MTestDatatype sendtype, recvtype;

    MTest_Init( &argc, &argv );

    /* The following illustrates the use of the routines to 
       run through a selection of communicators and datatypes.
       Use subsets of these for tests that do not involve combinations 
       of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
	/* Determine the sender and receiver */
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_size( comm, &size );
	
	count = 1;
	/* This must be very large to ensure that we reach the long message
	   algorithms */
	for (count = 4; count < 66000; count = count * 4) {
	    while (MTestGetDatatypes( &sendtype, &recvtype, count-1 )) {
		for (root=0; root<size; root++) {
		    if (rank == root) {
			sendtype.InitBuf( &sendtype );
			err = MPI_Bcast( sendtype.buf, sendtype.count,
					 sendtype.datatype, root, comm );
			if (err) {
			    errs++;
			    MTestPrintError( err );
			}
		    }
		    else {
			recvtype.InitBuf( &recvtype );
			err = MPI_Bcast( recvtype.buf, recvtype.count, 
				    recvtype.datatype, root, comm );
			if (err) {
			    errs++;
			    fprintf( stderr, "Error with communicator %s and datatype %s\n", 
				 MTestGetIntracommName(), 
				 MTestGetDatatypeName( &recvtype ) );
			    MTestPrintError( err );
			}
			err = MTestCheckRecv( 0, &recvtype );
			if (err) {
			    errs += errs;
			}
		    }
		}
		MTestFreeDatatype( &recvtype );
		MTestFreeDatatype( &sendtype );
	    }
	}
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
