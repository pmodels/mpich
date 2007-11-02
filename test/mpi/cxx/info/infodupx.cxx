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

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::Info info1, infodup;
    int nkeys, nkeysdup, i, vallen, flag, flagdup;
    char *key, *keydup;
    char *value, *valdup;

    MTest_Init( );

    key = new char [MPI::MAX_INFO_KEY];
    keydup = new char [MPI::MAX_INFO_KEY];
    value = new char [MPI::MAX_INFO_VAL];
    valdup = new char [MPI::MAX_INFO_VAL];

    info1 = MPI::Info::Create( );
    /* Use only named keys incase the info implementation only supports
       the predefined keys (e.g., IBM) */
    info1.Set( "host", "myhost.myorg.org" );
    info1.Set( "file", "runfile.txt" );
    info1.Set( "soft", "2:1000:4,3:1000:7" );

    infodup = info1.Dup();

    nkeysdup = infodup.Get_nkeys();
    nkeys    = info1.Get_nkeys();
    if (nkeys != nkeysdup) {
	errs++;
	cout << "Dup'ed info has a different number of keys; is " <<
	    nkeysdup << " should be " << nkeys << "\n";
    }
    vallen = MPI::MAX_INFO_VAL;
    for (i=0; i<nkeys; i++) {
	/* MPI requires that the keys are in the same order after the dup */
	info1.Get_nthkey( i, key );
	infodup.Get_nthkey( i, keydup );
	if (strcmp(key, keydup)) {
	    errs++;
	    cout << "keys do not match: " << keydup << " should be " <<
		key << "\n";
	}

	vallen = MPI::MAX_INFO_VAL;
	flag = info1.Get( key, vallen, value );
	flagdup = infodup.Get( keydup, vallen, valdup );
	if (!flag || !flagdup) {
	    errs++;
	    cout << "Info get failed for key " << key << "\n";
	}
	else if (strcmp( value, valdup )) {
	    errs++;
	    cout << "Info values for key " << key << 
		" not the same after dup\n";
	}
    }

    /* Change info and check that infodup does NOT have the new value 
       (ensure that lazy dups are still duped) */
    info1.Set( "path", "/a:/b:/c/d" );

    flag = infodup.Get( "path", vallen, value );
    if (flag) {
	errs++;
	cout << "inserting path into info changed infodup\n";
    }
    
    info1.Free();
    infodup.Free();
    
    MTest_Finalize( errs );
    delete [] key;
    delete [] keydup;
    delete [] value;
    delete [] valdup;

    MPI::Finalize();

    return 0;  
}
