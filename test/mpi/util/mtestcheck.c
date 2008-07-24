/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI_Comm comm;
    int minsize = 2, csize, count;

    MTest_Init( &argc, &argv );

    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;

	MPI_Comm_size( comm, &csize );
	count = 128000;
	{
	    int *ap, *bp;
	    int root;
	    ap = (int *)malloc( count * sizeof(int) );
	    bp = (int *)malloc( count * sizeof(int) );
	    root=0;
	    MPI_Reduce( ap, bp, count, MPI_INT, MPI_SUM, root, comm );
	    free( ap );
	    free( bp );
	}
	MTestFreeComm( &comm );
    }

  MTest_Finalize( errs );

  MPI_Finalize();

  return 0;
}
