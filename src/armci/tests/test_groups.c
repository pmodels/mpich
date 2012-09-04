/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>

#define PART_SIZE 1

int main(int argc, char **argv) {
  int                      me, nproc;
  int                      i, *procs;
  ARMCI_Group              g_world, g_odd, g_even;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  procs = malloc(sizeof(int) * ( nproc/2 + (nproc % 2 ? 1 : 0 )));

  if (me == 0) printf("ARMCI Group test starting on %d procs\n", nproc);

  ARMCI_Group_get_world(&g_world);
  
  if (me == 0) printf(" + Creating odd group\n");

  for (i = 1; i < nproc; i += 2) {
    procs[i/2] = i;
  }

  ARMCI_Group_create_child(i/2, procs, &g_odd, &g_world);

  if (me == 0) printf(" + Creating even group\n");

  for (i = 0; i < nproc; i += 2) {
    procs[i/2] = i;
  }

  ARMCI_Group_create_child(i/2, procs, &g_even, &g_world);

  if (me == 0) printf(" + Freeing groups\n");

  if (me % 2 > 0)
    ARMCI_Group_free(&g_odd);
  else
    ARMCI_Group_free(&g_even);

  free(procs);

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
