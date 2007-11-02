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

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitestcxx.h"

static int codesSeen[3], callcount;
void myerrhanfunc( MPI::File &fh, int *errcode, ... );

int main( int argc, char **argv )
{
    int errs = 0;
    MPI::File fh;
    MPI::Intracomm comm;
    MPI::Errhandler myerrhan, qerr;
    char filename[50];
    char *errstring;
    int code[2], newerrclass, eclass, rlen;

    MTest_Init( );

    errstring = new char [MPI::MAX_ERROR_STRING];

    callcount = 0;

    // Setup some new codes and classes
    newerrclass = MPI::Add_error_class();
    code[0] = MPI::Add_error_code( newerrclass );
    code[1] = MPI::Add_error_code( newerrclass );
    MPI::Add_error_string( newerrclass, "New Class" );
    MPI::Add_error_string( code[0], "First new code" );
    MPI::Add_error_string( code[1], "Second new code" );

    myerrhan = MPI::File::Create_errhandler( myerrhanfunc );

    // Create a new communicator so that we can leave the default errors-abort
    // on COMM_WORLD.  Use this comm for file_open, just to leave a little
    // more separation from comm_world

    comm = MPI::COMM_WORLD.Dup();
    fh = MPI::File::Open( comm, "testfile.txt", 
			 MPI::MODE_RDWR | MPI::MODE_CREATE, 
			 MPI::INFO_NULL );

    fh.Set_errhandler( myerrhan );
    qerr = fh.Get_errhandler();
    if (qerr != myerrhan) {
	errs++;
	cout << " Did not get expected error handler\n";
    }
    qerr.Free();

    // We can free our error handler now
    myerrhan.Free();

    fh.Call_errhandler( newerrclass );
    fh.Call_errhandler( code[0] );
    fh.Call_errhandler( code[1] );

    if (callcount != 3) {
	errs++;
	cout << " Expected 3 calls to error handler, found " << callcount <<
	    "\n";
    }
    else {
	if (codesSeen[0] != newerrclass) {
	    errs++;
	    cout << "Expected class " << newerrclass << " got " << 
		codesSeen[0] << "\n";
	}
	if (codesSeen[1] != code[0]) {
	    errs++;
	    cout << "(1)Expected code " << code[0] << " got " <<
		codesSeen[1] << "\n";
	}
	if (codesSeen[2] != code[1]) {
	    errs++;
	    cout << "(2)Expected code " << code[1] << " got " << 
		codesSeen[2] << "\n";
	}
    }
    
    fh.Close();
    comm.Free();
    MPI::File::Delete( "testfile.txt", MPI::INFO_NULL );

    // Check error strings while we're here...
    MPI::Get_error_string( newerrclass, errstring, rlen );
    if (strcmp(errstring,"New Class") != 0) {
	errs++;
	cout << " Wrong string for error class: " << errstring << "\n";
    }
    eclass = MPI::Get_error_class( code[0] );
    if (eclass != newerrclass) {
	errs++;
	cout << " Class for new code is not correct\n";
    }
    MPI::Get_error_string( code[0], errstring, rlen );
    if (strcmp( errstring, "First new code") != 0) {
	errs++;
	cout << " Wrong string for error code: " << errstring << "\n";
    }
    eclass = MPI::Get_error_class( code[1] );
    if (eclass != newerrclass) {
	errs++;
	cout << " Class for new code is not correct\n";
    }

    MPI::Get_error_string( code[1], errstring, rlen );
    if (strcmp( errstring, "Second new code") != 0) {
	errs++;
	cout << " Wrong string for error code: " << errstring << "\n";
    }
    
    delete [] errstring;

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}

void myerrhanfunc( MPI::File &fh, int *errcode, ... )
{
    char *errstring;
    int  rlen;

    errstring =  new char [MPI::MAX_ERROR_STRING];

    callcount++;

    // Remember the code we've seen
    if (callcount <  4) {
	codesSeen[callcount-1] = *errcode;
    }
    MPI::Get_error_string( *errcode, errstring, rlen );
    delete [] errstring;
}
