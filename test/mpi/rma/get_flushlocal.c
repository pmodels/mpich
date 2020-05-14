/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Get with Flush_local";
*/

int errs = 0;
const int NITER = 1000;
#define BUFCNT 1024

int main(int argc, char **argv)
{
    int rank, nproc, x, i, target_rank;
    int *winbuf = NULL, *locbuf = NULL;
    MPI_Win win;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    locbuf = malloc(BUFCNT * sizeof(int));

#ifdef USE_WIN_ALLOCATE
    MPI_Win_allocate(BUFCNT * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &winbuf,
                     &win);
#else
    winbuf = malloc(BUFCNT * sizeof(int));
    MPI_Win_create(winbuf, BUFCNT * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
#endif

    /* Initialize window buffer */
    MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
    for (i = 0; i < BUFCNT; i++)
        winbuf[i] = rank * BUFCNT + i;
    MPI_Win_unlock(rank, win);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock_all(0, win);
    for (x = 0; x < NITER; x++) {
        for (target_rank = 0; target_rank < nproc; target_rank++) {
            /* Test local completion with get+flush_local */
            memset(locbuf, 0, BUFCNT * sizeof(int));
            MPI_Get(locbuf, BUFCNT, MPI_INT, target_rank, 0, BUFCNT, MPI_INT, win);
            MPI_Win_flush_local(target_rank, win);

            for (i = 0; i < BUFCNT; i++) {
                int exp_val = target_rank * BUFCNT + i;
                if (locbuf[i] != exp_val) {
                    errs++;
                    printf("[%d] Error: target_rank %d locbuf[%d] got %d, expected %d at iter %d\n",
                           rank, target_rank, i, locbuf[i], exp_val, x);
                    break;
                }
            }
        }
    }

    MPI_Win_unlock_all(win);
    MPI_Win_free(&win);

#ifndef USE_WIN_ALLOCATE
    free(winbuf);
#endif
    free(locbuf);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
