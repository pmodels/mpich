/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Include stdio.h first to see if the MPI implementation can handle the
   conflicting definitions in stdio.h for the SEEK_SET, SEEK_CUR, and 
   SEEK_END */
#include "mpitestconf.h"

#include <stdio.h>
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

#include "mpi.h"
#include "mpitestcxx.h"

static char MTEST_Descrip[] = "Test availability of MPI::SEEK_SET and SEEK_SET from stdio.h";

int main( int argc, char *argv[] )
{
    int             errs = 0, err;
    int             rank;
    MPI::Intracomm  comm;
    MPI::Status     status;
    int             seekValues;

    MTest_Init( );
    comm = MPI::COMM_WORLD;

    // Make sure that we can access each value
    // First, the MPI C++ values
    seekValues = MPI::SEEK_SET;
    if (MPI::SEEK_CUR == seekValues) {
	errs++;
    }
    if (MPI::SEEK_END == seekValues) {
	errs++;
    }

    // Second, the stdio values
    seekValues = SEEK_SET;
    if (SEEK_CUR == seekValues) {
	errs++;
    }
    if (SEEK_END == seekValues) {
	errs++;
    }

    /* some workarounds for the SEEK_SET problem prevent its
       use as a case label */
    seekValues = SEEK_SET;
    switch (seekValues) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
        case SEEK_END:
        default:
            errs++;
            break;
    }
    
    MTest_Finalize( errs );
    MPI::Finalize( );
    return 0;
}
