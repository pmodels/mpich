/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include "mpitest.h"

/* tests multiple invocations of MPI_Win_free_keyval on the same keyval */

int delete_fn(MPI_Comm comm, int keyval, void *attr, void *extra);
int delete_fn(MPI_Comm comm, int keyval, void *attr, void *extra)
{
    MPI_Win_free_keyval(&keyval);
    return MPI_SUCCESS;
}

int main(int argc, char **argv)
{
    void *base_ptr = NULL;
    MPI_Win window;
    int keyval = MPI_KEYVAL_INVALID;
    int keyval_copy = MPI_KEYVAL_INVALID;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Alloc_mem(sizeof(int), MPI_INFO_NULL, &base_ptr);
    MPI_Win_create(base_ptr, sizeof(int), 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window);

    MPI_Win_create_keyval(MPI_NULL_COPY_FN, delete_fn, &keyval, NULL);
    keyval_copy = keyval;
    MPI_Win_set_attr(window, keyval, NULL);

    MPI_Win_free(&window);      /* first MPI_Win_free_keyval */
    MPI_Free_mem(base_ptr);
    MPI_Win_free_keyval(&keyval);   /* second MPI_Win_free_keyval */
    MPI_Win_free_keyval(&keyval_copy);      /* third MPI_Win_free_keyval */
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}

