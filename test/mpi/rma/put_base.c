/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* One-Sided MPI 2-D Strided Put Test
 *
 * Author: James Dinan <dinan@mcs.anl.gov> 
 * Date  : March, 2011
 *
 * This code performs N strided put operations into a 2d patch of a shared
 * array.  The array has dimensions [X, Y] and the subarray has dimensions
 * [SUB_X, SUB_Y] and begins at index [0, 0].  The input and output buffers are
 * specified using an MPI datatype.
 *
 * This test generates a datatype that is relative to an arbitrary base address
 * in memory and tests the RMA implementation's ability to perform the correct
 * transfer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mpi.h>

#define XDIM 1024
#define YDIM 1024
#define SUB_XDIM 1024
#define SUB_YDIM 1024
#define ITERATIONS 10

static int verbose = 0;

int main(int argc, char **argv) {
    int i, j, rank, nranks, peer, bufsize, errors;
    double  *win_buf, *src_buf, *dst_buf;
    MPI_Win buf_win;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &win_buf);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &src_buf);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &dst_buf);

    if (rank == 0)
        if (verbose) printf("MPI RMA Strided Put Test:\n");

    for (i = 0; i < XDIM*YDIM; i++) {
        *(win_buf  + i) = 1.0 + rank;
        *(src_buf + i) = 1.0 + rank;
    }

    MPI_Win_create(win_buf, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);

    peer = (rank+1) % nranks;

    /* Perform ITERATIONS strided put operations */

    for (i = 0; i < ITERATIONS; i++) {
      MPI_Aint idx_loc[SUB_YDIM];
      int idx_rem[SUB_YDIM];
      int blk_len[SUB_YDIM];
      MPI_Datatype src_type, dst_type;

      void *base_ptr = dst_buf;
      MPI_Aint base_int;

      MPI_Get_address(base_ptr, &base_int);

      if (rank == 0)
        if (verbose) printf(" + iteration %d\n", i);

      for (j = 0; j < SUB_YDIM; j++) {
        MPI_Get_address(&src_buf[j*XDIM], &idx_loc[j]);
        idx_loc[j] = idx_loc[j] - base_int;
        idx_rem[j] = j*XDIM*sizeof(double);
        blk_len[j] = SUB_XDIM*sizeof(double);
      }

      MPI_Type_create_hindexed(SUB_YDIM, blk_len, idx_loc, MPI_BYTE, &src_type);
      MPI_Type_create_indexed_block(SUB_YDIM, SUB_XDIM*sizeof(double), idx_rem, MPI_BYTE, &dst_type);

      MPI_Type_commit(&src_type);
      MPI_Type_commit(&dst_type);

      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, peer, 0, buf_win);
      MPI_Put(base_ptr, 1, src_type, peer, 0, 1, dst_type, buf_win);
      MPI_Win_unlock(peer, buf_win);

      MPI_Type_free(&src_type);
      MPI_Type_free(&dst_type);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Verify that the results are correct */

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
      if (rank == 0)
        printf(" No Errors\n");
      return 0;
    } else {
      printf("%d: Fail\n", rank);
      return 1;
    }
}
