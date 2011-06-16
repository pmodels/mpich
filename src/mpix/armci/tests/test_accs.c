/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define XDIM 1024 
#define YDIM 1024
#define ITERATIONS 10

int main(int argc, char **argv) {
    int i, j, rank, nranks, peer, bufsize, errors, total_errors;
    double **buffer, *src_buf;
    int count[2], src_stride, trg_stride, stride_level;
    double scaling, time;

    MPI_Init(&argc, &argv);
    ARMCI_Init();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    buffer = (double **) malloc(sizeof(double *) * nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    ARMCI_Malloc((void **) buffer, bufsize);
    src_buf = ARMCI_Malloc_local(bufsize);

    if (rank == 0)
        printf("ARMCI Strided Accumulate Test:\n");

    ARMCI_Access_begin(buffer[rank]);

    for (i = 0; i < XDIM*YDIM; i++) {
        *(buffer[rank] + i) = 1.0 + rank;
        *(src_buf + i) = 1.0 + rank;
    }

    ARMCI_Access_end(buffer[rank]);

    scaling = 2.0;

    src_stride = XDIM * sizeof(double);
    trg_stride = XDIM * sizeof(double);
    stride_level = 1;

    count[1] = YDIM;
    count[0] = XDIM * sizeof(double);

    ARMCI_Barrier();
    time = MPI_Wtime();

    peer = (rank+1) % nranks;

    for (i = 0; i < ITERATIONS; i++) {

      ARMCI_AccS(ARMCI_ACC_DBL,
          (void *) &scaling,
          src_buf,
          &src_stride,
          (void *) buffer[peer],
          &trg_stride,
          count,
          stride_level,
          peer);
    }

    ARMCI_Barrier();
    time = MPI_Wtime() - time;

    if (rank == 0) printf("Time: %f sec\n", time);

    ARMCI_Access_begin(buffer[rank]);
    for (i = errors = 0; i < XDIM; i++) {
      for (j = 0; j < YDIM; j++) {
        const double actual   = *(buffer[rank] + i + j*XDIM);
        const double expected = (1.0 + rank) + scaling * (1.0 + ((rank+nranks-1)%nranks)) * (ITERATIONS);
        if (actual - expected > 1e-10) {
          printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
              rank, j, i, expected, actual);
          errors++;
          fflush(stdout);
        }
      }
    }
    ARMCI_Access_end(buffer[rank]);

    MPI_Allreduce(&errors, &total_errors, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    ARMCI_Free((void *) buffer[rank]);
    ARMCI_Free_local(src_buf);

    ARMCI_Finalize();
    MPI_Finalize();

    if (total_errors == 0) {
      if (rank == 0) printf("Success.\n");
      return 0;
    } else {
      if (rank == 0) printf("Fail.\n");
      return 1;
    }
}
