/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "A simple test of Comm_spawn, called twice";
*/

/*
 * One bug report indicated that two calls to Spawn failed, even when 
 * one succeeded.  This test makes sure that an MPI program can make
 * multiple calls to spawn.
 */

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI_Comm      parentcomm, intercomm, intercomm2;
    MPI_Status    status;

    MTest_Init( &argc, &argv );

    MPI_Comm_get_parent( &parentcomm );

    if (parentcomm == MPI_COMM_NULL) {
	/* Create 2 more processes */
	/* This uses a Unix name for the program; 
	   Windows applications may need to use just spawn2 or
	   spawn2.exe.  We can't rely on using info to place . in the
	   path, since info is not required to be followed. */
	MPI_Comm_spawn( (char*)"./spawn2", MPI_ARGV_NULL, np,
			MPI_INFO_NULL, 0, MPI_COMM_WORLD,
			&intercomm, errcodes );
    }
    else 
	intercomm = parentcomm;

    /* We now have a valid intercomm */

    MPI_Comm_remote_size( intercomm, &rsize );
    MPI_Comm_size( intercomm, &size );
    MPI_Comm_rank( intercomm, &rank );

    if (parentcomm == MPI_COMM_NULL) {
	/* Master */
	if (rsize != np) {
	    errs++;
	    printf( "Did not create %d processes (got %d)\n", np, rsize );
	}
	if (rank == 0) {
	    for (i=0; i<rsize; i++) {
		MPI_Send( &i, 1, MPI_INT, i, 0, intercomm );
	    }
	    /* We could use intercomm reduce to get the errors from the 
	       children, but we'll use a simpler loop to make sure that
	       we get valid data */
	    for (i=0; i<rsize; i++) {
		MPI_Recv( &err, 1, MPI_INT, i, 1, intercomm, MPI_STATUS_IGNORE );
		errs += err;
	    }
	}
    }
    else {
	/* Child */
	char cname[MPI_MAX_OBJECT_NAME];
	int rlen;

	if (size != np) {
	    errs++;
	    printf( "(Child) Did not create %d processes (got %d)\n", 
		    np, size );
	}
	/* Check the name of the parent */
	cname[0] = 0;
	MPI_Comm_get_name( intercomm, cname, &rlen );
	/* MPI-2 section 8.4 requires that the parent have this
	   default name */
	if (strcmp( cname, "MPI_COMM_PARENT" ) != 0) {
	    errs++;
	    printf( "Name of parent is not correct\n" );
	    if (rlen > 0 && cname[0]) {
		printf( " Got %s but expected MPI_COMM_PARENT\n", cname );
	    }
	    else {
		printf( " Expected MPI_COMM_PARENT but no name set\n" );
	    }
	}
	MPI_Recv( &i, 1, MPI_INT, 0, 0, intercomm, &status );
	if (i != rank) {
	    errs++;
	    printf( "Unexpected rank on child %d (%d)\n", rank, i );
	}
	/* Send the errs back to the master process */
	MPI_Ssend( &errs, 1, MPI_INT, 0, 1, intercomm );
    }

    if (parentcomm == MPI_COMM_NULL) {
	/* Create 2 more processes */
	/* This uses a Unix name for the program; 
	   Windows applications may need to use just spawn2 or
	   spawn2.exe.  We can't rely on using info to place . in the
	   path, since info is not required to be followed. */
	MPI_Comm_spawn( (char*)"./spawn2", MPI_ARGV_NULL, np,
			MPI_INFO_NULL, 0, MPI_COMM_WORLD,
			&intercomm2, MPI_ERRCODES_IGNORE );
    }
    else 
	intercomm2 = parentcomm;

    /* We now have a valid intercomm */

    MPI_Comm_remote_size( intercomm2, &rsize );
    MPI_Comm_size( intercomm2, &size );
    MPI_Comm_rank( intercomm2, &rank );

    if (parentcomm == MPI_COMM_NULL) {
	/* Master */
	if (rsize != np) {
	    errs++;
	    printf( "Did not create %d processes (got %d)\n", np, rsize );
	}
	if (rank == 0) {
	    for (i=0; i<rsize; i++) {
		MPI_Send( &i, 1, MPI_INT, i, 0, intercomm2 );
	    }
	    /* We could use intercomm2 reduce to get the errors from the 
	       children, but we'll use a simpler loop to make sure that
	       we get valid data */
	    for (i=0; i<rsize; i++) {
		MPI_Recv( &err, 1, MPI_INT, i, 1, intercomm2, MPI_STATUS_IGNORE );
		errs += err;
	    }
	}

	MPI_Comm_free( &intercomm2 );
    }

    /* It isn't necessary to free the intercomm, but it should not hurt */
    MPI_Comm_free( &intercomm );

    /* Note that the MTest_Finalize get errs only over COMM_WORLD */
    /* Note also that both the parent and child will generate "No Errors"
       if both call MTest_Finalize */
    if (parentcomm == MPI_COMM_NULL) {
	MTest_Finalize( errs );
    }

    MPI_Finalize();
    return 0;
}
