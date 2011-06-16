/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include <string.h>

/*
static char MTEST_Descrip[] = "Locks with no RMA operations";
*/

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, i;
    MPI_Comm      comm;
    MPI_Win       win;
    int           *winbuf, count;

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    /* Allocate and initialize buf */
    count  = 1000;

    MPI_Alloc_mem( count*sizeof(int), MPI_INFO_NULL, &winbuf );

    MPI_Win_create( winbuf, count * sizeof(int), sizeof(int), MPI_INFO_NULL, 
		    comm, &win );

    /* Clear winbuf */
    memset( winbuf, 0, count*sizeof(int) );

    /* Note that for i == rank, this is a useful operation - it allows 
       the programmer to use direct loads and stores, rather than 
       put/get/accumulate, to access the local memory window. */
    for (i=0; i<size; i++) {
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, i, 0, win );
	MPI_Win_unlock( i, win );
    }

    for (i=0; i<size; i++) {
	MPI_Win_lock( MPI_LOCK_SHARED, i, 0, win );
	MPI_Win_unlock( i, win );
    }

    MPI_Win_free( &win );
    MPI_Free_mem( winbuf );

    /* If this test completes, no error has been found */
    /* A more complete test may ensure that local locks in fact block
       remote, exclusive locks */
    MTest_Finalize( errs );

    MPI_Finalize();
    return 0;
}
