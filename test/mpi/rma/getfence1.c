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
static char MTEST_Descrip[] = "Get with Fence";
*/

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, count; 
    MPI_Comm      comm;
    MPI_Win       win;
    MPI_Aint      extent;
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
	
	for (count = 1; count < 65000; count = count * 2) {
	    while (MTestGetDatatypes( &sendtype, &recvtype, count )) {
		/* Make sure that everyone has a recv buffer */
		recvtype.InitBuf( &recvtype );
		sendtype.InitBuf( &sendtype );
		/* By default, print information about errors */
		recvtype.printErrors = 1;
		sendtype.printErrors = 1;

		MPI_Type_extent( sendtype.datatype, &extent );
		MPI_Win_create( sendtype.buf, sendtype.count * extent, 
				(int)extent, MPI_INFO_NULL, comm, &win );
		MPI_Win_fence( 0, win );
		if (rank == source) {
		    /* The source does not need to do anything besides the
		       fence */
		    MPI_Win_fence( 0, win );
		}
		else if (rank == dest) {
		    /* To improve reporting of problems about operations, we
		       change the error handler to errors return */
		    MPI_Win_set_errhandler( win, MPI_ERRORS_RETURN );

		    /* This should have the same effect, in terms of
		       transfering data, as a send/recv pair */
		    err = MPI_Get( recvtype.buf, recvtype.count, 
				   recvtype.datatype, source, 0, 
				   sendtype.count, sendtype.datatype, win );
		    if (err) {
			errs++;
			if (errs < 10) {
			    MTestPrintError( err );
			}
		    }
		    err = MPI_Win_fence( 0, win );
		    if (err) {
			errs++;
			if (errs < 10) {
			    MTestPrintError( err );
			}
		    }
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) {
			errs += err;
		    }
		}
		else {
		    MPI_Win_fence( 0, win );
		}
		MPI_Win_free( &win );
		MTestFreeDatatype( &recvtype );
		MTestFreeDatatype( &sendtype );
	    }
	}
        MTestFreeComm(&comm);
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
