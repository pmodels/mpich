/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpi.h"
#include <stdio.h>

/*
    This version is used the check out the -mpedbg option when MPICH is
    built with the -mpedbg option (at configure!)
 */
main( int argc, char **argv )
{
    int dest = 0, *buffer=&dest;

    MPI_Init( &argc, &argv );
    /* Make erroneous call... */
    if (argc > 0) {
        buffer = 0;
        *buffer = 3;
    }
    else
        MPI_Send(buffer, 20, MPI_INT, dest, 1, NULL);
    MPI_Finalize();
}
