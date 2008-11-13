/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rc, result;
    int ranks[1];
    MPI_Group group, outgroup;
    MPI_Comm comm;

    MTest_Init( &argc, &argv );
    /* To improve reporting of problems about operations, we
       change the error handler to errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    while (MTestGetComm( &comm, 1 )) {
	if (comm == MPI_COMM_NULL) continue;

	MPI_Comm_group( comm, &group );
	rc = MPI_Group_incl( group, 0, 0, &outgroup );
	if (rc) {
	    errs++;
	    MTestPrintError( rc );
	    printf( "Error in creating an empty group with (0,0)\n" );
	    
	    /* Some MPI implementations may reject a null "ranks" pointer */
	    rc = MPI_Group_incl( group, 0, ranks, &outgroup );
	    if (rc) {
		errs++;
		MTestPrintError( rc );
		printf( "Error in creating an empty group with (0,ranks)\n" );
	    }
	}

	if (outgroup != MPI_GROUP_EMPTY) {
	    /* Is the group equivalent to group empty? */
	    rc = MPI_Group_compare( outgroup, MPI_GROUP_EMPTY, &result );
	    if (result != MPI_IDENT) {
		errs++;
		MTestPrintError( rc );
		printf( "Did not create a group equivalent to an empty group\n" );
	    }
	}
	rc = MPI_Group_free( &group );
	if (rc) {
	    errs++;
	    MTestPrintError( rc );
	}	    
	if (outgroup != MPI_GROUP_NULL) {
	    rc = MPI_Group_free( &outgroup );
	    if (rc) {
		errs++;
		MTestPrintError( rc );
	    }
	}

	MTestFreeComm( &comm );
    }
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
