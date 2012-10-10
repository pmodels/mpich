/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This simple test reads stdin and copies to stdout */
#include <stdio.h>
#include "mpi.h"

/* style: allow:puts:1 sig:0 */
/* style: allow:fgets:1 sig:0 */

int main( int argc, char *argv[] )
{
    int rank;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if (rank == 0) {
	char buf[128];
	while (fgets(buf,sizeof(buf),stdin)) {
	    puts(buf);
	}
    }
    MPI_Finalize();

    return 0;
}
