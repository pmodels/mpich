/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <iostream>
#include "mpitestconf.h"
#include "mpitestcxx.h"

static char MTEST_Descrip[] = "Simple intercomm gatherv test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *buf = 0;
    int *recvcounts;
    int *recvdispls;
    int leftGroup, i, count, rank, rsize;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init( );

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
	if (comm == MPI::COMM_NULL) continue;
	for (count = 1; count < 65000; count = 2 * count) {
	    /* Get an intercommunicator */
	    rsize = comm.Get_remote_size();
	    recvcounts = new int [ rsize ];
	    recvdispls = new int [ rsize ];
	    /* This simple test duplicates the Gather test, 
	       using the same lengths for all messages */
	    for (i=0; i<rsize; i++) {
		recvcounts[i] = count;
		recvdispls[i] = count * i;
	    }
	    if (leftGroup) {
		rank = comm.Get_rank();
		buf = new int [count * rsize];
		for (i=0; i<count*rsize; i++) buf[i] = -1;

		try
		{
    		    comm.Gatherv( NULL, 0, datatype,
    				  buf, recvcounts, recvdispls, datatype, 
				 (rank == 0) ? MPI::ROOT : MPI::PROC_NULL );
		}
		catch (MPI::Exception e)
		{ 
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		/* Test that no other process in this group received the 
		   broadcast */
		if (rank != 0) {
		    for (i=0; i<count; i++) {
			if (buf[i] != -1) {
			    errs++;
			}
		    }
		}
		else {
		    /* Check for the correct data */
		    for (i=0; i<count*rsize; i++) {
			if (buf[i] != i) {
			    errs++;
			}
		    }
		}
	    }
	    else {
		int size;
		/* In the right group */
		rank = comm.Get_rank();
		size = comm.Get_size();
		buf = new int [count];
		for (i=0; i<count; i++) buf[i] = rank * count + i;
		try
		{
		    comm.Gatherv( buf, count, datatype, NULL, 0, 0, datatype, 0 );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
	    }
	    delete [] buf;
	    delete [] recvcounts;
	    delete [] recvdispls;
	}
        MTestFreeComm(comm);
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
