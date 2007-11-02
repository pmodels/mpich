/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
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

int main( int argc, char **argv)
{
    int    errs = 0;
    void *v;
    bool flag;
    int  vval;
    int  rank, size;

    MTest_Init();
    size = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();

    flag = MPI::COMM_WORLD.Get_attr( MPI::TAG_UB, &v );
    if (!flag) {
	errs++;
	cout << "Could not get TAG_UB\n";
    }
    else {
	vval = *(int*)v;
	if (vval < 32767) {
	    errs++;
	    cout << "Got too-small value (" << vval << 
		") for TAG_UB\n";
	}
    }

    flag = MPI::COMM_WORLD.Get_attr( MPI::HOST, &v );
    if (!flag) {
	errs++;
	cout << "Could not get HOST\n";
    }
    else {
	vval = *(int*)v;
	if ((vval < 0 || vval >= size) && vval != MPI::PROC_NULL) {
	    errs++;
	    cout << "Got invalid value " << vval << " for HOST\n";
	}
    }
    flag = MPI::COMM_WORLD.Get_attr( MPI::IO, &v );
    if (!flag) {
	errs++;
	cout << "Could not get IO\n";
    }
    else {
	vval = *(int*)v;
	if ((vval < 0 || vval >= size) && vval != MPI::ANY_SOURCE &&
		  vval != MPI::PROC_NULL) {
	    errs++;
	    cout << "Got invalid value " << vval << " for IO\n";
	}
    }

    flag = MPI::COMM_WORLD.Get_attr( MPI::WTIME_IS_GLOBAL, &v );
    if (flag) {
	/* Wtime need not be set */
	vval = *(int*)v;
	if (vval < 0 || vval > 1) {
	    errs++;
	    cout << "Invalid value for WTIME_IS_GLOBAL (got " << vval << ")\n";
	}
    }

    flag = MPI::COMM_WORLD.Get_attr( MPI::APPNUM, &v );
    /* appnum need not be set */
    if (flag) {
	vval = *(int *)v;
	if (vval < 0) {
	    errs++;
	    cout << "MPI_APPNUM is defined as " << vval << 
		" but must be nonnegative\n";
	}
    }

    flag = MPI::COMM_WORLD.Get_attr( MPI::UNIVERSE_SIZE, &v );
    /* MPI_UNIVERSE_SIZE need not be set */
    if (flag) {
	/* But if it is set, it must be at least the size of comm_world */
	vval = *(int *)v;
	if (vval < size) {
	    errs++;
	    cout << "MPI_UNIVERSE_SIZE = " << vval << 
		", less than comm world (" << size << ")\n";
	}
    }
    
    flag = MPI::COMM_WORLD.Get_attr( MPI::LASTUSEDCODE, &v );
    /* Last used code must be defined and >= MPI_ERR_LASTCODE */
    if (flag) {
	vval = *(int*)v;
	if (vval < MPI_ERR_LASTCODE) {
	    errs++;
	    cout << "MPI_LASTUSEDCODE points to an integer (" << vval << 
		") smaller than MPI_ERR_LASTCODE (" << 
		MPI_ERR_LASTCODE << ")\n";
	}
    }
    else {
	errs++;
	cout << "MPI_LASTUSECODE is not defined\n";
    }

    MTest_Finalize( errs );
    MPI::Finalize( );
    
    return 0;
}
