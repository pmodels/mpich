/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <armcix.h>

#define PART_SIZE 1
#define DATA_SZ   100*sizeof(int)

int main(int argc, char **argv) {
  int          me, nproc, grp_me, grp_nproc;
  ARMCI_Group  g_world, g_new;
  void       **base_ptrs;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  base_ptrs = malloc(sizeof(void*)*nproc);

  if (me == 0) printf("ARMCI Group test starting on %d procs\n", nproc);

  ARMCI_Group_get_world(&g_world);
  
  if (me == 0) printf(" + Creating odd/even groups\n");

  ARMCIX_Group_split(&g_world, me%2, me, &g_new);

  ARMCI_Group_rank(&g_new, &grp_me);
  ARMCI_Group_size(&g_new, &grp_nproc);

  if (me == 0) printf(" + Performing group allocation\n");
  ARMCI_Malloc_group(base_ptrs, DATA_SZ, &g_new);
  ARMCI_Barrier();

  if (me == 0) printf(" + Freeing group allocation\n");

  ARMCI_Free_group(base_ptrs[grp_me], &g_new);
  ARMCI_Barrier();

  if (me == 0) printf(" + Freeing group\n");

  ARMCI_Group_free(&g_new);

  if (me == 0) printf(" + done\n");

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
