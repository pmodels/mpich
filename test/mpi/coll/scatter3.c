/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

/* This example sends contiguous data and receives a vector on some nodes
   and contiguous data on others.  There is some evidence that some
   MPI implementations do not check recvcount on the root process; this
   test checks for that case 
*/

int main( int argc, char **argv )
{
    MPI_Datatype vec;
    double *vecin, *vecout, ivalue;
    int    root, i, n, stride, errs = 0;
    int    rank, size;
    MPI_Aint vextent;

    MTest_Init( &argc, &argv );
    
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    n = 12;
    stride = 10;
    /* Note that vecout really needs to be only (n-1)*stride+1 doubles, but
       this is easier and allows a little extra room if there is a bug */
    vecout = (double *)malloc( n * stride * sizeof(double) );
    vecin  = (double *)malloc( n * size * sizeof(double) );
    
    MPI_Type_vector( n, 1, stride, MPI_DOUBLE, &vec );
    MPI_Type_commit( &vec );
    MPI_Type_extent( vec, &vextent );
    if (vextent != ((n-1)*(MPI_Aint)stride + 1) * sizeof(double) ) {
	errs++;
	printf( "Vector extent is %ld, should be %ld\n", 
		 (long) vextent, (long)(((n-1)*stride+1)*sizeof(double)) );
    }
    /* Note that the exted of type vector is from the first to the
       last element, not n*stride.
       E.g., with n=1, the extent is a single double */

    for (i=0; i<n*size; i++) vecin[i] = (double)i;
    for (root=0; root<size; root++) {
	for (i=0; i<n*stride; i++) vecout[i] = -1.0;
	if (rank == root) {
	    /* Receive into a vector */
	    MPI_Scatter( vecin, n, MPI_DOUBLE, vecout, 1, vec, 
			 root, MPI_COMM_WORLD );
	    for (i=0; i<n; i++) {
		ivalue = n*root + i;
		if (vecout[i*stride] != ivalue) {
		    errs++;
		    printf( "[%d] Expected %f but found %f for vecout[%d] on root\n", 
			    rank, ivalue, vecout[i*stride], i *stride );
		}
	    }
	}
	else {
	    /* Receive into contiguous data */
	    MPI_Scatter( NULL, -1, MPI_DATATYPE_NULL, vecout, n, MPI_DOUBLE,
			 root, MPI_COMM_WORLD );
	    for (i=0; i<n; i++) {
		ivalue = rank * n + i;
		if (vecout[i] != ivalue) {
		    printf( "[%d] Expected %f but found %f for vecout[%d]\n", 
			    rank, ivalue, vecout[i], i );
		    errs++;
		}
	    }
	}
    }
    
    MTest_Finalize( errs );
    MPI_Type_free( &vec );
    MPI_Finalize();
    return 0;
}

