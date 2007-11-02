/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"

/* 
 * This test makes sure that each process can send to each other process.
 * If there are bugs in the handling of request completions or in 
 * queue operations, then this test may fail on them (it did with
 * early EagerShort handling).
 */

#define MAXPES 32
#define MYBUFSIZE 16*1024
static int buffer[MAXPES][MYBUFSIZE];

#define NUM_RUNS 10

int main ( int argc, char *argv[] )
{
  int i;
  int count, size;
  int self, npes;
  double secs;
  MPI_Request request[MAXPES];
  MPI_Status status;

  MTest_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &self);
  MPI_Comm_size (MPI_COMM_WORLD, &npes);

  if (npes > MAXPES) {
    fprintf( stderr, "This program requires a comm_world no larger than %d",
	     MAXPES );
    MPI_Abort( MPI_COMM_WORLD, 1 );
  }

  for (size = 1; size  <= MYBUFSIZE ; size += size) {
      secs = -MPI_Wtime ();
      for (count = 0; count < NUM_RUNS; count++) {
	  MPI_Barrier (MPI_COMM_WORLD);

	  for (i = 0; i < npes; i++) {
	      if (i != self)
		MPI_Irecv (buffer[i], size, MPI_INT, i,
			 MPI_ANY_TAG, MPI_COMM_WORLD, &request[i]);
	    }

	  for (i = 0; i < npes; i++) {
	      if (i != self)
		MPI_Send (buffer[self], size, MPI_INT, i, 0, MPI_COMM_WORLD);
	    }

	  for (i = 0; i < npes; i++) {
	      if (i != self)
		MPI_Wait (&request[i], &status);
	    }

	}
      MPI_Barrier (MPI_COMM_WORLD);
      secs += MPI_Wtime ();

      if (self == 0) {
	  secs = secs / (double) NUM_RUNS;
	  MTestPrintfMsg( 1, "length = %d ints\n", size );
	}
    }

  /* Simple completion is all that we normally ask of this program */

  MTest_Finalize( 0 );

  MPI_Finalize();
  return 0;
}
