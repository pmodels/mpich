/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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

/* An unsupported optional type may be a valid handle that MPI calls reject,
 * so check it with an error-returning call (needs MPI_ERRORS_RETURN). */
static bool type_is_available(MPI_Datatype dtype)
{
    int size = 0;
    if (dtype == MPI_DATATYPE_NULL) {
        return false;
    }
    int err = MPI_Type_size(dtype, &size);
    return err == MPI_SUCCESS && size > 0;
}

int main(int argc, char *argv[])
{
    int num_ints, num_adds, num_types, combiner, errs = 0;

    MPI::Init();

    /* Errors not tied to an object go to the handler on MPI_COMM_SELF */
    MPI::COMM_SELF.Set_errhandler(MPI::ERRORS_RETURN);

    /* Check for the Fortran Datatypes */
#ifdef HAVE_FORTRAN_BINDING
    /* First, the optional types.  We skip the ones that are not available */
    if (type_is_available(MPI::REAL4)) {
        MPI::REAL4.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "REAL4 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::REAL8)) {
        MPI::REAL8.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "REAL8 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::REAL16)) {
        MPI::REAL16.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "REAL16 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::COMPLEX8)) {
        MPI::COMPLEX8.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "COMPLEX8 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::COMPLEX16)) {
        MPI::COMPLEX16.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "COMPLEX16 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::COMPLEX32)) {
        MPI::COMPLEX32.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "COMPLEX32 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::INTEGER1)) {
        MPI::INTEGER1.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "INTEGER1 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::INTEGER2)) {
        MPI::INTEGER2.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "INTEGER2 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::INTEGER4)) {
        MPI::INTEGER4.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "INTEGER4 not a NAMED type" << "\n";
            errs++;
        }
    }
    if (type_is_available(MPI::INTEGER8)) {
        MPI::INTEGER8.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "INTEGER8 not a NAMED type" << "\n";
            errs++;
        }
    }
#ifdef HAVE_MPI_INTEGER16
    if (type_is_available(MPI::INTEGER16)) {
        MPI::INTEGER16.Get_envelope(num_ints, num_adds, num_types, combiner);
        if (combiner != MPI::COMBINER_NAMED) {
            cout << "INTEGER16 not a NAMED type" << "\n";
            errs++;
        }
    }
#endif
    /* Here end the optional types */

    MPI::INTEGER.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "INTEGER not a NAMED type" << "\n";
        errs++;
    }
    MPI::REAL.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "REAL not a NAMED type" << "\n";
        errs++;
    }
    MPI::DOUBLE_PRECISION.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "DOUBLE_PRECISION not a NAMED type" << "\n";
        errs++;
    }
    MPI::F_COMPLEX.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "F_COMPLEX not a NAMED type" << "\n";
        errs++;
    }
    MPI::F_DOUBLE_COMPLEX.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "F_DOUBLE_COMPLEX not a NAMED type" << "\n";
        errs++;
    }
    MPI::LOGICAL.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "LOGICAL not a NAMED type" << "\n";
        errs++;
    }
    MPI::CHARACTER.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "CHARACTER not a NAMED type" << "\n";
        errs++;
    }
    MPI::TWOREAL.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "TWOREAL not a NAMED type" << "\n";
        errs++;
    }
    MPI::TWODOUBLE_PRECISION.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "TWODOUBLE_PRECISION not a NAMED type" << "\n";
        errs++;
    }
    MPI::TWOINTEGER.Get_envelope(num_ints, num_adds, num_types, combiner);
    if (combiner != MPI::COMBINER_NAMED) {
        cout << "TWOINTEGER not a NAMED type" << "\n";
        errs++;
    }
#endif

    MTest_Finalize(errs);

    return 0;
}
