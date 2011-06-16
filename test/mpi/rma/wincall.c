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
static char MTEST_Descrip[] = "Test win_call_errhandler";
*/

static int calls = 0;
static int errs = 0;
static MPI_Win mywin;
void eh( MPI_Win *win, int *err, ... );
void eh( MPI_Win *win, int *err, ... )
{
    if (*err != MPI_ERR_OTHER) {
	errs++;
	printf( "Unexpected error code\n" );
    }
    if (*win != mywin) {
	errs++;
	printf( "Unexpected window\n" );
    }
    calls++;
    return;
}
int main( int argc, char *argv[] )
{
    int buf[2];
    MPI_Win        win;
    MPI_Errhandler newerr;
    int            i;

    MTest_Init( &argc, &argv );

    /* Run this test multiple times to expose storage leaks (we found a leak
       of error handlers with this test) */
    for (i=0;i<1000; i++)  {
	calls = 0;
	
	MPI_Win_create( buf, 2*sizeof(int), sizeof(int), 
			MPI_INFO_NULL, MPI_COMM_WORLD, &win );
	mywin = win;
	
	MPI_Win_create_errhandler( eh, &newerr );
	
	MPI_Win_set_errhandler( win, newerr );
	MPI_Win_call_errhandler( win, MPI_ERR_OTHER );
	MPI_Errhandler_free( &newerr );
	if (calls != 1) {
	    errs++;
	    printf( "Error handler not called\n" );
	}
	MPI_Win_free( &win );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
