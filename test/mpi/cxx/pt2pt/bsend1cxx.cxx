/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Thanks to Jim Hoekstra of Iowa State University for several important
   bug fixes */

#include "mpi.h"
#include "mpitestcxx.h"
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
/* Needed for strcmp */
#include <string.h>

/* Rather than deal with the need to declare strncpy correctly 
   (avoiding possible conflicts with other header files), we 
   simply define a copy routine that is used for this example */
void CopyChars( char *, int, const char * );
void CopyChars( char *dest, int len, const char *src )
{
    int i;
    for (i=0; i<len && *src; i++) {
	dest[i] = src[i];
    }
    if (i < len) dest[i] = 0;
}

/* 
 * This is a simple program that tests bsend.  It may be run as a single
 * process to simplify debugging; in addition, bsend allows send-to-self
 * programs.
 */
int main( int argc, char *argv[] )
{
    MPI::Intracomm comm;
    int dest = 0, src = 0, tag = 1;
    char *buf;
    void *bbuf;
    char msg1[7], msg3[17];
    double msg2[2];
    char rmsg1[64], rmsg3[64];
    double rmsg2[64];
    int errs = 0, rank;
    int bufsize, bsize;
    MPI::Status status;

    MTest_Init();
    comm = MPI::COMM_WORLD;
    rank = comm.Get_rank();

    bufsize = 3 * MPI_BSEND_OVERHEAD + 7 + 17 + 2*sizeof(double);
    buf = new char[bufsize];
    MPI::Attach_buffer( buf, bufsize );

    CopyChars( msg1, sizeof(msg1), "012345" );
    CopyChars( msg3, sizeof(msg3), "0123401234012341" );
    msg2[0] = 1.23; msg2[1] = 3.21;

    if (rank == src) {
	/* These message sizes are chosen to expose any alignment problems */
	comm.Bsend( msg1, 7, MPI::CHAR, dest, tag );
	comm.Bsend( msg2, 2, MPI::DOUBLE, dest, tag );
	comm.Bsend( msg3, 17, MPI::CHAR, dest, tag );
    }

    if (rank == dest) {
	comm.Recv( rmsg1, 7, MPI::CHAR, src, tag, status );
	comm.Recv( rmsg2, 10, MPI::DOUBLE, src, tag, status );
	comm.Recv( rmsg3, 17, MPI::CHAR, src, tag, status  );

	if (strcmp( rmsg1, msg1 ) != 0) {
	    errs++;
	    cerr << "message 1 (" << rmsg1 << ") should be " << msg1 << "\n";
	}
	if (rmsg2[0] != msg2[0] || rmsg2[1] != msg2[1]) {
	    errs++;
	    cerr << "message 2 incorrect, values are (" << rmsg2[0] << "," 
		 << rmsg2[1] << ") but should be (" << msg2[0] << "," 
		 << msg2[1] << "\n";
	}
	if (strcmp( rmsg3, msg3 ) != 0) {
	    errs++;
	    cerr << "message 3 (" << rmsg3 << ") should be " << msg3 << "\n";
	}
    }

    /* We can't guarantee that messages arrive until the detach */
    bsize = MPI::Detach_buffer( bbuf );
    if (bbuf != (void *)buf) {
	errs++;
	cerr << "Returned buffer from detach is not the same as the initial buffer\n";
    }

    delete buf;

    MTest_Finalize( errs );

    MPI::Finalize();

    return 0;
}
