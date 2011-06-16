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
static char MTEST_Descrip[] = "Put with Fence";
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

		MTestPrintfMsg( 1, 
		       "Putting count = %d of sendtype %s receive type %s\n", 
				count, MTestGetDatatypeName( &sendtype ),
				MTestGetDatatypeName( &recvtype ) );

		/* Make sure that everyone has a recv buffer */
		recvtype.InitBuf( &recvtype );

		MPI_Type_extent( recvtype.datatype, &extent );
		MPI_Win_create( recvtype.buf, recvtype.count * extent, 
				extent, MPI_INFO_NULL, comm, &win );
		MPI_Win_fence( 0, win );
		if (rank == source) {
		    /* To improve reporting of problems about operations, we
		       change the error handler to errors return */
		    MPI_Win_set_errhandler( win, MPI_ERRORS_RETURN );

		    sendtype.InitBuf( &sendtype );
		    
		    err = MPI_Put( sendtype.buf, sendtype.count, 
				   sendtype.datatype, dest, 0, 
				   recvtype.count, recvtype.datatype, win );
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
		}
		else if (rank == dest) {
		    MPI_Win_fence( 0, win );
		    /* This should have the same effect, in terms of
		       transfering data, as a send/recv pair */
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) {
			if (errs < 10) {
			    printf( "Data in target buffer did not match for destination datatype %s (put with source datatype %s)\n", 
				    MTestGetDatatypeName( &recvtype ),
				    MTestGetDatatypeName( &sendtype ) );
			    /* Redo the test, with the errors printed */
			    recvtype.printErrors = 1;
			    (void)MTestCheckRecv( 0, &recvtype );
			}
			errs += err;
		    }
		}
		else {
		    MPI_Win_fence( 0, win );
		}
		MPI_Win_free( &win );
		MTestFreeDatatype( &sendtype );
		MTestFreeDatatype( &recvtype );
	    }
	}
        MTestFreeComm(&comm);
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
