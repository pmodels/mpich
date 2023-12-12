/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
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
 */

#include "mpitest.h"
#include <memory.h>

#ifdef MULTI_TESTS
#define run rma_rget_unlock
int run(const char *arg);
#endif

#define N_ELMS (128)
#define BLOCKSIZE (1)
#define N_BLOCKS (N_ELMS/BLOCKSIZE)

int run(const char *arg)
{
    MPI_Win win;
    int i;
    int *rbuf, *lbuf;
    int rank, size, trg;
    MPI_Request *reqs;
    int errs = 0;

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

    return errs;
}
