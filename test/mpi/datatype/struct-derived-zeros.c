/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Based on code from Jeff Parker at IBM. */

#include <stdio.h>

#include <mpi.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Datatype mystruct, vecs[3];
    MPI_Aint stride = 5, displs[3];
    int i = 0, blockcount[3];
    int errs = 0;

    MTest_Init(&argc, &argv);

    for (i = 0; i < 3; i++) {
        MPI_Type_hvector(i, 1, stride, MPI_INT, &vecs[i]);
        MPI_Type_commit(&vecs[i]);
        blockcount[i] = 1;
    }
    displs[0] = 0;
    displs[1] = -100;
    displs[2] = -200;   /* irrelevant */

    MPI_Type_struct(3, blockcount, displs, vecs, &mystruct);
    MPI_Type_commit(&mystruct);

    MPI_Type_free(&mystruct);
    for (i = 0; i < 3; i++) {
        MPI_Type_free(&vecs[i]);
    }

    /* this time with the first argument always 0 */
    for (i = 0; i < 3; i++) {
        MPI_Type_hvector(0, 1, stride, MPI_INT, &vecs[i]);
        MPI_Type_commit(&vecs[i]);
        blockcount[i] = 1;
    }
    displs[0] = 0;
    displs[1] = -100;
    displs[2] = -200;   /* irrelevant */

    MPI_Type_struct(3, blockcount, displs, vecs, &mystruct);
    MPI_Type_commit(&mystruct);

    MPI_Type_free(&mystruct);
    for (i = 0; i < 3; i++) {
        MPI_Type_free(&vecs[i]);
    }

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
