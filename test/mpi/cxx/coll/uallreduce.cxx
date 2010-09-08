/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

void uop( const void *invec, void *inoutvec, int count, 
	  const MPI::Datatype &datatype )
{
    int i;
    int *cin = (int*)invec, *cout = (int*)inoutvec;

    for (i=0; i<count; i++) {
	cout[i] = cin[i] + cout[i];
    }
}

int main( int argc, char **argv )
{
    MPI::Op sumop;
    MPI::Intracomm comm = MPI::COMM_WORLD;
    int errs = 0;
    int size, i, count;

    MTest_Init( );

    sumop.Init( uop, true );
    size = comm.Get_size();
    
    for (count = 1; count < 66000; count = count * 2) {
	int *vin, *vout;
	vin  = new int[count];
	vout = new int[count];

	for (i=0; i<count; i++) {
	    vin[i]  = i;
	    vout[i] = -1;
	}
	comm.Allreduce( vin, vout, count, MPI::INT, sumop );
	for (i=0; i<count; i++) {
	    if (vout[i] != i * size) {
		errs++;
		if (errs < 10) 
		    cerr << "vout[" << i << "] = " << vout[i] << endl;
	    }
	}
	
	delete[] vin;
	delete[] vout;
    }
    
    sumop.Free();
    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
