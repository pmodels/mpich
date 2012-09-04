/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <mpi.h>
#include <armci.h>

#define MAX_SIZE   262144
#define NUM_ROUNDS 1000

int main(int argc, char **argv) {
  int        me, nproc, zero, target;
  int        msg_length, round, i;
  double     t_start, t_stop;
  uint8_t  *snd_buf;  // Send buffer (byte array)
  uint8_t **rcv_buf;  // Receive buffer (byte array)

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (nproc < 2)
    ARMCI_Error("This benchmark should be run on at least two processes", 1);

  if (me == 0)
    printf("ARMCI ping-pong latency test, performing %d rounds at each xfer size.\n", NUM_ROUNDS);

  rcv_buf = malloc(nproc*sizeof(void*));

  ARMCI_Malloc((void*)rcv_buf, MAX_SIZE);
  snd_buf = ARMCI_Malloc_local(MAX_SIZE);

  zero = 0;

  for (i = 0; i < MAX_SIZE; i++) {
    snd_buf[i] = 1;
  }

  for (target = 1; target < nproc; target++) {
    if (me == 0) printf("\n========== Process pair: %d and %d ==========\n\n", 0, target);

    for (msg_length = 1; msg_length <= MAX_SIZE; msg_length *= 2) {
      ARMCI_Barrier();
      t_start = MPI_Wtime();

      if (me == 0 || me == target) {

        // Perform NUM_ROUNDS ping-pongs
        for (round = 0; round < NUM_ROUNDS*2; round++) {
          int my_target = me == 0 ? target : 0;

          // I am the sender
          if (round % 2 == me) {
            if ((round % 2 == 0 && me == 0) || (round % 2 != 0 && me != 0)) {
              // Clear start and end markers for next round
#ifdef DIRECT_ACCESS
              ((uint8_t*)rcv_buf[me])[0] = 0;
              ((uint8_t*)rcv_buf[me])[msg_length-1] = 0;
#else
              ARMCI_Put(&zero, &(((uint8_t*)rcv_buf[me])[0]),            1, me);
              ARMCI_Put(&zero, &(((uint8_t*)rcv_buf[me])[msg_length-1]), 1, me);
#endif

              ARMCI_Put(snd_buf, rcv_buf[my_target], msg_length, my_target);
              ARMCI_Fence(my_target); // This is optional, we don't need notification
            }

            // I am the receiver
            else {
#ifdef DIRECT_ACCESS
              while (((volatile uint8_t*)rcv_buf[me])[0] == 0) ;
              while (((volatile uint8_t*)rcv_buf[me])[msg_length-1] == 0) ;
#else
              uint8_t val;

              do {
                ARMCI_Get(&(((uint8_t*)rcv_buf[me])[0]), &val, 1, me);
              } while (val == 0);

              do {
                ARMCI_Get(&(((uint8_t*)rcv_buf[me])[msg_length-1]), &val, 1, me);
              } while (val == 0);
#endif
            }
          }
        }
      }

      ARMCI_Barrier(); // FIXME: Time here increases with nproc :(
      t_stop = MPI_Wtime();

      if (me == 0)
        printf("%8d bytes \t %12.8f us\n", msg_length, (t_stop-t_start)/NUM_ROUNDS*1.0e6);
    }

    ARMCI_Barrier();
  }

  ARMCI_Free(rcv_buf[me]);
  free(rcv_buf);
  ARMCI_Free_local(snd_buf);

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}
