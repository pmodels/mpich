/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
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

/*
  This program tests MPI_Alltoallw by having processor i send different
  amounts of data to each processor.  This is just the MPI_Alltoallv test,
  but with displacements in bytes rather than units of the datatype.

  Because there are separate send and receive types to alltoallw,
  there need to be tests to rearrange data on the fly.  Not done yet.
  
  The first test sends i items to processor i from all processors.

  Currently, the test uses only MPI_INT; this is adequate for testing systems
  that use point-to-point operations
 */

int main( int argc, char **argv )
{
    MPI::Intracomm comm;
    int      *sbuf, *rbuf;
    int      rank, size;
    int      *sendcounts, *recvcounts, *rdispls, *sdispls;
    int      i, j, *p, err;
    MPI::Datatype *sendtypes, *recvtypes;
    
    MTest_Init( );
    err = 0;
    
    while (MTestGetIntracommGeneral( comm, 2, true )) {
      if (comm == MPI::COMM_NULL) continue;

      /* Create the buffer */
      size = comm.Get_size();
      rank = comm.Get_rank();
      sbuf = new int [ size * size ];
      rbuf = new int [ size * size ];
      if (!sbuf || !rbuf) {
	cout << "Could not allocate buffers!\n";
	comm.Abort( 1 );
      }
      
      /* Load up the buffers */
      for (i=0; i<size*size; i++) {
	sbuf[i] = i + 100*rank;
	rbuf[i] = -i;
      }
      
      /* Create and load the arguments to alltoallv */
      sendcounts = new int [ size ];
      recvcounts = new int [ size ];
      rdispls    = new int [ size ];
      sdispls    = new int [ size ];
      sendtypes    = new MPI::Datatype [size];
      recvtypes    = new MPI::Datatype [size];
      if (!sendcounts || !recvcounts || !rdispls || !sdispls || !sendtypes || !recvtypes) {
	  cout << "Could not allocate arg items!\n";
	  comm.Abort( 1 );
      }
      /* Note that process 0 sends no data (sendcounts[0] = 0) */
      for (i=0; i<size; i++) {
	sendcounts[i] = i;
	recvcounts[i] = rank;
	rdispls[i]    = i * rank * sizeof(int);
	sdispls[i]    = (((i+1) * (i))/2) * sizeof(int);
        sendtypes[i] = recvtypes[i] = MPI::INT;
      }
      comm.Alltoallw( sbuf, sendcounts, sdispls, sendtypes,
		      rbuf, recvcounts, rdispls, recvtypes );
      
      /* Check rbuf */
      for (i=0; i<size; i++) {
	p = rbuf + rdispls[i]/sizeof(int);
	for (j=0; j<rank; j++) {
	  if (p[j] != i * 100 + (rank*(rank+1))/2 + j) {
	      cout << "[" << rank << "] got " << p[j] << " expected " <<
		  (i*(i+1))/2 + j << " for " << j << "th\n";
	      err++;
	  }
	}
      }

      delete [] sendtypes;
      delete [] recvtypes;
      delete [] sdispls;
      delete [] rdispls;
      delete [] recvcounts;
      delete [] sendcounts;
      delete [] rbuf;
      delete [] sbuf;
      MTestFreeComm(comm);
    }

    MTest_Finalize( err );
    MPI::Finalize();
    return 0;
}
