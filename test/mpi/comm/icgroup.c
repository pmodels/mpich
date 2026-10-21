/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_icgroup
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Get the group of an intercommunicator";
*/

int run(const char *arg)
{
    int errs = 0;
    int rank, size, grank, gsize;
    int minsize = 2, isleft;
    MPI_Comm comm;
    MPI_Group group;

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntercomm(&comm, &isleft, minsize)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        MPI_Comm_group(comm, &group);
        MPI_Group_rank(group, &grank);
        MPI_Group_size(group, &gsize);
        if (rank != grank) {
            errs++;
            fprintf(stderr, "Ranks of groups do not match %d != %d\n", rank, grank);
        }
        if (size != gsize) {
            errs++;
            fprintf(stderr, "Sizes of groups do not match %d != %d\n", size, gsize);
        }
        MPI_Group_free(&group);
        MTestFreeComm(&comm);
    }

    return errs;
}
