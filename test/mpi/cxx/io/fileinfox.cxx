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

int main( int argc, char **argv )
{
    int errs = 0;
    MPI::File fh;
    MPI::Info info1, info2;
    bool flag;
    char filename[50];
    char *mykey;
    char *myvalue;

    MTest_Init( );

    mykey = new char [MPI::MAX_INFO_KEY];
    myvalue = new char [MPI::MAX_INFO_VAL];

    // Open a simple file
    strcpy( filename, "iotest.txt" );
    fh = MPI::File::Open( MPI::COMM_WORLD, filename, MPI::MODE_RDWR |
			  MPI::MODE_CREATE, MPI::INFO_NULL );

    // Try to set one of the available info hints  
    info1 = MPI::Info::Create();
    info1.Set( "access_style", "read_once,write_once" );
    fh.Set_info( info1 );
    info1.Free();
      
    info2 = fh.Get_info();
    flag = info2.Get( "filename", MPI::MAX_INFO_VAL, myvalue );

    // An implementation isn't required to provide the filename (though
    // a high-quality implementation should)
    if (flag) {
	// If we find it, we must have the correct name
	if (strcmp( myvalue, filename ) != 0 ||
	    strlen(myvalue) != 10) {
            errs++;
            cout << " Returned wrong value for the filename\n";
	}
    }
    info2.Free();

    fh.Close();
    
    MPI::COMM_WORLD.Barrier();
    if (MPI::COMM_WORLD.Get_rank() == 0) 
	MPI::File::Delete( filename, MPI::INFO_NULL );
    
    delete [] mykey;
    delete [] myvalue;

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
