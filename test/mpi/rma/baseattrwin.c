/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_baseattrwin
int run(const char *arg);
#endif

int run(const char *arg)
{
    int errs = 0;
    void *v;
    int flag;
    int rank, size;
    int base[1024];
    MPI_Aint n;
    int disp;
    MPI_Win win;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Create a window; then extract the values */
    n = 1024;
    disp = 4;
    MPI_Win_create(base, n, disp, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_get_attr(win, MPI_WIN_BASE, &v, &flag);
    if (!flag) {
        errs++;
        fprintf(stderr, "Could not get WIN_BASE\n");
    } else {
        /* MPI 2.1, section 11.2.2.  v must be a pointer to the start of the
         * window.  It is not a pointer to a pointer to the start of the window.
         */
        if ((int *) v != base) {
            errs++;
            fprintf(stderr, "Got incorrect value for WIN_BASE (%p, should be %p)", v, base);
        }
    }

    MPI_Win_get_attr(win, MPI_WIN_SIZE, &v, &flag);
    if (!flag) {
        errs++;
        fprintf(stderr, "Could not get WIN_SIZE\n");
    } else {
        MPI_Aint vval = *(MPI_Aint *) v;
        if (vval != n) {
            errs++;
            fprintf(stderr, "Got wrong value for WIN_SIZE (%ld, should be %ld)\n",
                    (long) vval, (long) n);
        }
    }

    MPI_Win_get_attr(win, MPI_WIN_DISP_UNIT, &v, &flag);
    if (!flag) {
        errs++;
        fprintf(stderr, "Could not get WIN_DISP_UNIT\n");
    } else {
        int vval = *(int *) v;
        if (vval != disp) {
            errs++;
            fprintf(stderr, "Got wrong value for WIN_DISP_UNIT (%d, should be %d)\n", vval, disp);
        }
    }

    MPI_Win_free(&win);

    return errs;
}
