/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "A simple test of Reduce with all choices of root process";
*/

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, root;
    int *sendbuf, *recvbuf, i;
    int minsize = 2, count; 
    MPI_Comm      comm;

    MTest_Init( &argc, &argv );

    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
	/* Determine the sender and receiver */
	MPI_Comm_rank( comm, &rank );
	MPI_Comm_size( comm, &size );
	
	for (count = 1; count < 130000; count = count * 2) {
	    sendbuf = (int *)malloc( count * sizeof(int) );
	    recvbuf = (int *)malloc( count * sizeof(int) );
	    for (root = 0; root < size; root ++) {
		for (i=0; i<count; i++) sendbuf[i] = i;
		for (i=0; i<count; i++) recvbuf[i] = -1;
		MPI_Reduce( sendbuf, recvbuf, count, MPI_INT, MPI_SUM, 
			    root, comm );
		if (rank == root) {
		    for (i=0; i<count; i++) {
			if (recvbuf[i] != i * size) {
			    errs++;
			}
		    }
		}
	    }
	    free( sendbuf );
	    free( recvbuf );
	}

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
        if (0 == rank) {
            MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
            if (MPI_SUCCESS == MPI_Reduce(&rank, &rank, 1, MPI_INT, MPI_SUM, 0, comm))
                errs++;
        }
#endif

	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
