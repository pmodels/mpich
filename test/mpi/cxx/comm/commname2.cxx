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
#include <string.h>
#include <stdio.h>
#include "mpitestcxx.h"

int main( int argc, char **argv )
{
    int errs = 0;
    char name[MPI_MAX_OBJECT_NAME], tname[MPI_MAX_OBJECT_NAME];
    int  rlen;
    int  i, n;
    MPI::Comm *(comm[10]);
    
    MTest_Init( );

    // Test predefined names
    MPI::COMM_WORLD.Get_name( name, rlen );
    if (strcmp( name, "MPI_COMM_WORLD" ) != 0) {
	errs ++;
	cout << "Name of COMM_WORLD was " << name << "\n";
    }
    if (rlen != strlen(name)) {
	errs++;
	cout << "Resultlen for COMM_WORLD is incorrect: " << rlen << "\n";
    }

    MPI::COMM_SELF.Get_name( name, rlen );
    if (strcmp( name, "MPI_COMM_SELF" ) != 0) {
	errs ++;
	cout << "Name of COMM_SELF was " << name << "\n";
    }
    if (rlen != strlen(name)) {
	errs++;
	cout << "Resultlen for COMM_SELF is incorrect: " << rlen << "\n";
    }

    // Reset name of comm_world
    MPI::COMM_WORLD.Set_name( "FooBar !" );
    MPI::COMM_WORLD.Get_name( name, rlen );
    if (strcmp( name, "FooBar !" ) != 0) {
	errs ++;
	cout << "Changed name of COMM_WORLD was " << name << "\n";
    }

    // Test a few other communicators
    n = 0;
    while (MTestGetComm( &comm[n], 1 ) && n < 4) {
	sprintf( name, "test%d", n );
	comm[n]->Set_name( name );
	n++;
    }
    for (i=0; i<n; i++) {
	sprintf( tname, "test%d", i );
	comm[i]->Get_name( name, rlen );
	if (strcmp( name, tname ) != 0) {
	    errs++;
	    cout << "comm[" << i << "] gave name " << name << "\n";
	}
        MTestFreeComm ( *comm[i] );
    }
    
    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
