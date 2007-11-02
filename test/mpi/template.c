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

static char MTEST_Descrip[] = "";

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, count; 
    MPI_Comm      comm;
    MPI_Status    status;
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
	source = 0;
	dest   = size - 1;

	/* To improve reporting of problems about operations, we
	   change the error handler to errors return */
	MPI_Comm_set_errhandler( comm, MPI_ERRORS_RETURN );
	
	for (count = 1; count < 65000; count = count * 2) {
	    while (MTestGetDatatypes( &sendtype, &recvtype, count )) {
		if (rank == source) {
		    sendtype.InitBuf( &sendtype );
		    
		    err = MPI_Send( sendtype.buf, sendtype.count, 
				    sendtype.datatype, dest, 0, comm );
		    if (err) {
			errs++;
			MTestPrintError( err );
		    }
		    MTestFreeDatatype( &sendtype );
		}
		else if (rank == dest) {
		    recvtype.InitBuf( &recvtype );
		    err = MPI_Recv( recvtype.buf, recvtype.count, 
				    recvtype.datatype, source, 0, comm, &status );
		    if (err) {
			errs++;
			fprintf( stderr, "Error with communicator %s and datatype %s\n", 
				 MTestGetIntracommName(), 
				 MTestGetDatatypeName( &recvtype ) );
			MTestPrintError( err );
		    }
		    err = MTestCheckRecv( &status, &recvtype );
		    if (err) {
			errs += errs;
		    }
		    MTestFreeDatatype( &recvtype );
		}
	    }
	}
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
