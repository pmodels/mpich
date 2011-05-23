/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include <armci.h>

int main(int argc, char ** argv) {
  MPI_Init(&argc, &argv);
  ARMCI_Init();

  ARMCI_Get(NULL, NULL, 1, 0);

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
