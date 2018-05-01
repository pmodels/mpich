/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"

#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Datatype newtype;
    int b[1], d[1];

    MTest_Init(&argc, &argv);

    /* create a legitimate type to see that we don't
     * emit spurious errors.
     */
    MPI_Type_hvector(0, 1, 10, MPI_DOUBLE, &newtype);
    MPI_Type_commit(&newtype);
    MPI_Type_free(&newtype);

    MPI_Type_indexed(0, b, d, MPI_DOUBLE, &newtype);
    MPI_Type_commit(&newtype);

    MPI_Sendrecv(b, 1, newtype, 0, 0, d, 0, newtype, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Type_free(&newtype);

    MTest_Finalize(0);

    return 0;
}
