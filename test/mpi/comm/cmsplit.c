/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_cmsplit
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test comm split";
*/

int run(const char *arg)
{
    int errs = 0;
    int rank, size, color, srank;
    MPI_Comm comm, scomm;

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (size < 4) {
        fprintf(stderr, "This test requires at least four processes.");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    color = MPI_UNDEFINED;
    if (rank < 2)
        color = 1;
    MPI_Comm_split(comm, color, size - rank, &scomm);

    if (rank < 2) {
        /* Check that the ranks are ordered correctly */
        MPI_Comm_rank(scomm, &srank);
        if (srank != 1 - rank) {
            errs++;
        }
        MPI_Comm_free(&scomm);
    } else {
        if (scomm != MPI_COMM_NULL) {
            errs++;
        }
    }
    MPI_Comm_free(&comm);
    return errs;
}
