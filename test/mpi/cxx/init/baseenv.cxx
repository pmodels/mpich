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


int main( int argc, char **argv )
{
    bool flag;
    int errs = 0;
    int ver, subver;
    int rank, size;

    if (MPI::Is_initialized()) {
	cout << "Is_initialized returned true before init\n";
	errs ++;
    }

    MPI::Init();

    rank = MPI::COMM_WORLD.Get_rank();
    size = MPI::COMM_WORLD.Get_size();

    if (!MPI::Is_initialized()) {
	cout << "Is_initialized returned false after init\n";
	errs ++;
    }

    MPI::Get_version( ver, subver );
    if (ver != MPI_VERSION || subver != MPI_SUBVERSION) {
	cout << "Inconsistent values for version and/or subversion\n";
	errs++;
    }

    if (MPI::Is_finalized()) {
	cout << "Is_finalized returned true before finalize\n";
	errs ++;
    }

    MPI::Finalize();

    if (!MPI::Is_finalized()) {
	cout << "Is_finalized returned false after finalize\n";
	errs ++;
    }

    // Ignore the other processes for this test, particularly
    // since we need to execute code after the Finalize
    if (rank == 0) {
	if (errs) {
	    cout << " Found " << errs << " errors\n";
	}
	else {
	    cout << " No Errors\n"; 
	}
    }
    return 0;
}
