/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test intercomm merge, including the choice of the high value";
*/

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, rsize;
    int nsize, nrank;
    int minsize = 2;
    int isLeft;
    MPI_Comm      comm, comm1, comm2, comm3, comm4;

    MTest_Init( &argc, &argv );

    /* The following illustrates the use of the routines to 
       run through a selection of communicators and datatypes.
       Use subsets of these for tests that do not involve combinations 
       of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntercomm( &comm, &isLeft, minsize )) {
	if (comm == MPI_COMM_NULL) continue;
	/* Determine the sender and receiver */
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_remote_size( comm, &rsize );
	MPI_Comm_size( comm, &size );

	/* Try building intercomms */
	MPI_Intercomm_merge( comm, isLeft, &comm1 );
	/* Check the size and ranks */
	MPI_Comm_size( comm1, &nsize );
	MPI_Comm_rank( comm1, &nrank );
	if (nsize != size + rsize) {
	    errs++;
	    printf( "(1) Comm size is %d but should be %d\n", nsize,
		    size + rsize );
	    if (isLeft) {
		/* The left processes should be high */
		if (nrank != rsize + rank) {
		    errs++;
		    printf( "(1) rank for high process is %d should be %d\n",
			    nrank, rsize + rank );
		}
	    }
	    else {
		/* The right processes should be low */
		if (nrank != rank) {
		    errs++;
		    printf( "(1) rank for low process is %d should be %d\n",
			    nrank, rank );
		}
	    }
	}
	
 	MPI_Intercomm_merge( comm, !isLeft, &comm2 ); 
	/* Check the size and ranks */
	MPI_Comm_size( comm1, &nsize );
	MPI_Comm_rank( comm1, &nrank );
	if (nsize != size + rsize) {
	    errs++;
	    printf( "(2) Comm size is %d but should be %d\n", nsize,
		    size + rsize );
	    if (!isLeft) {
		/* The right processes should be high */
		if (nrank != rsize + rank) {
		    errs++;
		    printf( "(2) rank for high process is %d should be %d\n",
			    nrank, rsize + rank );
		}
	    }
	    else {
		/* The left processes should be low */
		if (nrank != rank) {
		    errs++;
		    printf( "(2) rank for low process is %d should be %d\n",
			    nrank, rank );
		}
	    }
	}
	

 	MPI_Intercomm_merge( comm, 0, &comm3 ); 

 	MPI_Intercomm_merge( comm, 1, &comm4 ); 
	
	MPI_Comm_free( &comm1 );
	MPI_Comm_free( &comm2 );
	MPI_Comm_free( &comm3 ); 
	MPI_Comm_free( &comm4 );
      
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
