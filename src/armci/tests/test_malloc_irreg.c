/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/** ARMCI Irregular memory allocation test -- James Dinan <dinan@mcs.anl.gov>
  * 
  * Perform a series of allocations where all processes but one give zero bytes
  * to ARMCI_Malloc.  The process that does a non-zero allocation initializes
  * their shared memory and then the data is fetched by a neighbor and tested.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define DATA_NELTS     1000
#define NUM_ITERATIONS 10
#define DATA_SZ        (DATA_NELTS*sizeof(int))

int main(int argc, char ** argv) {
  int     rank, nproc, test_iter, i;
  void ***base_ptrs;
  int    *buf;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI irregular memory allocation test with %d processes\n", nproc);

  buf       = malloc(DATA_SZ);
  base_ptrs = malloc(sizeof(void**)*NUM_ITERATIONS);

  // Perform a pile of allocations
  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    if (rank == 0) printf(" + allocation %d\n", test_iter);

    base_ptrs[test_iter] = malloc(sizeof(void*)*nproc);
    ARMCI_Malloc((void**)base_ptrs[test_iter], (test_iter % nproc == rank) ? DATA_SZ : 0);
  }

  ARMCI_Barrier();

  // Initialize data to my rank
  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    if (test_iter % nproc == rank) {
      ARMCI_Access_begin(base_ptrs[test_iter][rank]);
      for (i = 0; i < DATA_NELTS; i++)
        ((int*)base_ptrs[test_iter][rank])[i] = rank;
      ARMCI_Access_end(base_ptrs[test_iter][rank]);
    }
  }

  ARMCI_Barrier();

  // Fetch and test
  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    ARMCI_Get(base_ptrs[test_iter][test_iter%nproc], buf, DATA_SZ, test_iter%nproc);
    for (i = 0; i < DATA_NELTS; i++) {
      if (buf[i] != test_iter % nproc)
        printf("Error: got %d expected %d\n", buf[i], test_iter%nproc);
    }
  }

  // Free all allocations
  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    if (rank == 0) printf(" + free %d\n", test_iter);

    ARMCI_Free(((void**)base_ptrs[test_iter])[rank]);
    free(base_ptrs[test_iter]);
  }

  ARMCI_Barrier();

  free(base_ptrs);
  free(buf);

  if (rank == 0) printf("Test complete: PASS.\n");

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
