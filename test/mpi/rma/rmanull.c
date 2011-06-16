/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test the MPI_PROC_NULL is a valid target";
*/

int main( int argc, char *argv[] )
{
    int           errs = 0, err;
    int           rank, size;
    int           *buf, bufsize;
    int           *rmabuf, rsize, rcount;
    MPI_Comm      comm;
    MPI_Win       win;

    MTest_Init( &argc, &argv );

    bufsize = 256 * sizeof(int);
    buf     = (int *)malloc( bufsize );
    if (!buf) {
	fprintf( stderr, "Unable to allocated %d bytes\n", bufsize );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    rcount   = 16;
    rsize    = rcount * sizeof(int);
    rmabuf   = (int *)malloc( rsize );
    if (!rmabuf) {
	fprintf( stderr, "Unable to allocated %d bytes\n", rsize );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
	
    /* The following illustrates the use of the routines to 
       run through a selection of communicators and datatypes.
       Use subsets of these for tests that do not involve combinations 
       of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral( &comm, 1, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
	/* Determine the sender and receiver */
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_size( comm, &size );
	
	MPI_Win_create( buf, bufsize, sizeof(int), MPI_INFO_NULL, comm, &win );
	MPI_Win_fence( 0, win );
	/* To improve reporting of problems about operations, we
	   change the error handler to errors return */
	MPI_Win_set_errhandler( win, MPI_ERRORS_RETURN );

	err = MPI_Put( rmabuf, rcount, MPI_INT, 
		       MPI_PROC_NULL, 0, rcount, MPI_INT, win );
	if (err) {
	    errs++;
	    if (errs < 10) {
		MTestPrintErrorMsg( "PROC_NULL to Put", err );
	    }
	}
	err = MPI_Win_fence( 0, win );
	if (err) {
	    errs++;
	    if (errs < 10) {
		MTestPrintErrorMsg( "Fence after Put", err );
	    }
	}

	err = MPI_Get( rmabuf, rcount, MPI_INT, 
		       MPI_PROC_NULL, 0, rcount, MPI_INT, win );
	if (err) {
	    errs++;
	    if (errs < 10) {
		MTestPrintErrorMsg( "PROC_NULL to Get", err );
	    }
	}
	err = MPI_Win_fence( 0, win );
	if (err) {
	    errs++;
	    if (errs < 10) {
		MTestPrintErrorMsg( "Fence after Get", err );
	    }
	}

	err = MPI_Accumulate( rmabuf, rcount, MPI_INT, 
			      MPI_PROC_NULL, 0, rcount, MPI_INT, MPI_SUM, win );
	if (err) {
	    errs++;
	    if (errs < 10) {
		MTestPrintErrorMsg( "PROC_NULL to Accumulate", err );
	    }
	}
	err = MPI_Win_fence( 0, win );
	if (err) {
	    errs++;
	    if (errs < 10) {
		MTestPrintErrorMsg( "Fence after Accumulate", err );
	    }
	}

	MPI_Win_free( &win );
        MTestFreeComm(&comm);
    }

    free( buf );
    free( rmabuf );
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
