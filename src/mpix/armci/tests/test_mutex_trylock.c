/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/** ARMCI Mutex test -- James Dinan <dinan@mcs.anl.gov>
  * 
  * All processes create N mutexes then lock+unlock all mutexes on all
  * processes.  Locking is accomplished via trylock in a loop.
  */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <mpi.h>
#include <armci.h>
#include <armcix.h>

#define NUM_MUTEXES 10

int main(int argc, char ** argv) {
  int rank, nproc, i, j;
  armcix_mutex_hdl_t mhdl;
  ARMCI_Group world_group;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting ARMCIX mutex test with %d processes\n", nproc);

  ARMCI_Group_get_world(&world_group);
  mhdl = ARMCIX_Create_mutexes_hdl(NUM_MUTEXES, &world_group);

  for (i = 0; i < nproc; i++)
    for (j = 0; j < NUM_MUTEXES; j++) {
      while (ARMCIX_Trylock_hdl(mhdl, j, (rank+i)%nproc))
        ;
      ARMCIX_Unlock_hdl(mhdl, j, (rank+i)%nproc);
    }

  printf(" + %3d done\n", rank);
  fflush(NULL);

  ARMCIX_Destroy_mutexes_hdl(mhdl);

  if (rank == 0) printf("Test complete: PASS.\n");

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
