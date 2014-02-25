/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/** MPI Mutex test -- James Dinan <dinan@mcs.anl.gov>
  * 
  * All processes create N mutexes then lock+unlock all mutexes on all
  * processes.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>

#define NUM_MUTEXES 100

const int verbose = 0;

int main(int argc, char ** argv) {
  int rank, nproc, i, j;
  MPIX_Mutex mtx;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0 && verbose)
    printf("Starting mutex test with %d processes\n", nproc);

  MPIX_Mutex_create(NUM_MUTEXES, MPI_COMM_WORLD, &mtx);

  for (i = 0; i < nproc; i++)
    for (j = 0; j < NUM_MUTEXES; j++) {
      MPIX_Mutex_lock(  mtx, j, (rank+i)%nproc);
      MPIX_Mutex_unlock(mtx, j, (rank+i)%nproc);
    }

  if (verbose) {
    printf(" + %3d done\n", rank);
    fflush(NULL);
  }

  MPIX_Mutex_free(&mtx);

  if (rank == 0)
    printf(" No Errors\n");

  MPI_Finalize();

  return 0;
}
