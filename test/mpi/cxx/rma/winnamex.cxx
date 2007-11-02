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
#include <stdio.h>
#include "mpitestcxx.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::Win win;
    int cnt, namelen;
    char *name, *nameout;

    MTest_Init();

    name = new char [MPI::MAX_OBJECT_NAME];
    nameout = new char [MPI::MAX_OBJECT_NAME];

    cnt = 0;
    while (MTestGetWin( win, true )) {
	if (win == MPI::WIN_NULL) continue;
    
	sprintf( name, "win-%d", cnt );
	cnt++;
	win.Set_name( name );
	nameout[0] = 0;
	win.Get_name( nameout, namelen );
	if (strcmp( name, nameout )) {
	    errs++;
	    cout << "Unexpected name, was " << nameout << 
		" but should be " << name << "\n";
	}

	MTestFreeWin(win);
    }
    if (cnt == 0) {
        errs++;
        cout << "No windows created\n";
    }

    delete [] name;
    delete [] nameout;

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
