/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/** ARMCI Mutex RMW Test -- James Dinan <dinan@mcs.anl.gov>
  * 
  * A mutex and shared integer live on process 0.  All processes lock, add a
  * value to the integer, and unlock.  Process 0 confirms the final result.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define NITER 1000
#define ADDIN 5

int main(int argc, char ** argv) {
  int    rank, nproc, val, i;
  void **base_ptrs;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI mutex read-modify-write test with %d processes\n", nproc);

  base_ptrs = malloc(nproc*sizeof(void*));

  ARMCI_Create_mutexes(rank == 0 ? 1 : 0);
  ARMCI_Malloc(base_ptrs, (rank == 0) ? sizeof(int) : 0); // Proc 0 has a shared int

  if (rank == 0) {
    val = 0;
    ARMCI_Put(&val, base_ptrs[0], sizeof(int), 0);
  }

  ARMCI_Barrier();

  for (i = 0; i < NITER; i++) {
    ARMCI_Lock(0, 0);

    ARMCI_Get(base_ptrs[0], &val, sizeof(int), 0);
    val += ADDIN;
    ARMCI_Put(&val, base_ptrs[0], sizeof(int), 0);

    ARMCI_Unlock(0, 0);
  }

  printf(" + %3d done\n", rank);
  fflush(NULL);

  ARMCI_Barrier();

  if (rank == 0) {
    ARMCI_Get(base_ptrs[0], &val, sizeof(int), 0);

    if (val == ADDIN*nproc*NITER)
      printf("Test complete: PASS.\n");
    else
      printf("Test complete: FAIL.  Got %d, expected %d.\n", val, ADDIN*nproc*NITER);
  }

  ARMCI_Free(base_ptrs[rank]);
  ARMCI_Destroy_mutexes();

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
