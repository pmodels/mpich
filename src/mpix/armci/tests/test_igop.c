/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

#define DATA_SZ 100

int main(int argc, char ** argv) {
  int    rank, nproc, i;
  int   *buf;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCI GOP test with %d processes\n", nproc);

  buf = malloc(DATA_SZ*sizeof(int));

  if (rank == 0) printf(" - Testing ABSMIN\n");

  for (i = 0; i < DATA_SZ; i++)
    buf[i] = (rank+1) * ((i % 2) ? -1 : 1);

  armci_msg_igop(buf, DATA_SZ, "absmin");

  for (i = 0; i < DATA_SZ; i++)
    if (buf[i] != 1) {
      printf("Err: buf[%d] = %d expected 1\n", i, buf[i]);
      ARMCI_Error("Fail", 1);
    }

  if (rank == 0) printf(" - Testing ABSMAX\n");

  for (i = 0; i < DATA_SZ; i++)
    buf[i] = (rank+1) * ((i % 2) ? -1 : 1);

  armci_msg_igop(buf, DATA_SZ, "absmax");

  for (i = 0; i < DATA_SZ; i++)
    if (buf[i] != nproc) {
      printf("Err: buf[%d] = %d expected %d\n", i, buf[i], nproc);
      ARMCI_Error("Fail", 1);
    }

  free(buf);

  if (rank == 0) printf("Pass.\n");

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
