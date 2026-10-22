/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_nullpscw
int run(const char *arg);
#endif

static int use_win_allocate = 0;

int run(const char *arg)
{
    MPI_Win win;
    MPI_Group group;
    int errs = 0;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    use_win_allocate = MTestArgListGetInt_with_default(head, "use-win-allocate", 0);
    MTestArgListDestroy(head);

    if (use_win_allocate) {
        char *baseptr;
        MPI_Win_allocate(0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &baseptr, &win);
    } else {
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    }
    MPI_Win_get_group(win, &group);

    MPI_Win_post(group, 0, win);
    MPI_Win_start(group, 0, win);

    MPI_Win_complete(win);

    MPI_Win_wait(win);

    MPI_Group_free(&group);
    MPI_Win_free(&win);

    return errs;
}
