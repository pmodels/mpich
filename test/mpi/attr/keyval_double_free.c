/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include "mpitest.h"

/* tests multiple invocations of Keyval_free on the same keyval */

int delete_fn(MPI_Comm comm, int keyval, void *attr, void *extra);
int delete_fn(MPI_Comm comm, int keyval, void *attr, void *extra)
{
    MPI_Keyval_free(&keyval);
    return MPI_SUCCESS;
}

int main(int argc, char **argv)
{
    MPI_Comm duped;
    int keyval = MPI_KEYVAL_INVALID;
    int keyval_copy = MPI_KEYVAL_INVALID;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_dup(MPI_COMM_SELF, &duped);

    MPI_Keyval_create(MPI_NULL_COPY_FN, delete_fn, &keyval, NULL);
    keyval_copy = keyval;

    MPI_Attr_put(MPI_COMM_SELF, keyval, NULL);
    MPI_Attr_put(duped, keyval, NULL);

    MPI_Comm_free(&duped);      /* first MPI_Keyval_free */
    MPI_Keyval_free(&keyval);   /* second MPI_Keyval_free */
    MPI_Keyval_free(&keyval_copy);      /* third MPI_Keyval_free */
    MTest_Finalize(errs);
    MPI_Finalize();     /* fourth MPI_Keyval_free */
    return 0;
}
