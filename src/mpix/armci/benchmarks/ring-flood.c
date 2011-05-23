/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <mpi.h>
#include <armci.h>

#define MAX_XFER_SIZE 8192
#define NUM_XFERS     1024

int main(int argc, char **argv) {
  int          me, nproc;
  int          msg_length, i;
  double       t_start, t_stop;
  armci_hdl_t *handles;  // Non-blocking handles (NUM_XFERS)
  uint8_t    *snd_buf;  // Send buffer    (MAX_XFER_SIZE)
  uint8_t   **rcv_buf;  // Receive buffer (MAX_XFER_SIZE * NUM_XFERS)

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (nproc < 2)
    ARMCI_Error("This benchmark should be run on at least two processes", 1);

  if (me == 0)
    printf("ARMCI flood bandwidth test, performing %d non-blocking xfers at each size.\n\n", NUM_XFERS);

  handles = malloc(NUM_XFERS*sizeof(armci_hdl_t));

  rcv_buf = malloc(nproc*sizeof(void*));
  ARMCI_Malloc((void*)rcv_buf, MAX_XFER_SIZE*NUM_XFERS);

  snd_buf = ARMCI_Malloc_local(MAX_XFER_SIZE);

  for (i = 0; i < MAX_XFER_SIZE; i++) {
    snd_buf[i] = (uint8_t) me;
  }

  for (msg_length = 1; msg_length <= MAX_XFER_SIZE; msg_length *= 2) {
    int xfer;

    for (xfer = 0; xfer < NUM_XFERS; xfer++)
      ARMCI_INIT_HANDLE(&handles[xfer]);

    ARMCI_Barrier();
    t_start = MPI_Wtime();

    // Initiate puts, perform NUM_XFERS NB puts to my right neighbor
    for (xfer = 0; xfer < NUM_XFERS; xfer++) {
       ARMCI_NbPut(snd_buf, ((uint8_t*)rcv_buf[(me+1)%nproc])+msg_length*xfer,
           msg_length, (me+1)%nproc, &handles[xfer]);
    }

    // Wait for completion
    for (xfer = 0; xfer < NUM_XFERS; xfer++)
      ARMCI_Wait(&handles[xfer]);

    ARMCI_Barrier();
    t_stop = MPI_Wtime();

    if (me == 0)
      printf("%8d bytes \t %12.8f sec \t %12.8f GB/s\n", 
          msg_length*NUM_XFERS, (t_stop-t_start),
          (msg_length*NUM_XFERS)/(t_stop-t_start)/1.0e9);
  }

  ARMCI_Free(rcv_buf[me]);
  free(rcv_buf);
  free(handles);
  ARMCI_Free_local(snd_buf);

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
