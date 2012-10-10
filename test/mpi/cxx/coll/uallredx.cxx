/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include "mpitestcxx.h"
#include <assert.h>

static char MTEST_Descrip[] = "Test MPI_Allreduce with non-commutative user-defined operations";

/* We make the error count global so that we can easily control the output
   of error information (in particular, limiting it after the first 10 
   errors */
int errs = 0;

/* This implements a simple matrix-matrix multiply.  This is an associative
   but not commutative operation.  The matrix size is set in matSize;
   the number of matrices is the count argument. The matrix is stored
   in C order, so that
     c(i,j) is cin[j+i*matSize]
 */
#define MAXCOL 256
static int matSize = 0;  /* Must be < MAXCOL */
static int max_offset = 0;
void uop( const void *cinPtr, void *coutPtr, int count, const MPI::Datatype &dtype )
{
    const int *cin = (const int *)cinPtr;
    int *cout = (int *)coutPtr;
    int i, j, k, nmat;
    int tempcol[MAXCOL];
    int offset1, offset2;
    int matsize2 = matSize*matSize;

    for (nmat = 0; nmat < count; nmat++) {
	for (j=0; j<matSize; j++) {
	    for (i=0; i<matSize; i++) {
		tempcol[i] = 0;
		for (k=0; k<matSize; k++) {
		    /* col[i] += cin(i,k) * cout(k,j) */
		    offset1    = k+i*matSize;
		    offset2    = j+k*matSize;
		    assert(offset1 < max_offset);
		    assert(offset2 < max_offset);
		    tempcol[i] += cin[offset1] * cout[offset2];
		}
	    }
	    for (i=0; i<matSize; i++) {
		offset1       = j+i*matSize;
		assert(offset1 < max_offset);
		cout[offset1] = tempcol[i];
	    }
	}
	cin  += matsize2;
	cout += matsize2;
    }
}

/* Initialize the integer matrix as a permutation of rank with rank+1.
   If we call this matrix P_r, we know that product of P_0 P_1 ... P_{size-2}
   is the the matrix representing the permutation that shifts left by one.
   As the final matrix (in the size-1 position), we use the matrix that
   shifts RIGHT by one
*/   
static void initMat( MPI::Intracomm comm, int mat[] )
{
    int i, j, size, rank;
    int offset;
    
    rank = comm.Get_rank();
    size = comm.Get_size();

    for (i=0; i<size*size; i++) {
	assert(i < max_offset);
	mat[i] = 0;
    }

    if (rank < size-1) {
	/* Create the permutation matrix that exchanges r with r+1 */
	for (i=0; i<size; i++) {
	    if (i == rank) {
		offset = ((i+1)%size) + i * size;
		assert(offset < max_offset);
		mat[offset] = 1;
	    }
	    else if (i == ((rank + 1)%size)) {
		offset = ((i+size-1)%size) + i * size;
		assert(offset < max_offset);
		mat[offset] = 1;
	    }
	    else {
		offset = i+i*size;
		assert(offset < max_offset);
		mat[offset] = 1;
	    }
	}
    }
    else {
	/* Create the permutation matrix that shifts right by one */
	for (i=0; i<size; i++) {
	    for (j=0; j<size; j++) {
		offset = j + i * size;  /* location of c(i,j) */
		mat[offset] = 0;
		if ( ((j-i+size)%size) == 1 ) mat[offset] = 1;
	    }
	}
	
    }
}

/* Compare a matrix with the identity matrix */
static int isIdentity( MPI::Intracomm comm, int mat[] )
{
    int i, j, size, rank, lerrs = 0;
    int offset;

    rank = comm.Get_rank();
    size = comm.Get_size();

    for (i=0; i<size; i++) {
	for (j=0; j<size; j++) {
	    if (i == j) {
		offset = j+i*size;
		assert(offset < max_offset);
		if (mat[offset] != 1) {
		    lerrs++;
		    if (errs + lerrs< 10) {
			cerr << "[" << rank << "] mat[" << i << "," << j << "] = " << mat[offset] << ", expected 1 for comm " << MTestGetIntracommName() << endl;
		    }
		}
	    }
	    else {
		offset = j+i*size;
		assert(offset < max_offset);
		if (mat[offset] != 0) {
		    lerrs++;
		    if (errs + lerrs< 10) {
			cerr << "[" << rank << "] mat[" << i << "," << j << "] = " << mat[offset] << ", expected 0 for comm " << MTestGetIntracommName() << endl;
		    }
		}
	    }
	}
    }
    return lerrs;
}

int main( int argc, char *argv[] )
{
    int size;
    int minsize = 2, count; 
    MPI::Intracomm      comm;
    int *buf, *bufout;
    MPI::Op op;
    MPI::Datatype mattype;

    MTest_Init();

    op.Init(uop, false);

    while (MTestGetIntracommGeneral( comm, minsize, 1 )) {
	if ((MPI::Intracomm)comm == (MPI::Intracomm)MPI_COMM_NULL) {
	    continue;
	}
	size = comm.Get_size();
	matSize = size;

	/* Only one matrix for now */
	count = 1;

	/* A single matrix, the size of the communicator */
	mattype = MPI::INT.Create_contiguous( size*size );
	mattype.Commit();

	max_offset = count * size * size;
	buf = new int[max_offset];
	if (!buf) {
	    MPI::COMM_WORLD.Abort( 1 );
	}
	bufout = new int[max_offset];
	if (!bufout) {
	    MPI::COMM_WORLD.Abort( 1 );
	}

	initMat( comm, buf );
	comm.Allreduce( buf, bufout, count, mattype, op );
	errs += isIdentity( comm, bufout );

	/* Try the same test, but using MPI_IN_PLACE */
	initMat( comm, bufout );
	comm.Allreduce( MPI_IN_PLACE, bufout, count, mattype, op );
	errs += isIdentity( comm, bufout );

	delete [] buf;
	delete [] bufout;

	mattype.Free();
	MTestFreeComm( comm );
    }

    op.Free();

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
