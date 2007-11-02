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

static char MTEST_Descrip[] = "Test of Win_get_group";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int result;
    int buf[10];
    MPI::Win   win;
    MPI::Group group, wingroup;
    int minsize = 2;
    MPI::Intracomm      comm;

    MTest_Init();

    /* The following illustrates the use of the routines to 
       run through a selection of communicators and datatypes.
       Use subsets of these for tests that do not involve combinations 
       of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral( comm, minsize, true )) {
	if (comm == MPI::COMM_NULL) continue;

	win = MPI::Win::Create( buf, sizeof(int) * 10, sizeof(int), 
			MPI::INFO_NULL, comm );
	wingroup = win.Get_group();
	group = comm.Get_group();
	result = MPI::Group::Compare( group, wingroup );
	if (result != MPI::IDENT) {
	    errs++;
	    cout << "Group returned by Win_get_group not the same as the input group\n";
	}
	wingroup.Free();
	group.Free();
	win.Free();
	MTestFreeComm( comm );
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
