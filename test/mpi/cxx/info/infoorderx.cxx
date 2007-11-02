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
#include "mpitestcxx.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define NKEYS 3
int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::Info info;
    const char *keys1[NKEYS] = { "file", "soft", "host" };
    const char *values1[NKEYS] = { "runfile.txt", "2:1000:4,3:1000:7", 
				   "myhost.myorg.org" };

    char *value;
    int i, flag;

    MTest_Init();

    value = new char [MPI::MAX_INFO_VAL];

    /* 1,2,3 */
    info = MPI::Info::Create();
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    for (i=0; i<NKEYS; i++) {
	info.Set( keys1[i], values1[i] );
    }

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	flag = info.Get( keys1[i], MPI::MAX_INFO_VAL, value  );
	if (!flag) {
	    errs++;
	    cout << "No value for key " << keys1[i] << "\n";
	}
	if (strcmp( value, values1[i] )) {
	    errs++;
	    cout << "Incorrect value for key " << keys1[i] << "\n";
	}
    }
    info.Free();

    /* 3,2,1 */
    info = MPI::Info::Create();
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    for (i=NKEYS-1; i>=0; i--) {
	info.Set( keys1[i], values1[i] );
    }

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	flag = info.Get( keys1[i], MPI::MAX_INFO_VAL, value );
	if (!flag) {
	    errs++;
	    cout << "No value for key " << keys1[i] << "\n";
	}
	if (strcmp( value, values1[i] )) {
	    errs++;
	    cout << "Incorrect value for key " << keys1[i] << "\n";
	}
    }
    info.Free();

    /* 1,3,2 */
    info = MPI::Info::Create();
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    info.Set( keys1[0], values1[0] );
    info.Set( keys1[2], values1[2] );
    info.Set( keys1[1], values1[1] );

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	flag = info.Get( keys1[i], MPI::MAX_INFO_VAL, value );
	if (!flag) {
	    errs++;
	    cout << "No value for key " << keys1[i] << "\n";
	}
	if (strcmp( value, values1[i] )) {
	    errs++;
	    cout << "Incorrect value for key " << keys1[i] << "\n";
	}
    }
    info.Free( );

    /* 2,1,3 */
    info = MPI::Info::Create();
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    info.Set( keys1[1], values1[1] );
    info.Set( keys1[0], values1[0] );
    info.Set( keys1[2], values1[2] );

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	flag = info.Get( keys1[i], MPI::MAX_INFO_VAL, value );
	if (!flag) {
	    errs++;
	    cout << "No value for key " << keys1[i] << "\n";
	}
	if (strcmp( value, values1[i] )) {
	    errs++;
	    cout << "Incorrect value for key " << keys1[i] << "\n";
	}
    }
    info.Free( );

    /* 2,3,1 */
    info = MPI::Info::Create();
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    info.Set( keys1[1], values1[1] );
    info.Set( keys1[2], values1[2] );
    info.Set( keys1[0], values1[0] );

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	flag = info.Get( keys1[i], MPI::MAX_INFO_VAL, value );
	if (!flag) {
	    errs++;
	    cout << "No value for key " << keys1[i] << "\n";
	}
	if (strcmp( value, values1[i] )) {
	    errs++;
	    cout << "Incorrect value for key " << keys1[i] << "\n";
	}
    }
    info.Free();
    
    /* 3,1,2 */
    info = MPI::Info::Create();
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    info.Set( keys1[2], values1[2] );
    info.Set( keys1[0], values1[0] );
    info.Set( keys1[1], values1[1] );

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	flag = info.Get( keys1[i], MPI::MAX_INFO_VAL, value );
	if (!flag) {
	    errs++;
	    cout << "No value for key " << keys1[i] << "\n";
	}
	if (strcmp( value, values1[i] )) {
	    errs++;
	    cout << "Incorrect value for key " << keys1[i] << "\n";
	}
    }
    info.Free();
    delete [] value;
    
    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;  
}
