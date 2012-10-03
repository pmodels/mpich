/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h" 
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"

/* tests passive target RMA on 2 processes. tests the lock-single_op-unlock 
   optimization for less common cases:

   origin datatype derived, target datatype predefined

*/
int main(int argc, char *argv[]) 
{ 
    int          wrank, nprocs, *srcbuf, *rmabuf, i;
    int          memsize;
    MPI_Datatype vectype;
    MPI_Win      win;
    int          errs = 0;

    MTest_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&wrank); 

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    memsize = 10 * 4 * nprocs;
    /* Create and initialize data areas */
    srcbuf = (int *)malloc( sizeof(int) * memsize );
    MPI_Alloc_mem( sizeof(int) * memsize, MPI_INFO_NULL, &rmabuf );
    if (!srcbuf || !rmabuf) {
	printf( "Unable to allocate srcbuf and rmabuf of size %d\n", memsize );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    for (i=0; i<memsize; i++) {
      rmabuf[i] = -i;
      srcbuf[i] = i;
    }

    MPI_Win_create( rmabuf, memsize*sizeof(int), sizeof(int), MPI_INFO_NULL, 
		    MPI_COMM_WORLD, &win );

    /* Vector of 10 elements, separated by 4 */
    MPI_Type_vector( 10, 1, 4, MPI_INT, &vectype );
    MPI_Type_commit( &vectype );

    /* Accumulate with a derived origin type and target predefined type*/
    if (wrank == 0) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, 0, 0, win );
	for (i=0; i<10; i++) {
	    if (rmabuf[i] != -i + 4*i) {
		errs++;
		printf( "Acc: expected rmabuf[%d] = %d but saw %d\n", 
			i, -i + 4*i, rmabuf[i] );
	    }
	    rmabuf[i] = -i;
	}
	for (i=10; i<memsize; i++) {
	    if (rmabuf[i] != -i) {
		errs++;
		printf( "Acc: expected rmabuf[%d] = %d but saw %d\n", 
			i, -i, rmabuf[i] );
		rmabuf[i] = -i;
	    }
	}
	MPI_Win_unlock( 0, win );
    }
    else if (wrank == 1) {
	MPI_Win_lock( MPI_LOCK_SHARED, 0, 0, win );
	MPI_Accumulate( srcbuf, 1, vectype, 0, 0, 10, MPI_INT, MPI_SUM, win );
	MPI_Win_unlock( 0, win );
	MPI_Barrier( MPI_COMM_WORLD );
    }
    else {
	MPI_Barrier( MPI_COMM_WORLD );
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Put with a derived origin type and target predefined type*/
    if (wrank == 0) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, 0, 0, win );
	for (i=0; i<10; i++) {
	    if (rmabuf[i] != 4*i) {
		errs++;
		printf( "Put: expected rmabuf[%d] = %d but saw %d\n", 
			i, 4*i, rmabuf[i] );
	    }
	    rmabuf[i] = -i;
	}
	for (i=10; i<memsize; i++) {
	    if (rmabuf[i] != -i) {
		errs++;
		printf( "Put: expected rmabuf[%d] = %d but saw %d\n", 
			i, -i, rmabuf[i] );
		rmabuf[i] = -i;
	    }
	}
	MPI_Win_unlock( 0, win );
    }
    else if (wrank == 1) {
	MPI_Win_lock( MPI_LOCK_SHARED, 0, 0, win );
	MPI_Put( srcbuf, 1, vectype, 0, 0, 10, MPI_INT, win );
	MPI_Win_unlock( 0, win );
	MPI_Barrier( MPI_COMM_WORLD );
    }
    else {
	MPI_Barrier( MPI_COMM_WORLD );
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Put with a derived origin type and target predefined type, with 
       a get (see the move-to-end optimization) */
    if (wrank == 0) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, 0, 0, win );
	for (i=0; i<10; i++) {
	    if (rmabuf[i] != 4*i) {
		errs++;
		printf( "Put: expected rmabuf[%d] = %d but saw %d\n", 
			i, 4*i, rmabuf[i] );
	    }
	    rmabuf[i] = -i;
	}
	for (i=10; i<memsize; i++) {
	    if (rmabuf[i] != -i) {
		errs++;
		printf( "Put: expected rmabuf[%d] = %d but saw %d\n", 
			i, -i, rmabuf[i] );
		rmabuf[i] = -i;
	    }
	}
	MPI_Win_unlock( 0, win );
    }
    else if (wrank == 1) {
	int val;
	MPI_Win_lock( MPI_LOCK_SHARED, 0, 0, win );
	MPI_Get( &val, 1, MPI_INT, 0, 10, 1, MPI_INT, win );
	MPI_Put( srcbuf, 1, vectype, 0, 0, 10, MPI_INT, win );
	MPI_Win_unlock( 0, win );
	MPI_Barrier( MPI_COMM_WORLD );
	if (val != -10) {
	    errs++;
	    printf( "Get: Expected -10, got %d\n", val );
	}
    }
    else {
	MPI_Barrier( MPI_COMM_WORLD );
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Put with a derived origin type and target predefined type, with 
       a get already at the end (see the move-to-end optimization) */
    if (wrank == 0) {
	MPI_Barrier( MPI_COMM_WORLD );
	MPI_Win_lock( MPI_LOCK_EXCLUSIVE, 0, 0, win );
	for (i=0; i<10; i++) {
	    if (rmabuf[i] != 4*i) {
		errs++;
		printf( "Put: expected rmabuf[%d] = %d but saw %d\n", 
			i, 4*i, rmabuf[i] );
	    }
	    rmabuf[i] = -i;
	}
	for (i=10; i<memsize; i++) {
	    if (rmabuf[i] != -i) {
		errs++;
		printf( "Put: expected rmabuf[%d] = %d but saw %d\n", 
			i, -i, rmabuf[i] );
		rmabuf[i] = -i;
	    }
	}
	MPI_Win_unlock( 0, win );
    }
    else if (wrank == 1) {
	int val;
	MPI_Win_lock( MPI_LOCK_SHARED, 0, 0, win );
	MPI_Put( srcbuf, 1, vectype, 0, 0, 10, MPI_INT, win );
	MPI_Get( &val, 1, MPI_INT, 0, 10, 1, MPI_INT, win );
	MPI_Win_unlock( 0, win );
	MPI_Barrier( MPI_COMM_WORLD );
	if (val != -10) {
	    errs++;
	    printf( "Get: Expected -10, got %d\n", val );
	}
    }
    else {
	MPI_Barrier( MPI_COMM_WORLD );
    }

    MPI_Win_free( &win );
    MPI_Free_mem( rmabuf );
    free( srcbuf );
    MPI_Type_free( &vectype );

    MTest_Finalize(errs);
    MPI_Finalize(); 
    return 0; 
} 

