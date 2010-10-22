/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int errs = 0;
    int ranks[128], ranksout[128];
    MPI_Group gworld, gself;
    MPI_Comm  comm;
    int rank, size, i;

    MTest_Init( &argc, &argv );

    MPI_Comm_group( MPI_COMM_SELF, &gself );

    comm = MPI_COMM_WORLD;

    MPI_Comm_size( comm, &size );
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_group( comm, &gworld );
    for (i=0; i<size; i++) {
	ranks[i] = i;
	ranksout[i] = -1;
    }
    /* Try translating ranks from comm world compared against
       comm self, so most will be UNDEFINED */
    MPI_Group_translate_ranks( gworld, size, ranks, gself, ranksout );
    
    for (i=0; i<size; i++) {
	if (i == rank) {
	    if (ranksout[i] != 0) {
		printf( "[%d] Rank %d is %d but should be 0\n", rank, 
			i, ranksout[i] );
		errs++;
	    }
	}
	else {
	    if (ranksout[i] != MPI_UNDEFINED) {
		printf( "[%d] Rank %d is %d but should be undefined\n", rank, 
			i, ranksout[i] );
		errs++;
	    }
	}
    }

    /* MPI-2 Errata requires that MPI_PROC_NULL is mapped to MPI_PROC_NULL */
    ranks[0] = MPI_PROC_NULL;
    ranks[1] = 1;
    ranks[2] = rank;
    ranks[3] = MPI_PROC_NULL;
    for (i=0; i<4; i++) ranksout[i] = -1;

    MPI_Group_translate_ranks( gworld, 4, ranks, gself, ranksout );
    if (ranksout[0] != MPI_PROC_NULL) {
	printf( "[%d] Rank[0] should be MPI_PROC_NULL but is %d\n",
		rank, ranksout[0] );
	errs++;
    }
    if (rank != 1 && ranksout[1] != MPI_UNDEFINED) {
	printf( "[%d] Rank[1] should be MPI_UNDEFINED but is %d\n",
		rank, ranksout[1] );
	errs++;
    }
    if (rank == 1 && ranksout[1] != 0) {
	printf( "[%d] Rank[1] should be 0 but is %d\n",
		rank, ranksout[1] );
	errs++;
    }
    if (ranksout[2] != 0) {
	printf( "[%d] Rank[2] should be 0 but is %d\n",
		rank, ranksout[2] );
	errs++;
    }
    if (ranksout[3] != MPI_PROC_NULL) {
	printf( "[%d] Rank[3] should be MPI_PROC_NULL but is %d\n",
		rank, ranksout[3] );
	errs++;
    }

    MPI_Group_free(&gself);
    MPI_Group_free(&gworld);

    MTest_Finalize( errs );
    MPI_Finalize();

    return 0;
}
