/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run rma_get_flushlocal
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Get with Flush_local";
*/

static int errs = 0;
static const int NITER = 1000;
#define BUFCNT 1024

static int use_win_allocate = 0;

int run(const char *arg)
{
    int rank, nproc, x, i, target_rank;
    int *winbuf = NULL, *locbuf = NULL;
    MPI_Win win;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    use_win_allocate = MTestArgListGetInt_with_default(head, "use-win-allocate", 0);
    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    locbuf = malloc(BUFCNT * sizeof(int));

    if (use_win_allocate) {
        MPI_Win_allocate(BUFCNT * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &winbuf,
                         &win);
    } else {
        winbuf = malloc(BUFCNT * sizeof(int));
        MPI_Win_create(winbuf, BUFCNT * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD,
                       &win);
    }

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

    if (!use_win_allocate) {
        free(winbuf);
    }
    free(locbuf);

    return errs;
}
