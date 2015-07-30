/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    MPI_Datatype newtype;
    int b[1], d[1];

    MPI_Init(0, 0);

    /* create a legitimate type to see that we don't
     * emit spurious errors.
     */
    MPI_Type_hvector(0, 1, 10, MPI_DOUBLE, &newtype);
    MPI_Type_commit(&newtype);
    MPI_Type_free(&newtype);

    MPI_Type_indexed(0, b, d, MPI_DOUBLE, &newtype);
    MPI_Type_commit(&newtype);

    MPI_Sendrecv(b, 1, newtype, 0, 0, d, 0, newtype, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    printf(" No Errors\n");

    MPI_Type_free(&newtype);

    MPI_Finalize();

    return 0;
}
