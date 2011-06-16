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
static char MTEST_Descrip[] = "Test MPI_Reduce with non-commutative user-define operations and arbitrary root";
*/

/*
 * This tests that the reduce operation respects the noncommutative flag.
 * and that can distinguish between P_{root} P_{root+1} 
 * ... P_{root-1} and P_0 ... P_{size-1} .  The MPI standard clearly
 * specifies that the result is P_0 ... P_{size-1}, independent of the root 
 * (see 4.9.4 in MPI-1)
 */

/* This implements a simple matrix-matrix multiply.  This is an associative
   but not commutative operation.  The matrix size is set in matSize;
   the number of matrices is the count argument. The matrix is stored
   in C order, so that
     c(i,j) is cin[j+i*matSize]
 */
#define MAXCOL 256
static int matSize = 0;  /* Must be < MAXCOL */

void uop( void *cinPtr, void *coutPtr, int *count, MPI_Datatype *dtype );
void uop( void *cinPtr, void *coutPtr, int *count, MPI_Datatype *dtype )
{
    const int *cin;
    int       *cout;
    int       i, j, k, nmat;
    int       tempCol[MAXCOL];

    if (*count != 1) printf( "Panic!\n" );
    for (nmat = 0; nmat < *count; nmat++) {
	cin  = (const int *)cinPtr;
	cout = (int *)coutPtr;
	for (j=0; j<matSize; j++) {
	    for (i=0; i<matSize; i++) {
		tempCol[i] = 0;
		for (k=0; k<matSize; k++) {
		    /* col[i] += cin(i,k) * cout(k,j) */
		    tempCol[i] += cin[k+i*matSize] * cout[j+k*matSize];
		}
	    }
	    for (i=0; i<matSize; i++) {
		cout[j+i*matSize] = tempCol[i];
	    }
	}
	cinPtr = (int *)cinPtr + matSize*matSize;
	coutPtr = (int *)coutPtr + matSize*matSize;
    }
}

/* Initialize the integer matrix as a permutation of rank with rank+1.
   If we call this matrix P_r, we know that product of P_0 P_1 ... P_{size-1}
   is the matrix with rows ordered as
   1,size,2,3,4,...,size-1
   (The matrix is basically a circular shift right, 
   shifting right n-1 steps for an n x n dimensional matrix, with the last
   step swapping rows 1 and size)
*/   

static void initMat( MPI_Comm comm, int mat[] )
{
    int i, size, rank;
    
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    /* Remember the matrix size */
    matSize = size;

    for (i=0; i<matSize*matSize; i++) mat[i] = 0;

    for (i=0; i<matSize; i++) {
	if (i == rank)                   
	    mat[((i+1)%matSize) + i * matSize] = 1;
	else if (i == ((rank + 1)%matSize)) 
	    mat[((i+matSize-1)%matSize) + i * matSize] = 1;
	else                             
	    mat[i+i*matSize] = 1;
    }
}

/* Compare a matrix with the identity matrix */
/*
static int isIdentity( MPI_Comm comm, int mat[] )
{
    int i, j, size, rank, errs = 0;
    
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    for (i=0; i<size; i++) {
	for (j=0; j<size; j++) {
	    if (j == i) {
		if (mat[j+i*size] != 1) {
		    printf( "mat(%d,%d) = %d, should = 1\n", 
			    i, j, mat[j+i*size] );
		    errs++;
		}
	    }
	    else {
		if (mat[j+i*size] != 0) {
		    printf( "mat(%d,%d) = %d, should = 0\n",
			    i, j, mat[j+i*size] );
		    errs++;
		}
	    }
	}
    }
    return errs;
}
*/

/* Compare a matrix with the identity matrix with rows permuted to as rows
   1,size,2,3,4,5,...,size-1 */
static int isPermutedIdentity( MPI_Comm comm, int mat[] )
{
    int i, j, size, rank, errs = 0;
    
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    /* Check the first two last rows */
    i = 0;
    for (j=0; j<size; j++) {
	if (j==0) { 
	    if (mat[j] != 1) {
		printf( "mat(%d,%d) = %d, should = 1\n", 
			i, j, mat[j] );
		errs++;
	    }
	}
	else {
	    if (mat[j] != 0) {
		printf( "mat(%d,%d) = %d, should = 0\n", 
			i, j, mat[j] );
		errs++;
	    }
	}
    }
    i = 1;
    for (j=0; j<size; j++) {
	if (j==size-1) { 
	    if (mat[j+i*size] != 1) {
		printf( "mat(%d,%d) = %d, should = 1\n", 
			i, j, mat[j+i*size] );
		errs++;
	    }
	}
	else {
	    if (mat[j+i*size] != 0) {
		printf( "mat(%d,%d) = %d, should = 0\n", 
			i, j, mat[j+i*size] );
		errs++;
	    }
	}
    }
    /* The remaint rows are shifted down by one */
    for (i=2; i<size; i++) {
	for (j=0; j<size; j++) {
	    if (j == i-1) {
		if (mat[j+i*size] != 1) {
		    printf( "mat(%d,%d) = %d, should = 1\n", 
			    i, j, mat[j+i*size] );
		    errs++;
		}
	    }
	    else {
		if (mat[j+i*size] != 0) {
		    printf( "mat(%d,%d) = %d, should = 0\n",
			    i, j, mat[j+i*size] );
		    errs++;
		}
	    }
	}
    }
    return errs;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, root;
    int minsize = 2, count; 
    MPI_Comm      comm;
    int *buf, *bufout;
    MPI_Op op;
    MPI_Datatype mattype;

    MTest_Init( &argc, &argv );

    MPI_Op_create( uop, 0, &op );
    
    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
	MPI_Comm_size( comm, &size );
	MPI_Comm_rank( comm, &rank );

	if (size > MAXCOL) {
	    /* Skip because there are too many processes */
	    MTestFreeComm( &comm );
	    continue;
	}

	/* Only one matrix for now */
	count = 1;

	/* A single matrix, the size of the communicator */
	MPI_Type_contiguous( size*size, MPI_INT, &mattype );
	MPI_Type_commit( &mattype );
	
	buf = (int *)malloc( count * size * size * sizeof(int) );
	if (!buf) MPI_Abort( MPI_COMM_WORLD, 1 );
	bufout = (int *)malloc( count * size * size * sizeof(int) );
	if (!bufout) MPI_Abort( MPI_COMM_WORLD, 1 );

	for (root = 0; root < size; root ++) {
	    initMat( comm, buf );
	    MPI_Reduce( buf, bufout, count, mattype, op, root, comm );
	    if (rank == root) {
		errs += isPermutedIdentity( comm, bufout );
	    }

	    /* Try the same test, but using MPI_IN_PLACE */
	    initMat( comm, bufout );
	    if (rank == root) {
		MPI_Reduce( MPI_IN_PLACE, bufout, count, mattype, op, root, comm );
	    }
	    else {
		MPI_Reduce( bufout, NULL, count, mattype, op, root, comm );
	    }
	    if (rank == root) {
		errs += isPermutedIdentity( comm, bufout );
	    }
	}
	MPI_Type_free( &mattype );

	free( buf );
	free( bufout );

	MTestFreeComm( &comm );
    }

    MPI_Op_free( &op );

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
