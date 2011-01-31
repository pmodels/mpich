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

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::Datatype  type;
    MPI::Intracomm comm;
    MTestDatatype mstype, mrtype;
    char dtypename[MPI_MAX_OBJECT_NAME];
    int size1, size2, tnlen;

    MTest_Init();

    comm = MPI::COMM_WORLD;
    while (MTestGetDatatypes( &mstype, &mrtype, 1 )) {
	type = mstype.datatype;

	// Testing the pack size is tricky, since this is the 
	// size that is stored when packed with type.Pack, and
	// is not easily defined.  We look for consistency
	size1 = type.Pack_size( 1, comm );
	size2 = type.Pack_size( 2, comm );
	if (size1 <= 0 || size2 <= 0) {
	    errs++;
	    type.Get_name( dtypename, tnlen );
	    cout << "Pack size of datatype " << dtypename << 
		" is not positive\n";
	}
	if (size1 >= size2) {
	    errs++;
	    type.Get_name( dtypename, tnlen );
	    cout << "Pack size of 2 of " << dtypename << 
		" is smaller or the same as the pack size of 1 instance\n";
	}

	if (mrtype.datatype != mstype.datatype) {
	    type = mrtype.datatype;

	    // Testing the pack size is tricky, since this is the 
	    // size that is stored when packed with type.Pack, and
	    // is not easily defined.  We look for consistency
	    size1 = type.Pack_size( 1, comm );
	    size2 = type.Pack_size( 2, comm );
	    if (size1 <= 0 || size2 <= 0) {
		errs++;
		type.Get_name( dtypename, tnlen );
		cout << "Pack size of datatype " << dtypename << 
		    " is not positive\n";
	    }
	    if (size1 >= size2) {
		errs++;
		type.Get_name( dtypename, tnlen );
		cout << "Pack size of 2 of " << dtypename << 
		    " is smaller or the same as the pack size of 1 instance\n";
	    }
	}

        MTestFreeDatatype(&mrtype);
	MTestFreeDatatype(&mstype);
    }

    MTest_Finalize( errs );
    
    MPI::Finalize();

    return 0;
}

