/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2010 by Argonne National Laboratory.
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

static char MTEST_Descrip[] = "Test win_call_errhandler";

static int calls = 0;
static int errs = 0;
static MPI::Win mywin;
void eh( MPI::Win &win, int *err, ... )
{
    if (*err != MPI_ERR_OTHER) {
	errs++;
	cout << "Unexpected error code\n";
    }
    if (win != mywin) {
	char cname[MPI_MAX_OBJECT_NAME];
	int len;
	errs++;
	cout << "Unexpected window\n";
	win.Get_name( cname, len );
	cout << "Win is " << cname << "\n";
	mywin.Get_name( cname, len );
	cout << "Mywin is " << cname << "\n";
    }
    calls++;
    return;
}
int main( int argc, char *argv[] )
{
    MPI::Win        win;
    MPI::Errhandler newerr;
    int            i;
    int            reset_handler;
    int            buf[10];

    MTest_Init( );

    win  = MPI::Win::Create( buf, sizeof(buf), 1, MPI::INFO_NULL, MPI::COMM_WORLD );
    mywin = win;
    mywin.Set_name( "dup of win_world" );

    newerr = MPI::Win::Create_errhandler( eh );

    win.Set_errhandler( newerr );
    win.Call_errhandler( MPI_ERR_OTHER );
    newerr.Free();
    if (calls != 1) {
	errs++;
	cout << "Error handler not called\n";
    }
    win.Free();

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
