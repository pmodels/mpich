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

/* calling MPI_Unpublish_name before calling MPI_Publish_name 
   (a user error) would  cause the process manager to die. This is 
   added as a regression test so that it doesn't happen in the future. */

int main( int argc, char *argv[] )
{
    int errs = 0;
    char port_name[MPI_MAX_PORT_NAME], serv_name[256];

    MTest_Init( &argc, &argv );

    strcpy( port_name, "otherhost:122" );
    strcpy( serv_name, "MyTest" );

    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    MPI_Unpublish_name( serv_name, MPI_INFO_NULL, port_name );

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
