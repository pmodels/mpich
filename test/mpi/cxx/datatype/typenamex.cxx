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

/* Create an array with all of the MPI names in it */

typedef struct mpi_names_t { 
    MPI::Datatype dtype; const char *name; } mpi_names_t;


static mpi_names_t *mpi_names = 0;
void InitMPINames (void);

int main( int argc, char **argv )
{
    char *name;
    int namelen, i;
    int errs = 0;

    MPI::Init();

    namelen = MPI::MAX_OBJECT_NAME;
    name = new char[MPI::MAX_OBJECT_NAME];

    InitMPINames();

    /* Sample some datatypes */
    /* See 8.4, "Naming Objects" in MPI-2.  The default name is the same
       as the datatype name */

    MPI::DOUBLE.Get_name( name, namelen );
    if (strncmp( name, "MPI_DOUBLE", MPI::MAX_OBJECT_NAME )) {
	errs++;
	cout << "Expected MPI_DOUBLE but got :" << name << ":\n";
    }

    MPI::INT.Get_name( name, namelen );
    if (strncmp( name, "MPI_INT", MPI::MAX_OBJECT_NAME )) {
	errs++;
	cout << "Expected MPI_INT but got :" << name << ":\n";
    }

    /* Now we try them ALL */
    for (i=0; mpi_names[i].name != 0; i++) {
	/* The size-specific types, as well as the language optional
	   long long and long double, may be DATATYPE_NULL */
	if (mpi_names[i].dtype == MPI::DATATYPE_NULL) continue;
	name[0] = 0;
	mpi_names[i].dtype.Get_name( name, namelen );
	if (strncmp( name, mpi_names[i].name, namelen )) {
	    errs++;
	    cout << "Expected " << mpi_names[i].name << " but got " <<
		name << "\n";
	}
    }

    /* Try resetting the name */
    MPI::INT.Set_name( "int" );
    name[0] = 0;
    MPI::INT.Get_name( name, namelen );
    if (strncmp( name, "int", MPI::MAX_OBJECT_NAME )) {
	errs++;
	cout << "Expected int but got :" << name << ":\n";
    }


    if (errs) {
	cout << "Found " << errs << " errors\n";
    }
    else {
	cout << " No Errors\n";
    }
    delete [] name;
    delete [] mpi_names;

    MPI::Finalize();
    return 0;
}

// Initialize the mpi_names array here.  This make sure that we don't
// initialize the values before Init or Init_thread are called.

/* The MPI standard specifies that the names must be the MPI names,
   not the related language names (e.g., MPI_CHAR, not char).
*/
void InitMPINames (void) {
    int i;
    mpi_names_t lmpi_names[] = {
	{ MPI::CHAR, "MPI_CHAR" },
	{ MPI::SIGNED_CHAR, "MPI_SIGNED_CHAR" },
	{ MPI::UNSIGNED_CHAR, "MPI_UNSIGNED_CHAR" },
	{ MPI::BYTE, "MPI_BYTE" },
	{ MPI::WCHAR, "MPI_WCHAR" },
	{ MPI::SHORT, "MPI_SHORT" },
	{ MPI::UNSIGNED_SHORT, "MPI_UNSIGNED_SHORT" },
	{ MPI::INT, "MPI_INT" },
	{ MPI::UNSIGNED, "MPI_UNSIGNED" },
	{ MPI::LONG, "MPI_LONG" },
	{ MPI::UNSIGNED_LONG, "MPI_UNSIGNED_LONG" },
	{ MPI::FLOAT, "MPI_FLOAT" },
	{ MPI::DOUBLE, "MPI_DOUBLE" },
	{ MPI::LONG_DOUBLE, "MPI_LONG_DOUBLE" },
	/*    { MPI::LONG_LONG_INT, "MPI_LONG_LONG_INT" }, */
	{ MPI::LONG_LONG, "MPI_LONG_LONG" },
	{ MPI::UNSIGNED_LONG_LONG, "MPI_UNSIGNED_LONG_LONG" }, 
	{ MPI::PACKED, "MPI_PACKED" },
	{ MPI::LB, "MPI_LB" },
	{ MPI::UB, "MPI_UB" },
	{ MPI::FLOAT_INT, "MPI_FLOAT_INT" },
	{ MPI::DOUBLE_INT, "MPI_DOUBLE_INT" },
	{ MPI::LONG_INT, "MPI_LONG_INT" },
	{ MPI::SHORT_INT, "MPI_SHORT_INT" },
	{ MPI::TWOINT, "MPI_2INT" },
	{ MPI::LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT" },
	/* Fortran */
#ifdef HAVE_FORTRAN_BINDING
	{ MPI::F_COMPLEX, "MPI_COMPLEX" },
	{ MPI::F_DOUBLE_COMPLEX, "MPI_DOUBLE_COMPLEX" },
	{ MPI::LOGICAL, "MPI_LOGICAL" },
	{ MPI::REAL, "MPI_REAL" },
	{ MPI::DOUBLE_PRECISION, "MPI_DOUBLE_PRECISION" },
	{ MPI::INTEGER, "MPI_INTEGER" },
	{ MPI::TWOINTEGER, "MPI_2INTEGER" },
	{ MPI::TWOREAL, "MPI_2REAL" },
	{ MPI::TWODOUBLE_PRECISION, "MPI_2DOUBLE_PRECISION" },
	{ MPI::CHARACTER, "MPI_CHARACTER" },
    /* Size-specific (Fortran) types */
	{ MPI::REAL4, "MPI_REAL4" },
	{ MPI::REAL8, "MPI_REAL8" },
	{ MPI::REAL16, "MPI_REAL16" },
	{ MPI::COMPLEX8, "MPI_COMPLEX8" },
	{ MPI::COMPLEX16, "MPI_COMPLEX16" },
	{ MPI::COMPLEX32, "MPI_COMPLEX32" },
	{ MPI::INTEGER1, "MPI_INTEGER1" },
	{ MPI::INTEGER2, "MPI_INTEGER2" },
	{ MPI::INTEGER4, "MPI_INTEGER4" },
	{ MPI::INTEGER8, "MPI_INTEGER8" },
	{ MPI::INTEGER16, "MPI_INTEGER16" },
#endif
	/* C++ only types */
	{ MPI::BOOL, "MPI::BOOL" },
	{ MPI::COMPLEX, "MPI::COMPLEX" },
	{ MPI::DOUBLE_COMPLEX, "MPI::DOUBLE_COMPLEX" },
	{ MPI::LONG_DOUBLE_COMPLEX, "MPI::LONG_DOUBLE_COMPLEX" },
	{ 0, (char *)0 },  /* Sentinal used to indicate the last element */
    };

    mpi_names = new mpi_names_t [sizeof(lmpi_names)/sizeof(mpi_names_t)];
    i = 0;
    while (lmpi_names[i].name) {
	mpi_names[i] = lmpi_names[i];
	i++;
    }
    mpi_names[i].name = 0;
    mpi_names[i].dtype = 0;
}
