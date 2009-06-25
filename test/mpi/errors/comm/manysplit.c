/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

/* Test that comm_split fails gracefully when the number of communicators
   is exhausted.  Original version submitted by Paul Sack.
*/
#define MAX_COMMS 2000

/* Define VERBOSE to get an output line for every communicator */
/* #define VERBOSE 1 */
int main(int argc, char* argv[])
{
  int i, rank;
  int rc, maxcomm;
  int errs = 0;
  MPI_Comm comm[MAX_COMMS];

  MTest_Init( &argc, &argv );
  MPI_Comm_rank ( MPI_COMM_WORLD, &rank );
  MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

  maxcomm = -1;
  for (i = 0; i < MAX_COMMS; i++) {
#ifdef VERBOSE
    if (rank == 0) {
      if (i % 20 == 0) {
	fprintf(stderr, "\n %d: ", i); fflush(stdout);
      }
      else {
	fprintf(stderr, "."); fflush(stdout);
      }
    }
#endif
    rc = MPI_Comm_split(MPI_COMM_WORLD, 1, rank, &comm[i]);
    if (rc != MPI_SUCCESS) {
      break;
    }
    maxcomm = i;
  }
  for (i=0; i<=maxcomm; i++) {
    MPI_Comm_free( &comm[i] );
  }
  /* If we complete, there are no errors */
  MTest_Finalize( errs );
  MPI_Finalize();
  return 0;
}
