/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/** ARMCI Mutex test -- James Dinan <dinan@mcs.anl.gov>
  * 
  * All processes create N mutexes then lock+unlock all mutexes on all
  * processes.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define NUM_MUTEXES 10

int main(int argc, char ** argv) {
  int rank, nproc, i, j;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI mutex test with %d processes\n", nproc);

  ARMCI_Create_mutexes(NUM_MUTEXES);

  for (i = 0; i < nproc; i++)
    for (j = 0; j < NUM_MUTEXES; j++) {
      ARMCI_Lock(  j, (rank+i)%nproc);
      ARMCI_Unlock(j, (rank+i)%nproc);
    }

  printf(" + %3d done\n", rank);
  fflush(NULL);

  ARMCI_Destroy_mutexes();

  if (rank == 0) printf("Test complete: PASS.\n");

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
