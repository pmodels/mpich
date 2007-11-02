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

int main( int argc, char *argv[] )
{
    int errs = 0;
    char port_name[MPI_MAX_PORT_NAME], port_name_out[MPI_MAX_PORT_NAME];
    char serv_name[256];
    int merr, mclass;
    char errmsg[MPI_MAX_ERROR_STRING];
    int msglen;
    int rank;

    MTest_Init( &argc, &argv );

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Note that according to the MPI standard, port_name must
       have been created by MPI_Open_port.  For current testing
       purposes, we'll use a fake name.  This test should eventually use
       a valid name from Open_port */

    strcpy( port_name, "otherhost:122" );
    strcpy( serv_name, "MyTest" );

    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    if (rank == 0)
    {
	merr = MPI_Publish_name( serv_name, MPI_INFO_NULL, port_name );
	if (merr) {
	    errs++;
	    MPI_Error_string( merr, errmsg, &msglen );
	    printf( "Error in Publish_name: \"%s\"\n", errmsg );
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	
	merr = MPI_Unpublish_name( serv_name, MPI_INFO_NULL, port_name );
	if (merr) {
	    errs++;
	    MPI_Error_string( merr, errmsg, &msglen );
	    printf( "Error in Unpublish name: \"%s\"\n", errmsg );
	}

    }
    else
    {
	MPI_Barrier(MPI_COMM_WORLD);
	
	merr = MPI_Lookup_name( serv_name, MPI_INFO_NULL, port_name_out );
	if (merr) {
	    errs++;
	    MPI_Error_string( merr, errmsg, &msglen );
	    printf( "Error in Lookup name: \"%s\"\n", errmsg );
	}
	else {
	    if (strcmp( port_name, port_name_out )) {
		errs++;
		printf( "Lookup name returned the wrong value (%s)\n", 
			port_name_out );
	    }
	}

	MPI_Barrier(MPI_COMM_WORLD);
    }


    MPI_Barrier(MPI_COMM_WORLD);
    
    merr = MPI_Lookup_name( serv_name, MPI_INFO_NULL, port_name_out );
    if (!merr) {
	errs++;
	printf( "Lookup name returned name after it was unpublished\n" );
    }
    else {
	/* Must be class MPI_ERR_NAME */
	MPI_Error_class( merr, &mclass );
	if (mclass != MPI_ERR_NAME) {
	    errs++;
	    MPI_Error_string( merr, errmsg, &msglen );
	    printf( "Lookup name returned the wrong error class (%d), msg: \"%s\"\n", 
		    mclass, errmsg );
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
