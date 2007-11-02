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

void Explore( MPI::Datatype, int, int & );

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::Datatype ntype1, ntype2;

    MTest_Init();

    Explore( MPI::INT, MPI::COMBINER_NAMED, errs );
    Explore( MPI::BYTE, MPI::COMBINER_NAMED, errs );
    
    ntype1 = MPI::DOUBLE.Create_vector( 10, 1, 30 );
    ntype2 = ntype1.Dup();

    Explore( ntype1, MPI::COMBINER_VECTOR, errs );
    Explore( ntype2, MPI::COMBINER_DUP, errs );

    ntype2.Free();
    ntype1.Free();

    MTest_Finalize( errs );
    
    MPI::Finalize();

    return 0;
}

#define MAX_NINTS 10
#define MAX_DTYPES 10
#define MAX_ASIZEV 10
void Explore( MPI::Datatype dtype, int mycomb, int &errs )
{
    int nints, nadds, ntype, combiner;
    int intv[MAX_NINTS];
    MPI::Aint aintv[MAX_ASIZEV];
    MPI::Datatype dtypesv[MAX_DTYPES];

    dtype.Get_envelope( nints, nadds, ntype, combiner );
    
    if (combiner != MPI::COMBINER_NAMED) {
	dtype.Get_contents( MAX_NINTS, MAX_ASIZEV, MAX_DTYPES,
			    intv, aintv, dtypesv );

	if (combiner != mycomb) {
	    errs++;
	    cout << "Expected combiner " << mycomb << " but got " <<
		combiner << "\n";
	}

	// List all combiner types to check that they are defined in mpif.h
	if (combiner == MPI::COMBINER_NAMED) { ; }
	else if (combiner == MPI::COMBINER_DUP) { 
            dtypesv[0].Free(); 
        }
	else if (combiner == MPI::COMBINER_CONTIGUOUS) { ; }
	else if (combiner == MPI::COMBINER_VECTOR) { ; }
	else if (combiner == MPI::COMBINER_HVECTOR_INTEGER) { ; }
	else if (combiner == MPI::COMBINER_HVECTOR) { ; }
	else if (combiner == MPI::COMBINER_INDEXED) { ; }
	else if (combiner == MPI::COMBINER_HINDEXED_INTEGER) { ; }
	else if (combiner == MPI::COMBINER_HINDEXED) { ; }
	else if (combiner == MPI::COMBINER_INDEXED_BLOCK) { ; }
	else if (combiner == MPI::COMBINER_STRUCT_INTEGER) { ; }
	else if (combiner == MPI::COMBINER_STRUCT) { ; }
	else if (combiner == MPI::COMBINER_SUBARRAY) { ; }
	else if (combiner == MPI::COMBINER_DARRAY) { ; }
	else if (combiner == MPI::COMBINER_F90_REAL) { ; }
	else if (combiner == MPI::COMBINER_F90_COMPLEX) { ; }
	else if (combiner == MPI::COMBINER_F90_INTEGER) { ; }
	else if (combiner == MPI::COMBINER_RESIZED) { ; }
	else {
	   errs++;
	   cout << "Unknown combiner " << combiner << "\n";
	}
    }
}
