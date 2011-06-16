/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

/* One-Sided MPI 2-D Strided Accumulate Test
 *
 * Author: James Dinan <dinan@mcs.anl.gov> 
 * Date  : December, 2010
 *
 * This code performs N strided put operations followed by get operations into
 * a 2d patch of a shared array.  The array has dimensions [X, Y] and the
 * subarray has dimensions [SUB_X, SUB_Y] and begins at index [0, 0].  The
 * input and output buffers are specified using an MPI indexed type.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define XDIM 8
#define YDIM 1024
#define SUB_XDIM 8
#define SUB_YDIM 255
#define ITERATIONS 1

int main(int argc, char **argv) {
    int i, j, rank, nranks, peer, bufsize, errors;
    double *win_buf, *src_buf, *dst_buf;
    MPI_Win buf_win;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &win_buf);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &src_buf);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &dst_buf);

    if (rank == 0)
        printf("MPI RMA Strided Accumulate Test:\n");

    for (i = 0; i < XDIM*YDIM; i++) {
        *(win_buf  + i) = 1.0 + rank;
        *(src_buf + i) = 1.0 + rank;
    }

    MPI_Win_create(win_buf, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);

    peer = (rank+1) % nranks;

    // Perform ITERATIONS strided accumulate operations

    for (i = 0; i < ITERATIONS; i++) {
      MPI_Aint idx_loc[SUB_YDIM];
      int idx_rem[SUB_YDIM];
      int blk_len[SUB_YDIM];
      MPI_Datatype src_type, dst_type;

      for (i = 0; i < SUB_YDIM; i++) {
        MPI_Get_address(&src_buf[i*XDIM], &idx_loc[i]);
        idx_rem[i] = i*XDIM;
        blk_len[i] = SUB_XDIM;
      }

      MPI_Type_indexed(SUB_YDIM, blk_len, idx_rem, MPI_DOUBLE, &src_type);
      MPI_Type_indexed(SUB_YDIM, blk_len, idx_rem, MPI_DOUBLE, &dst_type);

      MPI_Type_commit(&src_type);
      MPI_Type_commit(&dst_type);

      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, peer, 0, buf_win);
      MPI_Put(src_buf, 1, src_type, peer, 0, 1, dst_type, buf_win);
      MPI_Win_unlock(peer, buf_win);

      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, peer, 0, buf_win);
      MPI_Get(dst_buf, 1, src_type, peer, 0, 1, dst_type, buf_win);
      MPI_Win_unlock(peer, buf_win);

      MPI_Type_free(&src_type);
      MPI_Type_free(&dst_type);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Verify that the results are correct

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, buf_win);
    errors = 0;
    for (i = 0; i < SUB_XDIM; i++) {
      for (j = 0; j < SUB_YDIM; j++) {
        const double actual   = *(win_buf + i + j*XDIM);
        const double expected = (1.0 + ((rank+nranks-1)%nranks));
        if (actual - expected > 1e-10) {
          printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
              rank, j, i, expected, actual);
          errors++;
          fflush(stdout);
        }
      }
    }
    for (i = SUB_XDIM; i < XDIM; i++) {
      for (j = 0; j < SUB_YDIM; j++) {
        const double actual   = *(win_buf + i + j*XDIM);
        const double expected = 1.0 + rank;
        if (actual - expected > 1e-10) {
          printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
              rank, j, i, expected, actual);
          errors++;
          fflush(stdout);
        }
      }
    }
    for (i = 0; i < XDIM; i++) {
      for (j = SUB_YDIM; j < YDIM; j++) {
        const double actual   = *(win_buf + i + j*XDIM);
        const double expected = 1.0 + rank;
        if (actual - expected > 1e-10) {
          printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
              rank, j, i, expected, actual);
          errors++;
          fflush(stdout);
        }
      }
    }
    MPI_Win_unlock(rank, buf_win);

    MPI_Win_free(&buf_win);
    MPI_Free_mem(win_buf);
    MPI_Free_mem(src_buf);
    MPI_Free_mem(dst_buf);

    MPI_Finalize();

    if (errors == 0) {
      printf("%d: Success\n", rank);
      return 0;
    } else {
      printf("%d: Fail\n", rank);
      return 1;
    }
}
