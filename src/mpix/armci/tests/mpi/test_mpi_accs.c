/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define XDIM 1024 
#define YDIM 1024
#define ITERATIONS 10

int main(int argc, char **argv) {
    int itr, i, j, rank, nranks, peer, bufsize, errors;
    double *buffer, *src_buf;
    MPI_Win buf_win;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &buffer);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &src_buf);

    if (rank == 0)
        printf("MPI RMA Strided Accumulate Test:\n");

    for (i = 0; i < XDIM*YDIM; i++) {
        *(buffer  + i) = 1.0 + rank;
        *(src_buf + i) = 1.0 + rank;
    }

    MPI_Win_create(buffer, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);

    peer = 0; //(rank+1) % nranks;

    for (itr = 0; itr < ITERATIONS; itr++) {

      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, peer, 0, buf_win);

      for (j = 0; j < YDIM; j++) {
        MPI_Accumulate(src_buf + j*XDIM, XDIM, MPI_DOUBLE, peer,
                       j*XDIM*sizeof(double), XDIM, MPI_DOUBLE, MPI_SUM, buf_win);
      }

      MPI_Win_unlock(peer, buf_win);
    }

    MPI_Barrier(MPI_COMM_WORLD);
#if 0
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, buf_win);
    for (i = errors = 0; i < XDIM; i++) {
      for (j = 0; j < YDIM; j++) {
        const double actual   = *(buffer + i + j*XDIM);
        const double expected = (1.0 + rank) + (1.0 + ((rank+nranks-1)%nranks)) * (ITERATIONS);
        if (actual - expected > 1e-10) {
          printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
              rank, j, i, expected, actual);
          errors++;
          fflush(stdout);
        }
      }
    }
    MPI_Win_unlock(rank, buf_win);
#endif

    MPI_Win_free(&buf_win);
    MPI_Free_mem(buffer);
    MPI_Free_mem(src_buf);

    MPI_Finalize();

    if (errors == 0) {
      printf("%d: Success\n", rank);
      return 0;
    } else {
      printf("%d: Fail\n", rank);
      return 1;
    }
}
