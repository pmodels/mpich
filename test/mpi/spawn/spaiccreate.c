/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Use Spawn to create an intercomm, then create a new intercomm that includes processes not in the initial spawn intercomm";
*/

/*
 * This test ensures that spawned processes are able to communicate with 
 * processes that were not in the communicator from which they were spawned.
 */

int main( int argc, char *argv[] )
{
    int errs = 0;
    int wrank, wsize;
    int np = 2;
    int errcodes[2];
    MPI_Comm      parentcomm, intercomm, intercomm2;

    MTest_Init( &argc, &argv );

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( MPI_COMM_WORLD, &wsize );

    MPI_Comm_get_parent( &parentcomm );

    if (parentcomm == MPI_COMM_NULL) {
	/* Create 2 more processes, from process 0 in the original 
	   comm world */
	if (wrank == 0) {
	    MPI_Comm_spawn( (char*)"./spaiccreate", MPI_ARGV_NULL, np,
			    MPI_INFO_NULL, 0, MPI_COMM_SELF,
			    &intercomm, errcodes );
	}
	else {
	    intercomm = MPI_COMM_NULL;
	}
    }
    else 
	intercomm = parentcomm;

    /* We now have a valid intercomm.  Use it to create a NEW intercomm
       that includes all processes */
    MPI_Intercomm_create( MPI_COMM_WORLD, 0, intercomm, 0, 123, &intercomm2 );
    
    /* Have the spawned processes send to rank 1 in the comm world of the 
       parent */
    if (parentcomm == MPI_COMM_NULL) {
	MPI_Send( &wrank, 1, MPI_INT, 1, wrank, intercomm2 );
    }
    else {
	if (wrank == 1) {
	    int i, rsize, rrank;
	    MPI_Status status;

	    MPI_Comm_remote_size( intercomm2, &rsize );
	    for (i=0; i<rsize; i++) {
		MPI_Recv( &rrank, 1, MPI_INT, i, i, intercomm2, &status );
		if (rrank != i) {
		    errs++;
		    printf( "Received %d from %d; expected %d\n",
			    rrank, i, i );
		}
	    }
	}
    }

    /*    printf( "%sAbout to barrier on intercomm2\n", 
	    (parentcomm == MPI_COMM_NULL) ? "<orig>" : "<spawned>" ); 
	    fflush(stdout);*/
    MPI_Barrier( intercomm2 );
    
    /* It isn't necessary to free the intercomms, but it should not hurt */
    if (intercomm != MPI_COMM_NULL) 
	MPI_Comm_free( &intercomm );
    MPI_Comm_free( &intercomm2 );

    /* Note that the MTest_Finalize get errs only over COMM_WORLD */
    /* Note also that both the parent and child will generate "No Errors"
       if both call MTest_Finalize */
    if (parentcomm == MPI_COMM_NULL) {
	MTest_Finalize( errs );
    }

    MPI_Finalize();
    return 0;
}
