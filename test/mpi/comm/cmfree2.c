/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_cmfree2
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test that communicators have reference count semantics";
*/

int run(const char *arg)
{
    int errs = 0;
    int rank, size;
    MPI_Comm comm;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        fprintf(stderr, "This test requires at least two processes.");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    if (rank == 0) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Ssend(NULL, 0, MPI_INT, 1, 0, comm);
        MPI_Comm_free(&comm);
    } else if (rank == 1) {
        MPI_Request req;
        /* recv an ssend after the user frees the comm */
        MPI_Irecv(NULL, 0, MPI_INT, 0, 0, comm, &req);
        MPI_Comm_free(&comm);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        MPI_Comm_free(&comm);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    return errs;
}
