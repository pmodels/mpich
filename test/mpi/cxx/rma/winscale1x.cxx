/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
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

/*
 * buf is a 2-d array, stored as a 1-d vector, with
 * buf(i,j) = buf[i + nrows*j], 0<=i<nrows, 0<=j<ncols+2
 * We store into cols 1<=j<=ncols, and put into the two
 * "ghost" columns.
 */
#define MAX_NROWS 25
#define MAX_NCOLS 12
int main( int argc, char *argv[] )
{
    MPI::Intracomm comm;
    int            errs = 0;
    MPI::Win       win;
    int            i, j, nrows=MAX_NROWS, ncols=MAX_NCOLS;
    int            left, right, ans, size, rank;
    int            buf[MAX_NROWS * (MAX_NCOLS + 2)];
    MPI::Aint      aint;
    MPI::Group     group, group2;
    int            nneighbors, nbrs[2];
         
    MTest_Init( );

    while( MTestGetIntracommGeneral( comm, 2, false ) ) {
	aint = nrows * (ncols + 2) * sizeof(int);
	win = MPI::Win::Create( buf, aint, sizeof(int) * nrows, 
				MPI::INFO_NULL, comm );
	size = comm.Get_size();
	rank = comm.Get_rank();

	// Create the group for the neighbors
	nneighbors = 0;
	left = rank - 1;
	if (left < 0)      left = MPI::PROC_NULL;
	else {
            nbrs[nneighbors++] = left;
	}
	right = rank + 1;
	if (right >= size) right = MPI::PROC_NULL;
	else {
            nbrs[nneighbors++] = right;
	}

	group = comm.Get_group();
	group2 = group.Incl( nneighbors, nbrs );
	group.Free();

	// Initialize the buffer 
	for (i=0; i<nrows; i++) {
	    buf[i]                 = -1;
	    buf[(ncols+1)*nrows+i] = -1;
	}
	for (j=0; j<ncols; j++) {
	    for (i=0; i<nrows; i++) {
		buf[i + (j+1) * nrows] = 
		    rank * (ncols * nrows) + i + j * nrows;
	    }
	}

	// The actual exchange
	win.Post( group2, 0 );
	win.Start( group2, 0 );

	win.Put( &buf[0+nrows], nrows, MPI::INT, left, ncols+1, 
		 nrows, MPI::INT );
	win.Put( &buf[0+ncols*nrows], nrows, MPI::INT, right, 0, 
		 nrows, MPI::INT );
	win.Complete( );
	win.Wait( );

	// Check the results
	if (left != MPI::PROC_NULL) {
	    for (i=0; i<nrows; i++) {
		ans = rank * (ncols * nrows) - nrows + i;
		if (buf[i] != ans) {
		    errs++;
		    if (errs != 10) {
			cout << rank << " buf[" << i << ",0] = " <<
			    buf[i] << " expected " << ans << "\n";
		    }
		}
	    }
	}
	if (right != MPI::PROC_NULL) {
	    for (i=0; i<nrows; i++) {
		ans = (rank + 1)* (ncols * nrows) + i;
		if (buf[i + (ncols+1)*nrows] != ans) {
		    errs++;
		    if (errs <= 10) {
			cout << rank << " buf[" << i << ",ncols+1] = " <<
			    buf[i+(ncols+1)*nrows] << " expected " << 
			    ans << "\n";
		    }
		}
	    }
	}

	
	group2.Free();
	win.Free();
	MTestFreeComm( comm );
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
