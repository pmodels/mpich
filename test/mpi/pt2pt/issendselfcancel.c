/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpi.h"

int main(int argc, char **argv)
{
    MPI_Request req;
    MPI_Status status;

    MPI_Init(NULL, NULL);

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

    printf(" No Errors\n");

    MPI_Finalize();
}
