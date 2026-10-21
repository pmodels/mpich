/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_zeroparms
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Datatype newtype;
    int b[1], d[1];

    /* create a legitimate type to see that we don't
     * emit spurious errors.
     */
    MPI_Type_create_hvector(0, 1, 10, MPI_DOUBLE, &newtype);
    MPI_Type_commit(&newtype);
    MPI_Type_free(&newtype);

    MPI_Type_indexed(0, b, d, MPI_DOUBLE, &newtype);
    MPI_Type_commit(&newtype);

    MPI_Sendrecv(b, 1, newtype, 0, 0, d, 0, newtype, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Type_free(&newtype);

    return 0;
}
