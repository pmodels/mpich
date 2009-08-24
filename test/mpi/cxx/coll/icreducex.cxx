/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <iostream>
#include "mpitestcxx.h"

static char MTEST_Descrip[] = "Simple intercomm reduce test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *sendbuf = 0, *recvbuf=0;
    int leftGroup, i, count, rank;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init();

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
        if (comm == MPI::COMM_NULL) continue;
	for (count = 1; count < 65000; count = 2 * count) {
	    sendbuf = new int [count];
	    recvbuf = new int [count];
	    /* Get an intercommunicator */
	    for (i=0; i<count; i++) {
		sendbuf[i] = -1;
		recvbuf[i] = -1;
	    }
	    if (leftGroup) {
		rank = comm.Get_rank();
		try
		{
		    comm.Reduce( sendbuf, recvbuf, count, datatype, MPI::SUM,
			(rank == 0) ? MPI::ROOT : MPI::PROC_NULL );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		/* Test that no other process in this group received the 
		   broadcast, and that we got the right answers */
		if (rank == 0) {
		    int rsize;
		    rsize = comm.Get_remote_size();
		    for (i=0; i<count; i++) {
			if (recvbuf[i] != i * rsize) {
			    errs++;
			}
		    }
		}
		else {
		    for (i=0; i<count; i++) {
			if (recvbuf[i] != -1) {
			    errs++;
			}
		    }
		}
	    }
	    else {
		/* In the right group */
		for (i=0; i<count; i++) sendbuf[i] = i;
		try
		{
		    comm.Reduce( sendbuf, recvbuf, count, datatype, MPI::SUM, 0 );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		/* Check that we have received no data */
		for (i=0; i<count; i++) {
		    if (recvbuf[i] != -1) {
			errs++;
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
