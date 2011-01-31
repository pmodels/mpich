/* -*- Mode: C++; c-basic-offset:4 ; -*- */
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

#include "mpitestcxx.h"
/* Needed for strcmp */
#include <string.h>

int main( int argc, char *argv[] )
{
    int errs = 0;
    bool flag;
    int provided, claimed;

    // This test must be invoked with two arguments: myarg1 myarg2
    provided = MPI::Init_thread( argc, argv, MPI::THREAD_MULTIPLE );
    
    if (argc != 3) {
	errs++;
	cout << "Expected argc=3 but saw argc=" << argc << "\n";
    }
    else {
	if (strcmp( argv[1], "myarg1" ) != 0) {
	    errs++;
	    cout << "Expected myarg1 for 1st argument but saw " << argv[1] 
		 << "\n";
	}
	if (strcmp( argv[2], "myarg2" ) != 0) {
	    errs++;
	    cout << "Expected myarg2 for 1st argument but saw " << argv[2] 
		 << "\n";
	}
    }

    // Confirm that MPI is properly initialized
    flag = MPI::Is_thread_main();
    if (!flag) {
	errs++;
	cout << "This thread call init_thread but Is_thread_main gave false\n";
    }
    claimed = MPI::Query_thread();
    if (claimed != provided) {
	errs++;
	cout << "Query thread gave thread level " << claimed << 
	    " but Init_thread gave " << provided << "\n";
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
