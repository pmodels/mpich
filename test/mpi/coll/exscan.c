/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
static char MTEST_Descrip[] = "Test MPI_Exscan";
*/

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size;
    int minsize = 2, count; 
    int *sendbuf, *recvbuf, i;
    MPI_Comm      comm;

    MTest_Init( &argc, &argv );

    /* The following illustrates the use of the routines to 
       run through a selection of communicators and datatypes.
       Use subsets of these for tests that do not involve combinations 
       of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;

	MPI_Comm_rank( comm, &rank );
	MPI_Comm_size( comm, &size );
	
	for (count = 1; count < 65000; count = count * 2) {

	    sendbuf = (int *)malloc( count * sizeof(int) );
	    recvbuf = (int *)malloc( count * sizeof(int) );

	    for (i=0; i<count; i++) {
		sendbuf[i] = rank + i * size;
		recvbuf[i] = -1;
	    }
	    
	    MPI_Exscan( sendbuf, recvbuf, count, MPI_INT, MPI_SUM, comm );

	    /* Check the results.  rank 0 has no data */
	    if (rank > 0) {
		int result;
		for (i=0; i<count; i++) {
		    result = rank * i * size + ((rank) * (rank-1))/2;
		    if (recvbuf[i] != result) {
			errs++;
			if (errs < 10) {
			    fprintf( stderr, "Error in recvbuf[%d] = %d on %d, expected %d\n",
				     i, recvbuf[i], rank, result );
			}
		    }
		}
	    }

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
            /* now try the MPI_IN_PLACE flavor */
            for (i=0; i<count; i++) {
                sendbuf[i] = -1; /* unused */
                recvbuf[i] = rank + i * size;
            }

            MPI_Exscan( MPI_IN_PLACE, recvbuf, count, MPI_INT, MPI_SUM, comm );

            /* Check the results.  rank 0's data must remain unchanged */
            for (i=0; i<count; i++) {
                int result;
                if (rank == 0)
                    result = rank + i * size;
                else
                    result = rank * i * size + ((rank) * (rank-1))/2;
                if (recvbuf[i] != result) {
                    errs++;
                    if (errs < 10) {
                        fprintf( stderr, "Error in recvbuf[%d] = %d on %d, expected %d\n",
                                 i, recvbuf[i], rank, result );
                    }
                }
            }
#endif

	    free( sendbuf );
	    free( recvbuf );
	}
	MTestFreeComm( &comm );
    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
