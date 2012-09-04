/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define DATA_NELTS     1000
#define NUM_ITERATIONS 10
#define DATA_SZ        (DATA_NELTS*sizeof(int))

       int armci_calls = 0;
extern int parmci_calls;

int main(int argc, char ** argv) {
  int    rank, nproc, i, test_iter;
  int   *my_data, *buf;
  void **base_ptrs;

  MPI_Init(&argc, &argv);
  ARMCI_Init();
  armci_calls++;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI test with %d processes\n", nproc);

  buf = malloc(DATA_SZ);
  base_ptrs = malloc(sizeof(void*)*nproc);

  for (test_iter = 0; test_iter < NUM_ITERATIONS; test_iter++) {
    if (rank == 0) printf(" + iteration %d\n", test_iter);

    /*** Allocate the shared array ***/
    ARMCI_Malloc(base_ptrs, DATA_SZ);
    my_data = base_ptrs[rank];

    /*** Get from our right neighbor and verify correct data ***/
    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) my_data[i] = rank*test_iter;
    ARMCI_Access_end(my_data);

    ARMCI_Barrier(); // Wait for all updates to data to complete
    armci_calls++;

    ARMCI_Get(base_ptrs[(rank+1) % nproc], buf, DATA_SZ, (rank+1) % nproc);
    armci_calls++;

    for (i = 0; i < DATA_NELTS; i++) {
      if (buf[i] != ((rank+1) % nproc)*test_iter) {
        printf("%d: GET expected %d, got %d\n", rank, (rank+1) % nproc, buf[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }

    ARMCI_Barrier(); // Wait for all gets to complete
    armci_calls++;

    /*** Put to our left neighbor and verify correct data ***/
    for (i = 0; i < DATA_NELTS; i++) buf[i] = rank*test_iter;
    ARMCI_Put(buf, base_ptrs[(rank+nproc-1) % nproc], DATA_SZ, (rank+nproc-1) % nproc);
    armci_calls++;

    ARMCI_Barrier(); // Wait for all updates to data to complete
    armci_calls++;

    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) {
      if (my_data[i] != ((rank+1) % nproc)*test_iter) {
        printf("%d: PUT expected %d, got %d\n", rank, (rank+1) % nproc, my_data[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }
    ARMCI_Access_end(my_data);

    ARMCI_Barrier(); // Wait for all gets to complete
    armci_calls++;

    /*** Accumulate to our left neighbor and verify correct data ***/
    for (i = 0; i < DATA_NELTS; i++) buf[i] = rank;
    
    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) my_data[i] = rank;
    ARMCI_Access_end(my_data);
    ARMCI_Barrier();
    armci_calls++;

    int scale = test_iter;
    ARMCI_Acc(ARMCI_ACC_INT, &scale, buf, base_ptrs[(rank+nproc-1) % nproc], DATA_SZ, (rank+nproc-1) % nproc);

    ARMCI_Barrier(); // Wait for all updates to data to complete
    armci_calls++;

    ARMCI_Access_begin(my_data);
    for (i = 0; i < DATA_NELTS; i++) {
      if (my_data[i] != rank + ((rank+1) % nproc)*test_iter) {
        printf("%d: ACC expected %d, got %d\n", rank, (rank+1) % nproc, my_data[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }
    ARMCI_Access_end(my_data);

    ARMCI_Free(my_data);
  }

  free(buf);
  free(base_ptrs);
  
  if (armci_calls == parmci_calls) {
    if (rank == 0) {
      printf("Profiling check ok: %d recorded == %d profiled calls\n", armci_calls, parmci_calls);
      printf("Test complete: PASS.\n");
    }
  } else {
    printf("%d: Profiling check failed -- %d recorded != %d profiled calls\n", rank, armci_calls, parmci_calls);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  ARMCI_Finalize();
  armci_calls++;

  MPI_Finalize();

  return 0;
}
