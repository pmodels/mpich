/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/** ARMCI RMW-FADD test -- James Dinan <dinan@mcs.anl.gov>
  * 
  * All processes allocate one shared integer counter per process.  All
  * processes perform NINC atomic fetch-and-add operations on every counter.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define NINC 100

#ifdef USE_ARMCI_LONG
#  define INC_TYPE long
#  define ARMCI_OP ARMCI_FETCH_AND_ADD_LONG
#else
#  define INC_TYPE int
#  define ARMCI_OP ARMCI_FETCH_AND_ADD
#endif

int main(int argc, char ** argv) {
  int        errors = 0;
  int        rank, nproc, i, j;
  void     **base_ptrs;
  INC_TYPE   val;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI RMW-FADD test with %d processes\n", nproc);

  base_ptrs = malloc(sizeof(void*)*nproc);
  ARMCI_Malloc(base_ptrs, sizeof(INC_TYPE));

  ARMCI_Access_begin(base_ptrs[rank]);
  *(int*) base_ptrs[rank] = 0;
  ARMCI_Access_end(base_ptrs[rank]);

  ARMCI_Barrier();

  for (i = 0; i < NINC; i++) {
    for (j = 0; j < nproc; j++) {
      ARMCI_Rmw(ARMCI_OP, &val, base_ptrs[j], 1, j);
    }
  }

  ARMCI_Barrier();

  ARMCI_Access_begin(base_ptrs[rank]);
  if (*(int*) base_ptrs[rank] != NINC*nproc) {
    errors++;
    printf("%3d -- Got %d, expected %d\n", rank, *(int*) base_ptrs[rank], NINC*nproc);
  }
  ARMCI_Access_end(base_ptrs[rank]);

  armci_msg_igop(&errors, 1, "+");

  if (rank == 0) {
    if (errors == 0) printf("Test complete: PASS.\n");
    else            printf("Test fail: %d errors.\n", errors);
  }

  ARMCI_Free(base_ptrs[rank]);
  free(base_ptrs);

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
