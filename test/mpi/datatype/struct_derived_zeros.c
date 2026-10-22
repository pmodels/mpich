/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Based on code from Jeff Parker at IBM. */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run datatype_struct_derived_zeros
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Datatype mystruct, vecs[3];
    MPI_Aint stride = 5, displs[3];
    int i = 0, blockcount[3];
    int errs = 0;

    for (i = 0; i < 3; i++) {
        MPI_Type_create_hvector(i, 1, stride, MPI_INT, &vecs[i]);
        MPI_Type_commit(&vecs[i]);
        blockcount[i] = 1;
    }
    displs[0] = 0;
    displs[1] = -100;
    displs[2] = -200;   /* irrelevant */

    MPI_Type_create_struct(3, blockcount, displs, vecs, &mystruct);
    MPI_Type_commit(&mystruct);

    MPI_Type_free(&mystruct);
    for (i = 0; i < 3; i++) {
        MPI_Type_free(&vecs[i]);
    }

    /* this time with the first argument always 0 */
    for (i = 0; i < 3; i++) {
        MPI_Type_create_hvector(0, 1, stride, MPI_INT, &vecs[i]);
        MPI_Type_commit(&vecs[i]);
        blockcount[i] = 1;
    }
    displs[0] = 0;
    displs[1] = -100;
    displs[2] = -200;   /* irrelevant */

    MPI_Type_create_struct(3, blockcount, displs, vecs, &mystruct);
    MPI_Type_commit(&mystruct);

    MPI_Type_free(&mystruct);
    for (i = 0; i < 3; i++) {
        MPI_Type_free(&vecs[i]);
    }

    return errs;
}
