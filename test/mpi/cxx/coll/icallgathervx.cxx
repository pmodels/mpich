/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <iostream>
#include "mpitestcxx.h"

static char MTEST_Descrip[] = "Simple intercomm allgatherv test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *rbuf = 0, *sbuf = 0;
    int *recvcounts, *recvdispls;
    int leftGroup, i, count, rank, rsize;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init( );

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
	if (comm == MPI::COMM_NULL) continue;
	for (count = 1; count < 65000; count = 2 * count) {
	    /* Get an intercommunicator */
	    /* The left group will send rank to the right group;
	       The right group will send -rank to the left group */
	    rank = comm.Get_rank();
	    rsize = comm.Get_remote_size();
	    rbuf = new int [ count * rsize ];
	    sbuf = new int [ count ];
	    recvcounts = new int [ rsize ];
	    recvdispls = new int [ rsize ];
	    for (i=0; i<count*rsize; i++) rbuf[i] = -1;
	    for (i=0; i<rsize; i++) {
		recvcounts[i] = count;
		recvdispls[i] = i * count;
	    }
	    if (leftGroup) {
		for (i=0; i<count; i++)       sbuf[i] = i + rank*count;
	    }
	    else {
		for (i=0; i<count; i++)       sbuf[i] = -(i + rank*count);
	    }
	    try
	    {
		comm.Allgatherv( sbuf, count, datatype,
		    rbuf, recvcounts, recvdispls, datatype );
	    }
	    catch (MPI::Exception e)
	    { 
		errs++;
		MTestPrintError( e.Get_error_code() );
	    }
	    if (leftGroup) {
		for (i=0; i<count*rsize; i++) {
		    if (rbuf[i] != -i) {
			errs++;
		    }
		}
	    }
	    else {
		for (i=0; i<count*rsize; i++) {
		    if (rbuf[i] != i) {
			errs++;
		    }
		}
	    }

	    /* Use Allgather in a unidirectional way */
	    for (i=0; i<count*rsize; i++) rbuf[i] = -1;
	    if (leftGroup) {
		try
		{
		    comm.Allgatherv( sbuf, 0, datatype,
			rbuf, recvcounts, recvdispls, datatype );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		for (i=0; i<count*rsize; i++) {
		    if (rbuf[i] != -i) {
			errs++;
		    }
		}
	    }
	    else {
                for (i=0; i<rsize; i++) {
                    recvcounts[i] = 0;
                    recvdispls[i] = 0;
                }
		try
		{
		    comm.Allgatherv( sbuf, count, datatype,
			rbuf, recvcounts, recvdispls, datatype );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		for (i=0; i<count*rsize; i++) {
		    if (rbuf[i] != -1) {
			errs++;
		    }
		}
	    }
	    delete [] rbuf;
	    delete [] sbuf;
	    delete [] recvcounts;
	    delete [] recvdispls;
        }
        MTestFreeComm(comm);
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
