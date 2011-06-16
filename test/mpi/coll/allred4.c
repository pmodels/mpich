/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Test MPI_Allreduce with non-commutative user-defined operations using matrix rotations";
*/

/* This example is similar to allred3.c, but uses only 3x3 matrics with 
   integer-valued entries.  This is an associative but not commutative
   operation.
   The number of matrices is the count argument. The matrix is stored
   in C order, so that
     c(i,j) is cin[j+i*3]

   Three different matrices are used:
   I = identity matrix
   A = (1 0 0    B = (0 1 0
        0 0 1         1 0 0
        0 1 0)        0 0 1)

   The product 

         I^k A I^(p-2-k-j) B I^j

   is 

   ( 0 1 0 
     0 0 1
     1 0 0 )

   for all values of k, p, and j.  
 */

void matmult( void *cinPtr, void *coutPtr, int *count, MPI_Datatype *dtype );

void matmult( void *cinPtr, void *coutPtr, int *count, MPI_Datatype *dtype )
{
    const int *cin = (const int *)cinPtr;
    int *cout = (int *)coutPtr;
    int i, j, k, nmat;
    int tempcol[3];
    int offset1, offset2;

    for (nmat = 0; nmat < *count; nmat++) {
	for (j=0; j<3; j++) {
	    for (i=0; i<3; i++) {
		tempcol[i] = 0;
		for (k=0; k<3; k++) {
		    /* col[i] += cin(i,k) * cout(k,j) */
		    offset1 = k+i*3;
		    offset2 = j+k*3;
		    tempcol[i] += cin[offset1] * cout[offset2];
		}
	    }
	    for (i=0; i<3; i++) {
		offset1 = j+i*3;
		cout[offset1] = tempcol[i];
	    }
	}
	/* Advance to the next matrix */
	cin += 9;
	cout += 9;
    }
}

/* Initialize the integer matrix as one of the 
   above matrix entries, as a function of count.
   We guarantee that both the A and B matrices are included.
*/   
static void initMat( int rank, int size, int nmat, int mat[] )
{
    int i, kind;

    /* Zero the matrix */
    for (i=0; i<9; i++) {
	mat[i] = 0;
    }

    /* Decide which matrix to create (I, A, or B) */
    if ( size == 2) {
	/* rank 0 is A, 1 is B */
	kind = 1 + rank;
    }
    else {
	int tmpA, tmpB;
	/* Most ranks are identity matrices */
	kind = 0;
	/* Make sure exactly one rank gets the A matrix
	   and one the B matrix */
	tmpA = size / 4;
	tmpB = (3 * size) / 4;
	
	if (rank == tmpA) kind = 1;
	if (rank == tmpB) kind = 2;
    }
    
    switch (kind) {
    case 0: /* Identity */
	mat[0] = 1;
	mat[4] = 1;
	mat[8] = 1;
	break;
    case 1: /* A */
	mat[0] = 1;
	mat[5] = 1;
	mat[7] = 1;
	break;
    case 2: /* B */
	mat[1] = 1;
	mat[3] = 1;
	mat[8] = 1;
	break;
    }
}

/* Compare a matrix with the known result */
static int checkResult( int nmat, int mat[], const char *msg )
{
    int n, k, errs = 0, wrank;
    static int solution[9] = { 0, 1, 0, 
                               0, 0, 1, 
                               1, 0, 0 };

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );

    for (n=0; n<nmat; n++) {
	for (k=0; k<9; k++) {
	    if (mat[k] != solution[k]) {
		errs ++;
		if (errs == 1) {
		    printf( "Errors for communicators %s\n", 
			    MTestGetIntracommName() ); fflush(stdout);

		}
		if (errs < 10) {
		    printf( "[%d]matrix #%d(%s): Expected mat[%d,%d] = %d, got %d\n",
			    wrank, n, msg, k / 3, k % 3, solution[k], mat[k] );
		    fflush(stdout);
		}
	    }
	}
	/* Advance to the next matrix */
	mat += 9;
    }
    return errs;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size, rank;
    int minsize = 2, count; 
    MPI_Comm      comm;
    int *buf, *bufout;
    MPI_Op op;
    MPI_Datatype mattype;
    int i;

    MTest_Init( &argc, &argv );

    MPI_Op_create( matmult, 0, &op );
    
    /* A single rotation matrix (3x3, stored as 9 consequetive elements) */
    MPI_Type_contiguous( 9, MPI_INT, &mattype );
    MPI_Type_commit( &mattype );

    /* Sanity check: test that our routines work properly */
    { int one = 1;
    buf = (int *)malloc( 4*9 * sizeof(int) );
    initMat( 0, 4, 0, &buf[0] );
    initMat( 1, 4, 0, &buf[9] );
    initMat( 2, 4, 0, &buf[18] );
    initMat( 3, 4, 0, &buf[27] );
    matmult( &buf[0], &buf[9], &one, &mattype );
    matmult( &buf[9], &buf[18], &one, &mattype );
    matmult( &buf[18], &buf[27], &one, &mattype );
    checkResult( 1, &buf[27], "Sanity Check" );
    free(buf);
    }
    
    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;

	MPI_Comm_size( comm, &size );
	MPI_Comm_rank( comm, &rank );

	for (count = 1; count < size; count ++ ) {
	    
	    /* Allocate the matrices */
	    buf = (int *)malloc( count * 9 * sizeof(int) );
	    if (!buf) {
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }

	    bufout = (int *)malloc( count * 9 * sizeof(int) );
	    if (!bufout) {
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }

	    for (i=0; i < count; i++) {
		initMat( rank, size, i, &buf[i*9] );
	    }
	    
	    MPI_Allreduce( buf, bufout, count, mattype, op, comm );
	    errs += checkResult( count, bufout, "" );

	    /* Try the same test, but using MPI_IN_PLACE */
	    for (i=0; i < count; i++) {
		initMat( rank, size, i, &bufout[i*9] );
	    }
	    MPI_Allreduce( MPI_IN_PLACE, bufout, count, mattype, op, comm );
	    errs += checkResult( count, bufout, "IN_PLACE" );

	    free( buf );
	    free( bufout );
	}
	MTestFreeComm( &comm );
    }
	
    MPI_Op_free( &op );
    MPI_Type_free( &mattype );

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
