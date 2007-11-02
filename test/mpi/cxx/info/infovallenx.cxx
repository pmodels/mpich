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

#define NKEYS 3
int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::Info info;
    const char *keys[NKEYS] = { "file", "soft", "host" };
    const char *values[NKEYS] = { "runfile.txt", "2:1000:4,3:1000:7", 
				  "myhost.myorg.org" };
    char *value;
    int i, flag, vallen;

    MTest_Init( );

    value = new char [MPI::MAX_INFO_VAL];

    info = MPI::Info::Create();
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    for (i=0; i<NKEYS; i++) {
	info.Set( keys[i], values[i] );
    }

    /* Check that all values are present */
    for (i=0; i<NKEYS; i++) {
	flag = info.Get_valuelen( keys[i], vallen );
	if (!flag) {
	    errs++;
	    cout << "get_valuelen failed for valid key " << 
		keys[i] << "\n";
	}
	flag = info.Get( keys[i], MPI::MAX_INFO_VAL, value );
	if (!flag) {
	    errs++;
	    cout << "No value for key " << keys[i] << "\n";
	}
	if (strcmp( value, values[i] )) {
	    errs++;
	    cout << "Incorrect value for key " << keys[i] << "\n";
	}
	if (strlen(value) != vallen) {
	    errs++;
	    cout << "value_len returned " << vallen << 
		" but actual len is " << strlen(value) << "\n";
	}
    }

    info.Free();
    delete [] value;
    
    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
