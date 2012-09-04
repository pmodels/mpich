/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define XDIM 5
#define YDIM 98
#define ITERATIONS 10

int main(int argc, char **argv) {
    int i, j, rank, nranks, peer, bufsize, errors;
    double *src_buf, *dst_buf;
    double **shr_bvec, **src_bvec, **dst_bvec;
    int count[2], src_stride, trg_stride, stride_level;

    MPI_Init(&argc, &argv);
    ARMCI_Init();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    shr_bvec = (double **) malloc(sizeof(double *) * nranks);
    src_bvec = (double **) malloc(sizeof(double *) * nranks);
    dst_bvec = (double **) malloc(sizeof(double *) * nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    ARMCI_Malloc((void **) shr_bvec, bufsize);
    ARMCI_Malloc((void **) src_bvec, bufsize);
    ARMCI_Malloc((void **) dst_bvec, bufsize);

    src_buf = src_bvec[rank];
    dst_buf = dst_bvec[rank];

    if (rank == 0)
        printf("ARMCI Strided DLA Put Test:\n");

    src_stride = XDIM * sizeof(double);
    trg_stride = XDIM * sizeof(double);
    stride_level = 1;

    count[1] = YDIM;
    count[0] = XDIM * sizeof(double);

    ARMCI_Barrier();

    peer = (rank+1) % nranks;

    for (i = 0; i < ITERATIONS; i++) {

      ARMCI_Access_begin(src_buf);
      for (j = 0; j < XDIM*YDIM; j++) {
        *(src_buf + j) = rank + i;
      }
      ARMCI_Access_end(src_buf);

      ARMCI_PutS(
          src_buf,
          &src_stride,
          (void *) shr_bvec[peer],
          &trg_stride,
          count,
          stride_level,
          peer);

      ARMCI_GetS(
          (void *) shr_bvec[peer],
          &trg_stride,
          dst_buf,
          &src_stride,
          count,
          stride_level,
          peer);
    }

    ARMCI_Barrier();

    ARMCI_Access_begin(shr_bvec[rank]);
    for (i = errors = 0; i < XDIM; i++) {
      for (j = 0; j < YDIM; j++) {
        const double actual   = *(shr_bvec[rank] + i + j*XDIM);
        const double expected = (1.0 + rank) + (1.0 + ((rank+nranks-1)%nranks)) + (ITERATIONS);
        if (actual - expected > 1e-10) {
          printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
              rank, j, i, expected, actual);
          errors++;
          fflush(stdout);
        }
      }
    }
    ARMCI_Access_end(shr_bvec[rank]);

    ARMCI_Free((void *) shr_bvec[rank]);
    ARMCI_Free((void *) src_bvec[rank]);
    ARMCI_Free((void *) dst_bvec[rank]);

    free(shr_bvec);
    free(src_bvec);
    free(dst_bvec);

    ARMCI_Finalize();
    MPI_Finalize();

    if (errors == 0) {
      printf("%d: Success\n", rank);
      return 0;
    } else {
      printf("%d: Fail\n", rank);
      return 1;
    }
}
