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

int main( int argc, char *argv[] )
{
    int errs = 0;
    bool flag;
    int provided, claimed;

    provided = MPI::Init_thread( MPI::THREAD_MULTIPLE );
    
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
