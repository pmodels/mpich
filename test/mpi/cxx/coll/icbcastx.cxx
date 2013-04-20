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

static char MTEST_Descrip[] = "Simple intercomm broadcast test";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int *buf = 0;
    int leftGroup, i, count, rank;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init();

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
        if (comm == MPI::COMM_NULL) continue;
	for (count = 1; count < 65000; count = 2 * count) {
	    buf = new int [count];
	    /* Get an intercommunicator */
	    if (leftGroup) {
		rank = comm.Get_rank();
		if (rank == 0) {
		    for (i=0; i<count; i++) buf[i] = i;
		}
		else {
		    for (i=0; i<count; i++) buf[i] = -1;
		}
		try {
		    comm.Bcast( buf, count, datatype, 
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
	    }
	    else {
		/* In the right group */
		for (i=0; i<count; i++) buf[i] = -1;
		try
		{
		    comm.Bcast( buf, count, datatype, 0 );
		}
		catch (MPI::Exception e)
		{
		    errs++;
		    MTestPrintError( e.Get_error_code() );
		}
		/* Check that we have received the correct data */
		for (i=0; i<count; i++) {
		    if (buf[i] != i) {
			errs++;
		    }
		}
	    }
	    delete [] buf;
	}
        MTestFreeComm(comm);
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
