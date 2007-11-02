/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitestcxx.h"

static int verbose = 0;

static int ncalls = 0;
void efn( MPI::Comm &comm, int *code, ... )
{
    ncalls ++;
}

int main( int argc, char *argv[] )
{
    MPI::Errhandler eh;
    char *filename;
    int size;
    bool foundMsg;
    int errs = 0, toterrs, rank;
    int sawErr;

    MPI::Init();

    size = MPI::COMM_WORLD.Get_size();

    // Test that we can change the default error handler to not throw
    // an exception (our error handler could also throw an exception,
    // but we want to make sure that the exception is not thrown 
    // by the MPI code)

    eh = MPI::Comm::Create_errhandler( efn );
    MPI::COMM_WORLD.Set_errhandler( eh );
    sawErr = 0;
    try {
	foundMsg = MPI::COMM_WORLD.Iprobe( size, 0 );
    } catch (MPI::Exception ex) {
	cout << "Caught exception from iprobe (should have called error handler instead)\n";
	errs++;
	if (ex.Get_error_class() == MPI_SUCCESS) {
	    errs++;
	    cout << "Unexpected error from Iprobe" << endl;
	}
	sawErr = 1;
    }
    if (ncalls != 1) {
	errs++;
	cout << "Did not invoke error handler when invoking iprobe with an invalid rank" << endl;
    }

    // Find out how many errors we saw
    MPI::COMM_WORLD.Allreduce( &errs, &toterrs, 1, MPI::INT, MPI::SUM );
    if (MPI::COMM_WORLD.Get_rank() == 0) {
	if (toterrs == 0) {
	    cout << " No Errors" << endl;
	}
	else {
	    cout << " Saw " << toterrs << " errors" << endl;
	}
    }

    MPI::Finalize();

    return 0;
}
