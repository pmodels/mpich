/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run attr_keyval_double_free_comm
int run(const char *arg);
#endif

/* tests multiple invocations of MPI_Comm_free_keyval on the same keyval */

static int delete_fn(MPI_Comm comm, int keyval, void *attr, void *extra)
{
    MPI_Comm_free_keyval(&keyval);
    return MPI_SUCCESS;
}

int run(const char *arg)
{
    MPI_Comm duped;
    int keyval = MPI_KEYVAL_INVALID;
    int keyval_copy = MPI_KEYVAL_INVALID;
    int errs = 0;

    MPI_Comm_dup(MPI_COMM_SELF, &duped);

    MPI_Comm_create_keyval(MPI_NULL_COPY_FN, delete_fn, &keyval, NULL);
    keyval_copy = keyval;

    MPI_Comm_set_attr(MPI_COMM_SELF, keyval, NULL);
    MPI_Comm_set_attr(duped, keyval, NULL);

    MPI_Comm_free(&duped);      /* first MPI_Comm_free_keyval */
    MPI_Comm_free_keyval(&keyval);      /* second MPI_Comm_free_keyval */
    MPI_Comm_free_keyval(&keyval_copy); /* third MPI_Comm_free_keyval */
    return errs;
}
