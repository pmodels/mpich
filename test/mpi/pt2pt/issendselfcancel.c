/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run pt2pt_issendselfcancel
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Request req;
    MPI_Status status;

    MPI_Issend(NULL, 0, MPI_BYTE, 0, 123, MPI_COMM_SELF, &req);

    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_SELF, &status);
    assert(status.MPI_SOURCE == 0);
    assert(status.MPI_TAG == 123);

    MPI_Cancel(&req);
    assert(req != MPI_REQUEST_NULL);

    MPI_Request_free(&req);

    MPI_Irecv(NULL, 0, MPI_BYTE, 0, 123, MPI_COMM_SELF, &req);
    MPI_Cancel(&req);
    MPI_Wait(&req, &status);

    return 0;
}
