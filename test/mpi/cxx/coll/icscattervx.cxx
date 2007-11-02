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

static char MTEST_Descrip[] = "Simple intercomm scatterv test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *buf = 0;
    int *sendcounts;
    int *senddispls;
    int leftGroup, i, count, rank, rsize;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init( );

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
	if (comm == MPI::COMM_NULL) continue;
	rsize = comm.Get_remote_size();
	for (count = 1; count < 65000; count = 2 * count) {
	    /* Get an intercommunicator */
	    buf = 0;
	    sendcounts = new int [ rsize ];
	    senddispls = new int [ rsize ];
	    for (i=0; i<rsize; i++) {
		sendcounts[i] = count;
		senddispls[i] = count * i;
	    }
	    if (leftGroup) {
		rank = comm.Get_rank();

		buf = new int [count*rsize];
		if (rank == 0) {
		    for (i=0; i<count*rsize; i++) buf[i] = i;
		}
		else {
		    for (i=0; i<count*rsize; i++) buf[i] = -1;
		}
		try
		{
		    comm.Scatterv( buf, sendcounts, senddispls, datatype, 
			NULL, 0, datatype,
			(rank == 0) ? MPI::ROOT : MPI::PROC_NULL );
		}
		catch (MPI::Exception e)
		{ 
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		/* Test that no other process in this group received the 
		   scatter */
		if (rank != 0) {
		    for (i=0; i<count*rsize; i++) {
			if (buf[i] != -1) {
			    if (errs < 10) {
				cout << "Received data on root group!\n";
			    }
			    errs++;
			}
		    }
		}
	    }
	    else {
		int rank, size;
		rank = comm.Get_rank();
		size = comm.Get_size();

		buf = new int [ count ];
		/* In the right group */
		for (i=0; i<count; i++) buf[i] = -1;
		try
		{
		    comm.Scatterv( NULL, 0, 0, datatype, 
			buf, count, datatype, 0 );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		/* Check that we have received the correct data */
		for (i=0; i<count; i++) {
		    if (buf[i] != i + rank * count) {
			if (errs < 10) 
			    cout << "buf[" << i << "] = " << buf[i] << 
				" on " << rank << "\n";
			errs++;
		    }
		}
	    }
	    delete [] sendcounts;
	    delete [] senddispls;
	    delete [] buf;
	}
        MTestFreeComm(comm);
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
