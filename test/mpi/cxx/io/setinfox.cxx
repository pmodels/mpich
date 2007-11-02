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

static char MTEST_Descrip[] = "Test file_set_view";

/* 
 * access style is explicitly described as modifiable.  values include
 * read_once, read_mostly, write_once, write_mostlye, random
 *
 * 
 */
int main( int argc, char *argv[] )
{
    int             errs = 0, err;
    int             buf[10];
    int             rank;
    MPI::Intracomm  comm;
    MPI::Status     status;
    MPI::File       fh;
    MPI::Info       infoin, infoout;
    char            value[1024];
    bool            flag;
    int             count;

    MTest_Init( );
    comm = MPI::COMM_WORLD;

    rank = comm.Get_rank();
    infoin = MPI::Info::Create();
    infoin.Set( "access_style", "write_once,random" );
    fh = MPI::File::Open( comm, "testfile", MPI::MODE_RDWR | MPI::MODE_CREATE,
			  infoin);
    buf[0] = rank;
    try {
	fh.Write_ordered( buf, 1, MPI::INT );
    } catch ( MPI::Exception e  ) {
	errs ++;
	MTestPrintError( e.Get_error_code() );
    }

    infoin.Set( "access_style", "read_once" );
    try {
	fh.Seek_shared( 0, MPI_SEEK_SET );   // Use MPI_xx to avoid problems with SEEK_SET
    } catch ( MPI::Exception e ) {
	errs ++;
	MTestPrintError( e.Get_error_code() );
    }

    try {
	fh.Set_info( infoin );
    } catch ( MPI::Exception e ) {
	errs ++;
	MTestPrintError( e.Get_error_code() );
    }
    buf[0] = -1;
    
    try {
	fh.Read_ordered( buf, 1, MPI::INT, status );
    } catch ( MPI::Exception e ) {
	errs ++;
	MTestPrintError( e.Get_error_code() );
    }
    count = status.Get_count( MPI::INT );
    if (count != 1) {
	errs++;
	cout << "Expected to read one int, read " << count << "\n";
    }
    if (buf[0] != rank) {
	errs++;
	cout << "Did not read expected value (" << buf[0] << ")\n";
    }

    try {
	infoout = fh.Get_info();
    } catch ( MPI::Exception e ) {
	errs ++;
	MTestPrintError( e.Get_error_code() );
    }
    flag = infoout.Get( "access_style", 1024, value );
    /* Note that an implementation is allowed to ignore the set_info,
       so we'll accept either the original or the updated version */
    if (!flag) {
	;
	/*
	errs++;
	printf( "Access style hint not saved\n" );
	*/
    }
    else {
	if (strcmp( value, "read_once" ) != 0 &&
	    strcmp( value, "write_once,random" ) != 0) {
	    errs++;
	    cout << "value for access_style unexpected; is " << value << "\n";
	}
    }
    infoout.Free();

    try {
	fh.Close();
    } catch ( MPI::Exception e ) {
	errs ++;
	MTestPrintError( e.Get_error_code() );
    }
    comm.Barrier();
    rank = comm.Get_rank();
    if (rank == 0) {
	try {
	    MPI::File::Delete( "testfile", MPI::INFO_NULL );
	} catch ( MPI::Exception e ) {
	    errs ++;
	    MTestPrintError( e.Get_error_code() );
	}
    }
    
    MTest_Finalize( errs );
    MPI::Finalize( );
    return 0;
}
