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
static char MTEST_Descrip[] = "Put with Post/Start/Complete/Wait";
*/

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, count; 
    MPI_Comm      comm;
    MPI_Win       win;
    MPI_Aint      extent;
    MPI_Group     wingroup, neighbors;
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

		MPI_Type_extent( recvtype.datatype, &extent );
		MPI_Win_create( recvtype.buf, recvtype.count * extent, 
				(int)extent, MPI_INFO_NULL, comm, &win );
		MPI_Win_get_group( win, &wingroup );
		if (rank == source) {
		    /* To improve reporting of problems about operations, we
		       change the error handler to errors return */
		    MPI_Win_set_errhandler( win, MPI_ERRORS_RETURN );
		    sendtype.InitBuf( &sendtype );
		    
		    /* Neighbor is dest only */
		    MPI_Group_incl( wingroup, 1, &dest, &neighbors );
		    err = MPI_Win_start( neighbors, 0, win );
		    if (err) {
			errs++;
			if (errs < 10) {
			    MTestPrintError( err );
			}
		    }
		    MPI_Group_free( &neighbors );
		    err = MPI_Put( sendtype.buf, sendtype.count, 
				    sendtype.datatype, dest, 0, 
				   recvtype.count, recvtype.datatype, win );
		    if (err) {
			errs++;
			MTestPrintError( err );
		    }
		    err = MPI_Win_complete( win );
		    if (err) {
			errs++;
			if (errs < 10) {
			    MTestPrintError( err );
			}
		    }
		}
		else if (rank == dest) {
		    MPI_Group_incl( wingroup, 1, &source, &neighbors );
		    MPI_Win_post( neighbors, 0, win );
		    MPI_Group_free( &neighbors );
		    MPI_Win_wait( win );
		    /* This should have the same effect, in terms of
		       transfering data, as a send/recv pair */
		    err = MTestCheckRecv( 0, &recvtype );
		    if (err) {
			errs += errs;
		    }
		}
		else {
		    /* Nothing; the other processes need not call any 
		       MPI routines */
		    ;
		}
		MPI_Win_free( &win );
		MTestFreeDatatype( &sendtype );
		MTestFreeDatatype( &recvtype );
		MPI_Group_free( &wingroup );
	    }
	}
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
