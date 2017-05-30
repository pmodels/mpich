/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 *
 *
 *  This is a test case to make sure synchronization in unlock works correctly.
 *
 *  Essentially this program does the following:
 *
 *  lock_all
 *  req=rget(buf)
 *  unlock_all
 *  re-use buf
 *  wait(req)
 *
 *  This program is valid but if unlock_all does not implement the synchronization
 *  semantics correctly reusing the buffer would race with outstanding rgets.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <mpi.h>
#include "mpitest.h"

#define N_ELMS (128)
#define BLOCKSIZE (1)
#define N_BLOCKS (N_ELMS/BLOCKSIZE)

int main(int argc, char *argv[])
{
    MPI_Win win;
    int i;
    int *rbuf, *lbuf;
    int rank, size, trg;
    MPI_Request *reqs;
    int errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    trg = (rank + 1) % size;

    rbuf = malloc(sizeof(int) * N_ELMS);
    for (i = 0; i < N_ELMS; i++)
        rbuf[i] = rank;

    lbuf = malloc(sizeof(int) * N_ELMS);
    memset(lbuf, -1, sizeof(int) * N_ELMS);

    reqs = malloc(sizeof(MPI_Request) * N_BLOCKS);

    MPI_Win_create(rbuf, sizeof(int) * N_ELMS, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_lock_all(MPI_MODE_NOCHECK, win);
    for (i = 0; i < N_BLOCKS; i++)
        MPI_Rget(lbuf + i * BLOCKSIZE, BLOCKSIZE, MPI_INT, trg, i * BLOCKSIZE, BLOCKSIZE, MPI_INT,
                 win, &reqs[i]);
    MPI_Win_unlock_all(win);
    for (i = 0; i < N_ELMS; i++)
        lbuf[i] = -2;

    MPI_Waitall(N_BLOCKS, reqs, MPI_STATUSES_IGNORE);
    for (i = 0; i < N_ELMS; i++) {
        int v = lbuf[i];
        if (v != -2) {
            printf("lbuf[%d]=%d, expected -2\n", i, v);
            errs++;
        }
    }
    MPI_Win_free(&win);

    free(reqs);
    free(lbuf);
    free(rbuf);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
