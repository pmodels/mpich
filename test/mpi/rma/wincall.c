/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_wincall
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test win_call_errhandler";
*/

static int calls = 0;
static int errs = 0;
static MPI_Win mywin;

static void eh(MPI_Win * win, int *err, ...)
{
    if (*err != MPI_ERR_OTHER) {
        errs++;
        printf("Unexpected error code\n");
    }
    if (*win != mywin) {
        errs++;
        printf("Unexpected window\n");
    }
    calls++;
    return;
}

int run(const char *arg)
{
    int buf[2];
    MPI_Win win;
    MPI_Errhandler newerr;
    int i;

    /* Run this test multiple times to expose storage leaks (we found a leak
     * of error handlers with this test) */
    for (i = 0; i < 1000; i++) {
        calls = 0;

        MPI_Win_create(buf, 2 * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        mywin = win;

        MPI_Win_create_errhandler(eh, &newerr);

        MPI_Win_set_errhandler(win, newerr);
        MPI_Win_call_errhandler(win, MPI_ERR_OTHER);
        MPI_Errhandler_free(&newerr);
        if (calls != 1) {
            errs++;
            printf("Error handler not called\n");
        }
        MPI_Win_free(&win);
    }

    return errs;
}
