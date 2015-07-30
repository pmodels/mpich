/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* One-Sided MPI 2-D Strided Accumulate Test
 *
 * Author: James Dinan <dinan@mcs.anl.gov>
 * Date  : November, 2012
 *
 * This code performs N strided put operations followed by get operations into
 * a 2d patch of a shared array.  The array has dimensions [X, Y] and the
 * subarray has dimensions [SUB_X, SUB_Y] and begins at index [0, 0].  The
 * input and output buffers are specified using an MPI indexed type.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include "mpitest.h"
#include "squelch.h"

#define XDIM 8
#define YDIM 1024
#define SUB_XDIM 8
#define SUB_YDIM 255
#define ITERATIONS 10

int main(int argc, char **argv)
{
    int rank, nranks, rank_world, nranks_world;
    int i, j, peer, bufsize, errors;
    double *win_buf, *src_buf, *dst_buf;
    MPI_Win buf_win;
    MPI_Comm shr_comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank_world);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks_world);

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shr_comm);

    MPI_Comm_rank(shr_comm, &rank);
    MPI_Comm_size(shr_comm, &nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &src_buf);
    MPI_Alloc_mem(bufsize, MPI_INFO_NULL, &dst_buf);

    MPI_Win_allocate_shared(bufsize, 1, MPI_INFO_NULL, shr_comm, &win_buf, &buf_win);

    MPI_Win_fence(0, buf_win);

    for (i = 0; i < XDIM * YDIM; i++) {
        *(win_buf + i) = -1.0;
        *(src_buf + i) = 1.0 + rank;
    }

    MPI_Win_fence(0, buf_win);

    peer = (rank + 1) % nranks;

    /* Perform ITERATIONS strided accumulate operations */

    for (i = 0; i < ITERATIONS; i++) {
        int idx_rem[SUB_YDIM];
        int blk_len[SUB_YDIM];
        MPI_Datatype src_type, dst_type;

        for (j = 0; j < SUB_YDIM; j++) {
            idx_rem[j] = j * XDIM;
            blk_len[j] = SUB_XDIM;
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

    MPI_Barrier(shr_comm);

    /* Verify that the results are correct */

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, buf_win);
    errors = 0;
    for (i = 0; i < SUB_XDIM; i++) {
        for (j = 0; j < SUB_YDIM; j++) {
            const double actual = *(win_buf + i + j * XDIM);
            const double expected = (1.0 + ((rank + nranks - 1) % nranks));
            if (fabs(actual - expected) > 1.0e-10) {
                SQUELCH(printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
                               rank, j, i, expected, actual););
                errors++;
                fflush(stdout);
            }
        }
    }
    for (i = SUB_XDIM; i < XDIM; i++) {
        for (j = 0; j < SUB_YDIM; j++) {
            const double actual = *(win_buf + i + j * XDIM);
            const double expected = -1.0;
            if (fabs(actual - expected) > 1.0e-10) {
                SQUELCH(printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
                               rank, j, i, expected, actual););
                errors++;
                fflush(stdout);
            }
        }
    }
    for (i = 0; i < XDIM; i++) {
        for (j = SUB_YDIM; j < YDIM; j++) {
            const double actual = *(win_buf + i + j * XDIM);
            const double expected = -1.0;
            if (fabs(actual - expected) > 1.0e-10) {
                SQUELCH(printf("%d: Data validation failed at [%d, %d] expected=%f actual=%f\n",
                               rank, j, i, expected, actual););
                errors++;
                fflush(stdout);
            }
        }
    }
    MPI_Win_unlock(rank, buf_win);

    MPI_Win_free(&buf_win);
    MPI_Free_mem(src_buf);
    MPI_Free_mem(dst_buf);
    MPI_Comm_free(&shr_comm);

    MTest_Finalize(errors);
    MPI_Finalize();
    return MTestReturnValue(errors);
}
