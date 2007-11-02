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

static char MTEST_Descrip[] = "Simple intercomm alltoall test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *sendbuf = 0, *recvbuf = 0;
    int leftGroup, i, j, idx, count, rank, rsize;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init( );

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
	if (comm == MPI::COMM_NULL) continue;
	for (count = 1; count < 66000; count = 2 * count) {
	    /* Get an intercommunicator */
	    rsize = comm.Get_remote_size();
	    rank  = comm.Get_rank();
	    sendbuf = new int [ rsize * count ];
	    recvbuf = new int [ rsize * count ];
	    for (i=0; i<rsize*count; i++) recvbuf[i] = -1;
	    if (leftGroup) {
		idx = 0;
		for (j=0; j<rsize; j++) {
		    for (i=0; i<count; i++) {
			sendbuf[idx++] = i + rank;
		    }
		}
		try
		{
		    comm.Alltoall( sendbuf, count, datatype, NULL, 0, datatype );
		}
		catch (MPI::Exception e)
		{ 
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
	    }
	    else {
		int rank, size;

		rank = comm.Get_rank();
		size = comm.Get_size();

		/* In the right group */
		try
		{
		    comm.Alltoall( NULL, 0, datatype, recvbuf, count, datatype );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		/* Check that we have received the correct data */
		idx = 0;
		for (j=0; j<rsize; j++) {
		    for (i=0; i<count; i++) {
			if (recvbuf[idx++] != i + j) {
			    errs++;
			    if (errs < 10) 
				cout << "buf[" << i << "] = " <<
				    recvbuf[i] << " on " << rank << "\n";
			}
		    }
		}
	    }
	    delete [] recvbuf;
	    delete [] sendbuf;
	}
        MTestFreeComm(comm);
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
