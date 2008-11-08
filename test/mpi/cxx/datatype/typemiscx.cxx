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

int main( int argc, char *argv[] )
{
    int num_ints, num_adds, num_types, combiner, errs = 0;

    MPI::Init();

    /* Check for the Fortran Datatypes */
#ifdef HAVE_FORTRAN_BINDING
    /* First, the optional types.  We allow these to be DATATYPE_NULL */
    if (MPI::REAL4 != MPI::DATATYPE_NULL) {
	MPI::REAL4.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "REAL4 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::REAL8 != MPI::DATATYPE_NULL) {
	MPI::REAL8.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "REAL8 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::REAL16 != MPI::DATATYPE_NULL) {
	MPI::REAL16.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "REAL16 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::COMPLEX8 != MPI::DATATYPE_NULL) {
	MPI::COMPLEX8.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "COMPLEX8 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::COMPLEX16 != MPI::DATATYPE_NULL) {
	MPI::COMPLEX16.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "COMPLEX16 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::COMPLEX32 != MPI::DATATYPE_NULL) {
	MPI::COMPLEX32.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "COMPLEX32 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::INTEGER1 != MPI::DATATYPE_NULL) {
	MPI::INTEGER1.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "INTEGER1 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::INTEGER2 != MPI::DATATYPE_NULL) {
	MPI::INTEGER2.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "INTEGER2 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::INTEGER4 != MPI::DATATYPE_NULL) {
	MPI::INTEGER4.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "INTEGER4 not a NAMED type" << "\n";
	    errs++;
	}
    }
    if (MPI::INTEGER8 != MPI::DATATYPE_NULL) {
	MPI::INTEGER8.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "INTEGER8 not a NAMED type" << "\n";
	    errs++;
	}
    }
#ifdef HAVE_MPI_INTEGER16
    if (MPI::INTEGER16 != MPI::DATATYPE_NULL) {
	MPI::INTEGER16.Get_envelope( num_ints, num_adds, num_types, combiner );
	if (combiner != MPI::COMBINER_NAMED) {
	    cout << "INTEGER16 not a NAMED type" << "\n";
	    errs++;
	}
    }
#endif
    /* Here end the optional types */

    MPI::INTEGER.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "INTEGER not a NAMED type" << "\n";
	errs++;
    }
    MPI::REAL.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "REAL not a NAMED type" << "\n";
	errs++;
    }
    MPI::DOUBLE_PRECISION.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "DOUBLE_PRECISION not a NAMED type" << "\n";
	errs++;
    }
    MPI::F_COMPLEX.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "F_COMPLEX not a NAMED type" << "\n";
	errs++;
    }
    MPI::F_DOUBLE_COMPLEX.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "F_DOUBLE_COMPLEX not a NAMED type" << "\n";
	errs++;
    }
    MPI::LOGICAL.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "LOGICAL not a NAMED type" << "\n";
	errs++;
    }
    MPI::CHARACTER.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "CHARACTER not a NAMED type" << "\n";
	errs++;
    }
    MPI::TWOREAL.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "TWOREAL not a NAMED type" << "\n";
	errs++;
    }
    MPI::TWODOUBLE_PRECISION.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "TWODOUBLE_PRECISION not a NAMED type" << "\n";
	errs++;
    }
    MPI::TWOINTEGER.Get_envelope( num_ints, num_adds, num_types, combiner );
    if (combiner != MPI::COMBINER_NAMED) {
	cout << "TWOINTEGER not a NAMED type" << "\n";
	errs++;
    }
#endif

    if (MPI::COMM_WORLD.Get_rank() == 0) {
	if (errs) {
	    cout << "Found " << errs << " errors\n";
	}
	else {
	    cout << " No Errors\n";
	}
    }
    MPI::Finalize();

    return 0;
}
