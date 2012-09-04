/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/** ARMCI Malloc test -- James Dinan <dinan@mcs.anl.gov>
  * 
  * Perform a pile of allocations and then free them.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define DATA_NELTS     1000
#define NUM_ITERATIONS 100
#define DATA_SZ        (DATA_NELTS*sizeof(int))

int main(int argc, char ** argv) {
  int     rank, nproc, test_iter;
  void ***base_ptrs;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI memory allocation test with %d processes\n", nproc);

  base_ptrs = malloc(sizeof(void**)*NUM_ITERATIONS);

  // Perform a pile of allocations
  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    if (rank == 0) printf(" + allocation %d\n", test_iter);

    base_ptrs[test_iter] = malloc(sizeof(void*)*nproc);
    ARMCI_Malloc((void**)base_ptrs[test_iter], (test_iter % 4 == 0) ? 0 : DATA_SZ);
  }

  ARMCI_Barrier();

  // Free all allocations
  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    if (rank == 0) printf(" + free %d\n", test_iter);

    ARMCI_Free(((void**)base_ptrs[test_iter])[rank]);
    free(base_ptrs[test_iter]);
  }

  free(base_ptrs);

  if (rank == 0) printf("Test complete: PASS.\n");

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
