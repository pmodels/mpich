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

int main( int argc, char *argv[] )
{
    int errs = 0, i, k;
    int dims[2], periods[2], wsize;
    int outdims[2], outperiods[2], outcoords[2];
    int topo_type;
    int *index, *edges, *outindex, *outedges;
    MPI_Comm comm1, comm2;

    MTest_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &wsize );

    /* Create a cartesian topology, get its characteristics, then 
       dup it and check that the new communicator has the same properties */
    dims[0] = dims[1] = 0;
    MPI_Dims_create( wsize, 2, dims );
    periods[0] = periods[1] = 0;
    MPI_Cart_create( MPI_COMM_WORLD, 2, dims, periods, 0, &comm1 );

    MPI_Comm_dup( comm1, &comm2 );
    MPI_Topo_test( comm2, &topo_type );
    if (topo_type != MPI_CART) {
	errs++;
	printf( "Topo type of duped cart was not cart\n" );
    }
    else {
	MPI_Cart_get( comm2, 2, outdims, outperiods, outcoords );
	for (i=0; i<2; i++) {
	    if (outdims[i] != dims[i]) {
		errs++;
		printf( "%d = outdims[%d] != dims[%d] = %d\n", outdims[i],
			i, i, dims[i] );
	    }
	    if (outperiods[i] != periods[i]) {
		errs++;
		printf( "%d = outperiods[%d] != periods[%d] = %d\n", 
			outperiods[i], i, i, periods[i] );
	    }
	}
    }
    MPI_Comm_free( &comm2 );
    MPI_Comm_free( &comm1 );

    /* Now do the same with a graph topology */
    if (wsize >= 3) {
	index = (int*)malloc(wsize * sizeof(int) );
	edges = (int*)malloc(wsize * 2 * sizeof(int) );
	if (!index || !edges) {
	    printf( "Unable to allocate %d words for index or edges\n", 
		    3 * wsize );
	    MPI_Abort( MPI_COMM_WORLD, 1 );
	}
	index[0] = 2;
	for (i=1; i<wsize; i++) {
	    index[i] = 2 + index[i-1];
	}
	k=0;
	for (i=0; i<wsize; i++) {
	    edges[k++] = (i-1+wsize) % wsize;
	    edges[k++] = (i+1) % wsize;
	}
	MPI_Graph_create( MPI_COMM_WORLD, wsize, index, edges, 0, &comm1 );
	MPI_Comm_dup( comm1, &comm2 );
	MPI_Topo_test( comm2, &topo_type );
	if (topo_type != MPI_GRAPH) {
	    errs++;
	    printf( "Topo type of duped graph was not graph\n" );
	}
	else {
	    int nnodes, nedges;
	    MPI_Graphdims_get( comm2, &nnodes, &nedges );
	    if (nnodes != wsize) {
		errs++;
		printf( "Nnodes = %d, should be %d\n", nnodes, wsize );
	    }
	    if (nedges != 2*wsize) {
		errs++;
		printf( "Nedges = %d, should be %d\n", nedges, 2*wsize );
	    }
	    outindex = (int*)malloc(wsize * sizeof(int) );
	    outedges = (int*)malloc(wsize * 2 * sizeof(int) );
	    if (!outindex || !outedges) {
		printf( "Unable to allocate %d words for outindex or outedges\n", 
			3 * wsize );
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }
	    
	    MPI_Graph_get( comm2, wsize, 2*wsize, outindex, outedges );
	    for (i=0; i<wsize; i++) {
		if (index[i] != outindex[i]) {
		    printf( "%d = index[%d] != outindex[%d] = %d\n",
			    index[i], i, i, outindex[i] );
		    errs++;
		}
	    }
	    for (i=0; i<2*wsize; i++) {
		if (edges[i] != outedges[i]) {
		    printf( "%d = edges[%d] != outedges[%d] = %d\n",
			    edges[i], i, i, outedges[i] );
		    errs++;
		}
	    }
	    free( outindex );
	    free( outedges );
	}
	free( index );
	free( edges );

	MPI_Comm_free( &comm2 );
	MPI_Comm_free( &comm1 );
    }
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
