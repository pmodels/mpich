/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <iostream>
#include "mpitestcxx.h"

static char MTEST_Descrip[] = "Simple intercomm allgather test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *rbuf = 0, *sbuf = 0;
    int leftGroup, i, count, rank, rsize;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init( );

    datatype = MPI_INT;
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
	    for (i=0; i<count*rsize; i++) rbuf[i] = -1;
	    if (leftGroup) {
		for (i=0; i<count; i++)       sbuf[i] = i + rank*count;
	    }
	    else {
		for (i=0; i<count; i++)       sbuf[i] = -(i + rank*count);
	    }
	    try
	    {
		comm.Allgather( sbuf, count, datatype, rbuf, count, datatype );
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
		    comm.Allgather( sbuf, 0, datatype, rbuf, count, datatype );
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
		try
		{
		    comm.Allgather( sbuf, count, datatype, rbuf, 0, datatype );
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
	}
        MTestFreeComm(comm);
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
