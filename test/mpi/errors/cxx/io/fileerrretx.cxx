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
void efn( MPI::File &fh, int *code, ... )
{
    ncalls ++;
}

int main( int argc, char *argv[] )
{
    MPI::File fh;
    MPI::Errhandler eh;
    char *filename;
    int errs = 0, toterrs, rank;
    int sawErr;

    MPI::Init();
    // Test that the default error handler is errors return for files

    filename = new char[10];
    strncpy( filename, "t1", 10 );

    MPI::FILE_NULL.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
    sawErr = 0;
    try {
	fh = MPI::File::Open(MPI::COMM_WORLD, filename, 
			 MPI::MODE_RDWR, MPI::INFO_NULL );
    } catch (MPI::Exception ex) {
	if (verbose) cout << "Caught exception from open\n";
	if (ex.Get_error_class() == MPI_SUCCESS) {
	    errs++;
	    cout << "Unexpected error from Open" << endl;
	}
	sawErr = 1;
    }
    if (!sawErr) {
	errs++;
	cout << "Did not see error when opening a non-existent file for writing and reading (without MODE_CREATE)\n";
    }

    // Test that we can change the default error handler by changing
    // the error handler on MPI::FILE_NULL.
    eh = MPI::File::Create_errhandler( efn );
    MPI::FILE_NULL.Set_errhandler( eh );
    eh.Free();
    sawErr = 0;
    try {
	fh = MPI::File::Open(MPI::COMM_WORLD, filename, 
			 MPI::MODE_RDWR, MPI::INFO_NULL );
    } catch (MPI::Exception ex) {
	cout << "Caught exception from open (should have called error handler instead)\n";
	errs++;
	if (ex.Get_error_class() == MPI_SUCCESS) {
	    errs++;
	    cout << "Unexpected error from Open" << endl;
	}
	sawErr = 1;
    }
    if (ncalls != 1) {
	errs++;
	cout << "Did not invoke error handler when opening a non-existent file for writing and reading (without MODE_CREATE)" << endl;
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
