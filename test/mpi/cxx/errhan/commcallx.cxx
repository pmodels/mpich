/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
#include <stdio.h>
#include "mpitestcxx.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static char MTEST_Descrip[] = "Test comm_call_errhandler";

static int calls = 0;
static int errs = 0;
static MPI::Intracomm mycomm;
void eh( MPI::Comm &comm, int *err, ... )
{
    if (*err != MPI_ERR_OTHER) {
	errs++;
	cout << "Unexpected error code\n";
    }
    if (comm != mycomm) {
	char cname[MPI_MAX_OBJECT_NAME];
	int len;
	errs++;
	cout << "Unexpected communicator\n";
	comm.Get_name( cname, len );
	cout << "Comm is " << cname << "\n";
	mycomm.Get_name( cname, len );
	cout << "mycomm is " << cname << "\n";
    }
    calls++;
    return;
}
int main( int argc, char *argv[] )
{
    MPI::Intracomm  comm;
    MPI::Errhandler newerr;
    int            i;
    int            reset_handler;

    MTest_Init( );

    comm = MPI::COMM_WORLD;
    mycomm = comm;
    mycomm.Set_name( "dup of comm_world" );

    newerr = MPI::Comm::Create_errhandler( eh );

    comm.Set_errhandler( newerr );
    comm.Call_errhandler( MPI_ERR_OTHER );
    newerr.Free();
    if (calls != 1) {
	errs++;
	cout << "Error handler not called\n";
    }

    // Here we apply the test to many copies of a communicator 
    for (reset_handler = 0; reset_handler <= 1; ++reset_handler) {
        for (i=0; i<1000; i++) {
            MPI::Intracomm comm2;
            calls = 0;
	    comm = MPI::COMM_WORLD.Dup();
            mycomm = comm;
	    mycomm.Set_name( "dup of comm_world" );
            newerr = MPI::Comm::Create_errhandler( eh );

	    comm.Set_errhandler( newerr );
	    comm.Call_errhandler( MPI_ERR_OTHER );
            if (calls != 1) {
                errs++;
		cout << "Error handler not called\n";
            }
	    comm2 = comm.Dup();
            calls = 0;
            mycomm = comm2;
	    mycomm.Set_name( "dup of dup of comm_world" );
            // comm2 must inherit the error handler from comm 
	    comm2.Call_errhandler( MPI_ERR_OTHER );
            if (calls != 1) {
                errs++;
		cout << "Error handler not called\n";
            }

            if (reset_handler) {
                // extra checking of the reference count handling 
		comm.Set_errhandler( MPI::ERRORS_THROW_EXCEPTIONS );
            }
	    newerr.Free();
	    comm.Free();
	    comm2.Free();
        }
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
