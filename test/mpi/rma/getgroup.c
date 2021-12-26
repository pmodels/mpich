/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_getgroup
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test of Win_get_group";
*/

int run(const char *arg)
{
    int errs = 0;
    int result;
    int buf[10];
    MPI_Win win;
    MPI_Group group, wingroup;
    int minsize = 2;
    MPI_Comm comm;

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Win_create(buf, sizeof(int) * 10, sizeof(int), MPI_INFO_NULL, comm, &win);
        MPI_Win_get_group(win, &wingroup);
        MPI_Comm_group(comm, &group);
        MPI_Group_compare(group, wingroup, &result);
        if (result != MPI_IDENT) {
            errs++;
            fprintf(stderr, "Group returned by Win_get_group not the same as the input group\n");
        }
        MPI_Group_free(&wingroup);
        MPI_Group_free(&group);
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    return errs;
}
