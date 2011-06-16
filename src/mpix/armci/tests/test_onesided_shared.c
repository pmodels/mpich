/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define VERBOSE        0
#define DATA_NELTS     1000
#define NUM_ITERATIONS 10
#define DATA_SZ        (DATA_NELTS*sizeof(int))

int main(int argc, char ** argv) {
  int    rank, nproc, i, test_iter;
  int   *my_data, *buf;
  void **base_ptrs;
  void **buf_shared;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI test with %d processes\n", nproc);

  base_ptrs  = malloc(sizeof(void*)*nproc);
  buf_shared = malloc(sizeof(void*)*nproc);

  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    if (rank == 0) printf(" + iteration %d\n", test_iter);

    if (rank == 0 && VERBOSE) printf("   - Allocating shared buffers\n");

    /*** Allocate the shared array ***/
    ARMCI_Malloc(base_ptrs,  DATA_SZ);
    ARMCI_Malloc(buf_shared, DATA_SZ);

    buf     = buf_shared[rank];
    my_data = base_ptrs[rank];

    if (rank == 0 && VERBOSE) printf("   - Testing one-sided get\n");

    /*** Get from our right neighbor and verify correct data ***/
    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) my_data[i] = rank*test_iter;
    ARMCI_Access_end(my_data);

    ARMCI_Barrier(); // Wait for all updates to data to complete

    ARMCI_Get(base_ptrs[(rank+1) % nproc], buf, DATA_SZ, (rank+1) % nproc);

    ARMCI_Access_begin(buf);

    for (i = 0; i < DATA_NELTS; i++) {
      if (buf[i] != ((rank+1) % nproc)*test_iter) {
        printf("%d: GET expected %d, got %d\n", rank, (rank+1) % nproc, buf[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }

    ARMCI_Access_end(buf);

    ARMCI_Barrier(); // Wait for all gets to complete

    if (rank == 0 && VERBOSE) printf("   - Testing one-sided put\n");

    /*** Put to our left neighbor and verify correct data ***/
    for (i = 0; i < DATA_NELTS; i++) buf[i] = rank*test_iter;
    ARMCI_Put(buf, base_ptrs[(rank+nproc-1) % nproc], DATA_SZ, (rank+nproc-1) % nproc);

    ARMCI_Barrier(); // Wait for all updates to data to complete

    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) {
      if (my_data[i] != ((rank+1) % nproc)*test_iter) {
        printf("%d: PUT expected %d, got %d\n", rank, (rank+1) % nproc, my_data[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }
    ARMCI_Access_end(my_data);

    ARMCI_Barrier(); // Wait for all gets to complete

    if (rank == 0 && VERBOSE) printf("   - Testing one-sided accumlate\n");

    /*** Accumulate to our left neighbor and verify correct data ***/
    ARMCI_Access_begin(buf);
    for (i = 0; i < DATA_NELTS; i++) buf[i] = rank;
    ARMCI_Access_end(buf);
    
    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) my_data[i] = rank;
    ARMCI_Access_end(my_data);
    ARMCI_Barrier();

    int scale = test_iter;
    ARMCI_Acc(ARMCI_ACC_INT, &scale, buf, base_ptrs[(rank+nproc-1) % nproc], DATA_SZ, (rank+nproc-1) % nproc);

    ARMCI_Barrier(); // Wait for all updates to data to complete

    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) {
      if (my_data[i] != rank + ((rank+1) % nproc)*test_iter) {
        printf("%d: ACC expected %d, got %d\n", rank, (rank+1) % nproc, my_data[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }
    ARMCI_Access_end(my_data);

    if (rank == 0 && VERBOSE) printf("   - Freeing shared buffers\n");

    ARMCI_Free(my_data);
    ARMCI_Free(buf);
  }

  free(base_ptrs);

  if (rank == 0) printf("Test complete: PASS.\n");

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
