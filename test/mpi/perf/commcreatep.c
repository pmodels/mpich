/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

#define MAX_LOG_WSIZE 31
#define MAX_LOOP 20

int main( int argc, char *argv[] )
{
    MPI_Group gworld, g;
    MPI_Comm  comm, newcomm[MAX_LOOP];
    int       wsize, wrank, range[1][3], errs=0;
    double    t[MAX_LOG_WSIZE], tf;
    int       maxi, i, k, ts, gsize[MAX_LOG_WSIZE];

    MTest_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );

    if (wrank == 0)
	MTestPrintfMsg( 1, "size\ttime\n" );

    MPI_Comm_group( MPI_COMM_WORLD, &gworld );
    ts = 1;
    comm = MPI_COMM_WORLD;
    for (i=0; ts<=wsize; i++, ts = ts + ts) {
	/* Create some groups with at most ts members */
	range[0][0] = ts-1;
	range[0][1] = 0;
	range[0][2] = -1;
	MPI_Group_range_incl( gworld, 1, range, &g );
	
	MPI_Barrier( MPI_COMM_WORLD );
	tf       = MPI_Wtime();
	for (k=0; k<MAX_LOOP; k++) 
	    MPI_Comm_create( comm, g, &newcomm[k] );
	tf     = MPI_Wtime() - tf;
	MPI_Allreduce( &tf, &t[i], 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD );
	t[i] = t[i] / MAX_LOOP;
	gsize[i] = ts;
	if (wrank == 0)
	    MTestPrintfMsg( 1, "%d\t%e\n", ts, t[i] );
	MPI_Group_free( &g );
	if (newcomm[0] != MPI_COMM_NULL) 
	    for (k=0; k<MAX_LOOP; k++) 
		MPI_Comm_free( &newcomm[k] );
    }
    MPI_Group_free( &gworld );
    maxi = i-1;

    /* The cost should be linear or at worst ts*log(ts).
       We can check this in a number of ways.  
     */
    if (wrank == 0) {
	for (i=4; i<=maxi; i++) {
	    double rdiff;
	    if (t[i] > 0) {
		rdiff = (t[i] - t[i-1]) / t[i];
		if (rdiff >= 4) {
		    errs++;
		    fprintf( stderr, "Relative difference between group of size %d and %d is %e exceeds 4\n", 
			     gsize[i-1], gsize[i], rdiff );
		}
	    }
	}
    }

    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}
