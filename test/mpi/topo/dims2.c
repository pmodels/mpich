/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int prodof( int, const int[] );
/*
 * Test edge cases of Dims_create
 */
int prodof( int ndims, const int dims[] )
{
    int i, prod=1;
    for (i=0; i<ndims; i++) 
	prod *= dims[i];
    return prod;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    int dims[4], nnodes;

    MTest_Init( &argc, &argv );

    /* 2 dimensional tests */
    for (nnodes=1; nnodes <= 32; nnodes = nnodes * 2) {
	dims[0] = 0;
	dims[1] = nnodes;
	
	MPI_Dims_create( nnodes, 2, dims );
	if (prodof(2, dims) != nnodes) {
	    errs++;
	    printf( "Dims_create returned the wrong decomposition.  " );
	    printf( "Is [%d x %d], should be 1 x %d\n", dims[0], dims[1], 
		    nnodes );
	}
	
	/* Try calling Dims_create with nothing to do (all dimensions
	   specified) */
	dims[0] = 1;
	dims[1] = nnodes;
	MPI_Dims_create( nnodes, 2, dims );
	if (prodof(2, dims) != nnodes) {
	    errs++;
	    printf( "Dims_create returned the wrong decomposition (all given).  " );
	    printf( "Is [%d x %d], should be 1 x %d\n", dims[0], dims[1], 
		    nnodes );
	}

    }

    /* 4 dimensional tests */
    for (nnodes=4; nnodes <= 32; nnodes = nnodes * 2) {
	dims[0] = 0;
	dims[1] = nnodes/2;
	dims[2] = 0;
	dims[3] = 2;
	
	MPI_Dims_create( nnodes, 4, dims );
	if (prodof(4, dims) != nnodes) {
	    errs++;
	    printf( "Dims_create returned the wrong decomposition.  " );
	    printf( "Is [%d x %d x %d x %d], should be 1 x %d x 1 x 2\n", 
		    dims[0], dims[1], dims[2], dims[3],
		    nnodes/2 );
	}
	
	/* Try calling Dims_create with nothing to do (all dimensions
	   specified) */
	dims[0] = 1;
	dims[1] = nnodes/2;
	dims[2] = 1;
	dims[3] = 2;
	MPI_Dims_create( nnodes, 4, dims );
	if (prodof(4, dims) != nnodes) {
	    errs++;
	    printf( "Dims_create returned the wrong decomposition (all given).  " );
	    printf( "Is [%d x %d x %d x %d], should be 1 x %d x 1 x 2\n", 
		    dims[0], dims[1], dims[2], dims[3],
		    nnodes/2 );
	}

    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
