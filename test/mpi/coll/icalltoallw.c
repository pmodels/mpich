/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

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

    MPI_Comm comm;
    int      *sbuf, *rbuf;
    int      rank, size, lsize, asize;
    int      *sendcounts, *recvcounts, *rdispls, *sdispls;
    int      i, j, *p, err;
    MPI_Datatype *sendtypes, *recvtypes;
    int      leftGroup;

    MTest_Init( &argc, &argv );
    err = 0;

    while (MTestGetIntercomm( &comm, &leftGroup, 4 )) {
      if (comm == MPI_COMM_NULL) continue;

      /* Create the buffer */
      MPI_Comm_size( comm, &lsize );
      MPI_Comm_remote_size( comm, &size );
      asize = (lsize > size) ? lsize : size;
      MPI_Comm_rank( comm, &rank );
      sbuf = (int *)malloc( size * size * sizeof(int) );
      rbuf = (int *)malloc( asize * asize * sizeof(int) );
      if (!sbuf || !rbuf) {
	fprintf( stderr, "Could not allocated buffers!\n" );
	MPI_Abort( comm, 1 );
      }
      
      /* Load up the buffers */
      for (i=0; i<size*size; i++) {
	sbuf[i] = i + 100*rank;
	rbuf[i] = -i;
      }

      /* Create and load the arguments to alltoallv */
      sendcounts = (int *)malloc( size * sizeof(int) );
      recvcounts = (int *)malloc( size * sizeof(int) );
      rdispls    = (int *)malloc( size * sizeof(int) );
      sdispls    = (int *)malloc( size * sizeof(int) );
      sendtypes  = (MPI_Datatype *)malloc( size * sizeof(MPI_Datatype) );
      recvtypes  = (MPI_Datatype *)malloc( size * sizeof(MPI_Datatype) );
      if (!sendcounts || !recvcounts || !rdispls || !sdispls || !sendtypes || !recvtypes) {
	fprintf( stderr, "Could not allocate arg items!\n" );
	MPI_Abort( comm, 1 );
      }
      /* Note that process 0 sends no data (sendcounts[0] = 0) */
      for (i=0; i<size; i++) {
	sendcounts[i] = i;
	sdispls[i]    = (((i+1) * (i))/2) * sizeof(int);
        sendtypes[i]  = MPI_INT;
	recvcounts[i] = rank;
	rdispls[i]    = i * rank * sizeof(int);
	recvtypes[i]  = MPI_INT;
      }
      MPI_Alltoallw( sbuf, sendcounts, sdispls, sendtypes,
		     rbuf, recvcounts, rdispls, recvtypes, comm );
      
      /* Check rbuf */
      for (i=0; i<size; i++) {
	p = rbuf + rdispls[i]/sizeof(int);
	for (j=0; j<rank; j++) {
	  if (p[j] != i * 100 + (rank*(rank+1))/2 + j) {
	    fprintf( stderr, "[%d] got %d expected %d for %dth\n",
		     rank, p[j],(i*(i+1))/2 + j, j );
	    err++;
	  }
	}
      }

      free(sendtypes);
      free(recvtypes);
      free( sdispls );
      free( rdispls );
      free( recvcounts );
      free( sendcounts );
      free( rbuf );
      free( sbuf );
      MTestFreeComm( &comm );
    }

    MTest_Finalize( err );
    MPI_Finalize();
    return 0;
}
