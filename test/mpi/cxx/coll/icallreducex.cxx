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

static char MTEST_Descrip[] = "Simple intercomm allreduce test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *sendbuf = 0, *recvbuf = 0;
    int leftGroup, i, count, rank, rsize;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init();

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
	if (comm == MPI::COMM_NULL) continue;
	rank = comm.Get_rank();
	for (count = 1; count < 65000; count = 2 * count) {
	    sendbuf = new int [ count ];
	    recvbuf = new int [ count ];
	    /* Get an intercommunicator */
	    if (leftGroup) {
		for (i=0; i<count; i++) sendbuf[i] = i;
	    }
	    else {
		for (i=0; i<count; i++) sendbuf[i] = -i;
	    }
	    for (i=0; i<count; i++) recvbuf[i] = 0;
	    try
	    {
		comm.Allreduce( sendbuf, recvbuf, count, datatype, MPI::SUM );
	    }
	    catch (MPI::Exception e)
	    {
		errs++;
		MTestPrintError( e.Get_error_code() );
	    }
	    /* In each process should be the sum of the values from the
	       other process */
	    rsize = comm.Get_remote_size();
	    if (leftGroup) {
		for (i=0; i<count; i++) {
		    if (recvbuf[i] != -i * rsize) {
			errs++;
			if (errs < 10) {
			    cout << "recvbuf[" << i << "] = " <<
				recvbuf[i] << "\n";
			}
		    }
		}
	    }
	    else {
		for (i=0; i<count; i++) {
		    if (recvbuf[i] != i * rsize) {
			errs++;
			if (errs < 10) {
			    cout << "recvbuf[" << i << "] = " << 
				recvbuf[i] << "\n";
			}
		    }
		}
	    }
	    delete [] sendbuf;
	    delete [] recvbuf;
	}
        MTestFreeComm(comm);
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
