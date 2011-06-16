/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test error reporting from faults with point to point communication";
*/

int ReportErr( int errcode, const char name[] );

int main( int argc, char *argv[] )
{
    int wrank, wsize, rank, size, color;
    int j, tmp;
    int err, toterrs, errs = 0;
    MPI_Comm newcomm;

    MPI_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );

    /* Color is 0 or 1; 1 will be the processes that "fault" */
    /* process 0 and wsize/2+1...wsize-1 are in non-faulting group */
    color = (wrank > 0) && (wrank <= wsize/2);
    MPI_Comm_split( MPI_COMM_WORLD, color, wrank, &newcomm );

    MPI_Comm_size( newcomm, &size );
    MPI_Comm_rank( newcomm, &rank );

    /* Set errors return on COMM_WORLD and the new comm */
    MPI_Comm_set_errhandler( MPI_ERRORS_RETURN, MPI_COMM_WORLD );
    MPI_Comm_set_errhandler( MPI_ERRORS_RETURN, newcomm );

    err = MPI_Barrier( MPI_COMM_WORLD );
    if (err) errs += ReportErr( err, "Barrier" );
    if (color) {
	/* Simulate a fault on some processes */
	exit(1);
    }
    else {
	/* To improve the chance that the "faulted" processes will have
	   exited, wait for 1 second */
	sleep( 1 );
    }
    
    /* Can we still use newcomm? */
    for (j=0; j<rank; j++) {
	err = MPI_Recv( &tmp, 1, MPI_INT, j, 0, newcomm, MPI_STATUS_IGNORE );
	if (err) errs += ReportErr( err, "Recv" );
    }
    for (j=rank+1; j<size; j++) {
	err = MPI_Send( &rank, 1, MPI_INT, j, 0, newcomm );
	if (err) errs += ReportErr( err, "Recv" );
    }

    /* Now, try sending in MPI_COMM_WORLD on dead processes */
    /* There is a race condition here - we don't know for sure that the faulted
       processes have exited.  However, we can ensure a failure by using 
       synchronous sends - the sender will wait until the reciever handles 
       receives the message, which will not happen (the process will exit 
       without matching the message, even if it has not yet exited). */
    for (j=1; j<=wsize/2; j++) {
	err = MPI_Ssend( &rank, 1, MPI_INT, j, 0, MPI_COMM_WORLD );
	if (!err) {
	    errs++;
	    fprintf( stderr, "Ssend succeeded to dead process %d\n", j );
	}
    }

    err = MPI_Allreduce( &errs, &toterrs, 1, MPI_INT, MPI_SUM, newcomm );
    if (err) errs += ReportErr( err, "Allreduce" );
    MPI_Comm_free( &newcomm );

    MPI_Finalize();

    if (wrank == 0) {
	if (toterrs > 0) {
	    printf( " Found %d errors\n", toterrs );
	}
	else {
	    printf( " No Errors\n" );
	}
    }

    return 0;
}

int ReportErr( int errcode, const char name[] )
{
    int errclass, errlen;
    char errmsg[MPI_MAX_ERROR_STRING];
    MPI_Error_class( errcode, &errclass );
    MPI_Error_string( errcode, errmsg, &errlen );
    fprintf( stderr, "In %s, error code %d(class %d) = %s\n",
	     name, errcode, errclass, errmsg );
    return 1;
}    
