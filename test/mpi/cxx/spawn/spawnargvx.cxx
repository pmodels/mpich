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
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static char MTEST_Descrip[] = "A simple test of Comm_spawn, with complex arguments";

int main( int argc, char *argv[] )
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI::Intercomm      parentcomm, intercomm;
    MPI::Status         status;
    const char * inargv[]  = { "a", "b=c", "d e", "-pf", " Ss", 0 };
    const char * outargv[] = { "a", "b=c", "d e", "-pf", " Ss", 0 };

    MTest_Init( );

    parentcomm = MPI::Comm::Get_parent();

    if (parentcomm == MPI::COMM_NULL) {
	/* Create 2 more processes */
	/* ./ is unix specific .
	   The more generic approach would be to specify "spawnargv" as the 
	   executable and pass an info with ("path", ".") */
	intercomm = MPI::COMM_WORLD.Spawn( "./spawnargvx", inargv, np,
			MPI::INFO_NULL, 0, errcodes );
    }
    else 
	intercomm = parentcomm;

    /* We now have a valid intercomm */

    rsize = intercomm.Get_remote_size();
    size  = intercomm.Get_size();
    rank  = intercomm.Get_rank();

    if (parentcomm == MPI::COMM_NULL) {
	/* Master */
	if (rsize != np) {
	    errs++;
	    cout << "Did not create " << np << " processes (got " <<
		rsize << ")\n";
	}
	for (i=0; i<rsize; i++) {
	    intercomm.Send( &i, 1, MPI::INT, i, 0 );
	}
	/* We could use intercomm reduce to get the errors from the 
	   children, but we'll use a simpler loop to make sure that
	   we get valid data */
	for (i=0; i<rsize; i++) {
	    intercomm.Recv( &err, 1, MPI::INT, i, 1 );
	    errs += err;
	}
    }
    else {
	/* Child */
	/* FIXME: This assumes that stdout is handled for the children
	   (the error count will still be reported to the parent) */
	if (size != np) {
	    errs++;
	    cout << "(Child) Did not create " << np << " (got " <<
		size << ")\n";
	}
	intercomm.Recv( &i, 1, MPI::INT, 0, 0, status );
	if (i != rank) {
	    errs++;
	    cout << "Unexpected rank on child " << rank << " (" << 
		i << "\n";
	}
	/* Check the command line */
	for (i=1; i<argc; i++) {
	    if (!outargv[i-1]) {
		errs++;
		cout << "Wrong number of arguments (" << argc << ")\n";
		break;
	    }
	    if (strcmp( argv[i], outargv[i-1] ) != 0) {
		errs++;
		cout << "Found arg " << argv[i] << " but expected " <<
		    outargv[i-1] << "\n";
	    }
	}
	if (outargv[i-1]) {
	    /* We had too few args in the spawned command */
	    errs++;
	    cout << "Too few arguments to spawned command\n";
	}
	/* Send the errs back to the master process */
	intercomm.Ssend( &errs, 1, MPI::INT, 0, 1 );
    }

    /* It isn't necessary to free the intercomm, but it should not hurt */
    intercomm.Free();

    /* Note that the MTest_Finalize get errs only over COMM_WORLD */
    if (parentcomm == MPI::COMM_NULL) {
	MTest_Finalize( errs );
    }

    MPI::Finalize();
    return 0;
}
