/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test checks for proper error checking in MPI_Reduce_local, especially
 * handling of MPI_IN_PLACE and buffer aliasing. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpi.h"
#include "mpitest.h"

/* quick cop-out for now */
#define check(c_) assert(c_)

int main(int argc, char *argv[])
{
    int err, errs = 0, len, i, errclass;
    int rank = -1, size = -1;
    int *buf;
    int *recvbuf;
    char msg[MPI_MAX_ERROR_STRING];

    MTest_Init(&argc, &argv);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    buf = malloc(size * sizeof(int));
    recvbuf = malloc(size * sizeof(int));
    for (i = 0; i < size; ++i) {
        buf[i] = i;
        recvbuf[i] = -1;
    }

    /* check valid reduce_local does not fail */
    err = MPI_Reduce_local(buf, recvbuf, size, MPI_INT, MPI_SUM);
    check(err == MPI_SUCCESS);

    /* ERR: check inbuf==MPI_IN_PLACE */
    err = MPI_Reduce_local(MPI_IN_PLACE, recvbuf, size, MPI_INT, MPI_SUM);
    check(err != MPI_SUCCESS);
    MPI_Error_class(err, &errclass);
    check(errclass == MPI_ERR_BUFFER);

    /* ERR: check inoutbuf==MPI_IN_PLACE */
    err = MPI_Reduce_local(buf, MPI_IN_PLACE, size, MPI_INT, MPI_SUM);
    check(err != MPI_SUCCESS);
    MPI_Error_class(err, &errclass);
    check(errclass == MPI_ERR_BUFFER);

    /* ERR: check buffer aliasing is caught */
    err = MPI_Reduce_local(recvbuf, recvbuf, size, MPI_INT, MPI_SUM);
    check(err != MPI_SUCCESS);
    MPI_Error_class(err, &errclass);
    check(errclass == MPI_ERR_BUFFER);

    free(recvbuf);
    free(buf);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
