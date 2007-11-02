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
// Those that do need the std namespace; otherwise, a bare "cout"
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
    MPI::Intracomm comm;
    MPI::File fh;
    int r, s, i;
    int fileintsize;
    int errs = 0;
    char filename[1024];
    MPI::Offset offset;

    MTest_Init();

    strcpy( filename,"iotest.txt");
    comm = MPI::COMM_WORLD;
    s = comm.Get_size();
    r = comm.Get_rank();
    // Try writing the file, then check it
    fh = MPI::File::Open( comm, filename, MPI::MODE_RDWR | MPI::MODE_CREATE, 
			  MPI::INFO_NULL );

    // Get the size of an INT in the file
    fileintsize = fh.Get_type_extent( MPI::INT );

    // We let each process write in turn, getting the position after each 
    // write
    for (i=0; i<s; i++) {
	if (i == r) {
	    fh.Write_shared( &i, 1, MPI::INT );
	}
        comm.Barrier();
	offset = fh.Get_position_shared();
	if (offset != fileintsize * (i+1)) {
	    errs++;
	    cout << r << " Shared position is " <<  offset <<
		" should be " << fileintsize * (i+1) << "\n";
	}
        comm.Barrier();
    }

    fh.Close();
    comm.Barrier();
    if (r == 0) {
	MPI::File::Delete( filename, MPI::INFO_NULL );
    }
    
    MTest_Finalize( errs );
    MPI::Finalize();
}
