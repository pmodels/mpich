/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* One-Sided MPI 2-D Strided Get Test
 *
 * Author: James Dinan <dinan@mcs.anl.gov> 
 * Date  : December, 2010
 *
 * This code performs N strided get operations from a 2d patch of a shared
 * array.  The array has dimensions [X, Y] and the subarray has dimensions
 * [SUB_X, SUB_Y] and begins at index [0, 0].  The input and output buffers are
 * specified using an MPI indexed type.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define XDIM 8
#define YDIM 1024
#define SUB_XDIM 8
#define SUB_YDIM 256

static int verbose = 0;

int main(int argc, char **argv) {
    int i, j, rank, nranks, peer, bufsize, errors;
    double *win_buf, *loc_buf;
    MPI_Win buf_win;

    MPI_Aint idx_loc[SUB_YDIM];
    int idx_rem[SUB_YDIM];
    int blk_len[SUB_YDIM];
    MPI_Datatype loc_type, rem_type;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &win_buf);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &loc_buf);

    if (rank == 0)
        if (verbose) printf("MPI RMA Strided Get Test:\n");

    for (i = 0; i < XDIM*YDIM; i++)
        *(win_buf + i) = 1.0 + rank;

    MPI_Win_create(win_buf, bufsize, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &buf_win);

    peer = (rank+1) % nranks;

    /* Build the datatype */

    for (i = 0; i < SUB_YDIM; i++) {
      MPI_Get_address(&loc_buf[i*XDIM], &idx_loc[i]);
      idx_rem[i] = i*XDIM;
      blk_len[i] = SUB_XDIM;
    }

    MPI_Type_indexed(SUB_YDIM, blk_len, idx_rem, MPI_DOUBLE, &loc_type);
    MPI_Type_indexed(SUB_YDIM, blk_len, idx_rem, MPI_DOUBLE, &rem_type);

    MPI_Type_commit(&loc_type);
    MPI_Type_commit(&rem_type);

    /* Perform get operation */

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, peer, 0, buf_win);

    MPI_Get(loc_buf, 1, loc_type, peer, 0, 1, rem_type, buf_win);

    /* Use the datatype only on the remote side (must have SUB_XDIM == XDIM) */
    /* MPI_Get(loc_buf, SUB_XDIM*SUB_YDIM, MPI_DOUBLE, peer, 0, 1, rem_type, buf_win); */

    MPI_Win_unlock(peer, buf_win);

    MPI_Type_free(&loc_type);
    MPI_Type_free(&rem_type);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Verify that the results are correct */

    errors = 0;
    for (i = 0; i < SUB_XDIM; i++) {
      for (j = 0; j < SUB_YDIM; j++) {
        const double actual   = *(loc_buf + i + j*XDIM);
        const double expected = (1.0 + peer);
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
        const double actual   = *(loc_buf + i + j*XDIM);
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
        const double actual   = *(loc_buf + i + j*XDIM);
        const double expected = 1.0 + rank;
        if (actual - expected > 1e-10) {
          printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
              rank, j, i, expected, actual);
          errors++;
          fflush(stdout);
        }
      }
    }

    MPI_Win_free(&buf_win);
    MPI_Free_mem(win_buf);
    MPI_Free_mem(loc_buf);

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
