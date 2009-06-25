/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpitestconf.h"
#include "mpi.h"
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

static int ncalls = 0;
void efn( MPI::Comm &comm, int *code, ... )
{
    ncalls ++;
}

int main( int argc, char *argv[] )
{
    MPI::Errhandler eh;
    int size;
    bool foundMsg;
    int errs = 0;

    MTest_Init( );

    size = MPI::COMM_WORLD.Get_size();

    // Test that we can change the default error handler to not throw
    // an exception (our error handler could also throw an exception,
    // but we want to make sure that the exception is not thrown 
    // by the MPI code)

    eh = MPI::Comm::Create_errhandler( efn );
    MPI::COMM_WORLD.Set_errhandler( eh );
    try {
	foundMsg = MPI::COMM_WORLD.Iprobe( size, 0 );
    } catch (MPI::Exception ex) {
	cout << "Caught exception from iprobe (should have called error handler instead)\n";
	errs++;
	if (ex.Get_error_class() == MPI_SUCCESS) {
	    errs++;
	    cout << "Unexpected error from Iprobe" << endl;
	}
    }
    if (ncalls != 1) {
	errs++;
	cout << "Did not invoke error handler when invoking iprobe with an invalid rank" << endl;
    }

    // Find out how many errors we saw

    MTest_Finalize( errs );
    MPI::Finalize();

    return 0;
}
