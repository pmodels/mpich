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
// Those that do need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include "mpitestcxx.h"

static char MTEST_Descrip[] = "A simple test of Comm_spawn, followed by intercomm merge";

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI::Intercomm      parentcomm, intercomm;
    MPI::Intracomm      intracomm, intracomm2, intracomm3;
    bool           isChild = false;
    MPI::Status    status;

    MTest_Init( );

    parentcomm = MPI::Comm::Get_parent();

    if (parentcomm == MPI::COMM_NULL) {
	/* Create 2 more processes */
	intercomm = MPI::COMM_WORLD.Spawn( "./spawnintrax", MPI::ARGV_NULL, np,
			MPI::INFO_NULL, 0, errcodes );
    }
    else 
	intercomm = parentcomm;

    /* We now have a valid intercomm */

    rsize = intercomm.Get_remote_size( );
    size  = intercomm.Get_size();
    rank  = intercomm.Get_rank();

    if (parentcomm == MPI::COMM_NULL) {
	/* Master */
	if (rsize != np) {
	    errs++;
	    cout << "Did not create " << np << " processes (got " <<
		rsize << ")\n";
	}
	if (rank == 0) {
	    for (i=0; i<rsize; i++) {
		intercomm.Send( &i, 1, MPI::INT, i, 0 );
	    }
	}
    }
    else {
	/* Child */
	isChild = true;
	if (size != np) {
	    errs++;
	    cout << "(Child) Did not create " << np << 
		" processes (got " << size << ")\n";
	}
	intercomm.Recv( &i, 1, MPI::INT, 0, 0, status );
	if (i != rank) {
	    errs++;
	    cout << "Unexpected rank on child " << rank << " (" <<
		i << ")\n";
	}
    }

    /* At this point, try to form the intracommunicator */
    intracomm = intercomm.Merge( isChild );

    /* Check on the intra comm */
    {
	int icsize, icrank, wrank;
	
	icsize = intracomm.Get_size();
	icrank = intracomm.Get_rank();
	wrank  = MPI::COMM_WORLD.Get_rank();

	if (icsize != rsize + size) {
	    errs++;
	    cout << "Intracomm rank " << icrank << " thinks size is " <<
		icsize << ", not " << rsize + size << "\n";
	}
	/* Make sure that the processes are ordered correctly */
	if (isChild) {
	    int psize;
	    psize = parentcomm.Get_remote_size();
	    if (icrank != psize + wrank ) {
		errs++;
		cout << "Intracomm rank " << icrank << 
		    " (from child) should have rank " << psize + wrank << "\n";
	    }
	}
	else {
	    if (icrank != wrank) {
		errs++;
		cout << "Intracomm rank " << icrank << 
		    " (from parent) should have rank " << wrank << "\n";
	    }
	}
    }

    /* At this point, try to form the intracommunicator, with the other 
     processes first */
    intracomm2 = intercomm.Merge( !isChild );

    /* Check on the intra comm */
    {
	int icsize, icrank, wrank;
	
	icsize = intracomm2.Get_size();
	icrank = intracomm2.Get_rank();
	wrank  = MPI::COMM_WORLD.Get_rank();

	if (icsize != rsize + size) {
	    errs++;
	    cout << "(2)Intracomm rank " << icrank << 
		" thinks size is " << icsize << ", not " << rsize + size << 
		"\n";
	}
	/* Make sure that the processes are ordered correctly */
	if (isChild) {
	    if (icrank != wrank ) {
		errs++;
		cout << "(2)Intracomm rank " << icrank << 
		    " (from child) should have rank " << wrank << "\n";
	    }
	}
	else {
	    int csize;
	    csize = intercomm.Get_remote_size();
	    if (icrank != wrank + csize) {
		errs++;
		cout << "(2)Intracomm rank " << icrank << 
		    " (from parent) should have rank " << wrank + csize << 
		    "\n";
	    }
	}
    }

    /* At this point, try to form the intracommunicator, with an 
       arbitrary choice for the first group of processes */
    intracomm3 = intercomm.Merge( false );
    /* Check on the intra comm */
    {
	int icsize, icrank, wrank;
	
	icsize = intracomm3.Get_size();
	icrank = intracomm3.Get_rank();
	wrank = MPI::COMM_WORLD.Get_rank();

	if (icsize != rsize + size) {
	    errs++;
	    cout << "(3)Intracomm rank " << icrank << 
		" thinks size is " << icsize << ", not " << rsize + size << 
		"\n";
	}
	/* Eventually, we should test that the processes are ordered 
	   correctly, by groups (must be one of the two cases above) */
    }

    /* Update error count */
    if (isChild) {
	/* Send the errs back to the master process */
	intercomm.Ssend( &errs, 1, MPI::INT, 0, 1 );
    }
    else {
	if (rank == 0) {
	    /* We could use intercomm reduce to get the errors from the 
	       children, but we'll use a simpler loop to make sure that
	       we get valid data */
	    for (i=0; i<rsize; i++) {
		intercomm.Recv( &err, 1, MPI::INT, i, 1 );
		errs += err;
	    }
	}
    }

    /* It isn't necessary to free the intracomms, but it should not hurt */
    intracomm.Free();
    intracomm2.Free();
    intracomm3.Free();

    /* It isn't necessary to free the intercomm, but it should not hurt */
    intercomm.Free();

    /* Note that the MTest_Finalize get errs only over COMM_WORLD */
    /* Note also that both the parent and child will generate "No Errors"
       if both call MTest_Finalize */
    if (parentcomm == MPI::COMM_NULL) {
	MTest_Finalize( errs );
    }

    MPI::Finalize();
    return 0;
}
