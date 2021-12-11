/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run attr_keyval_double_free_type
int run(const char *arg);
#endif

/* tests multiple invocations of MPI_Type_free_keyval on the same keyval */

static int delete_fn(MPI_Datatype datatype, int keyval, void *attr, void *extra)
{
    MPI_Type_free_keyval(&keyval);
    return MPI_SUCCESS;
}

int run(const char *arg)
{
    MPI_Datatype type;
    MPI_Datatype type_dup;
    int keyval = MPI_KEYVAL_INVALID;
    int keyval_copy = MPI_KEYVAL_INVALID;
    int errs = 0;

    MPI_Type_dup(MPI_INT, &type);
    MPI_Type_dup(MPI_INT, &type_dup);

    MPI_Type_create_keyval(MPI_TYPE_NULL_COPY_FN, delete_fn, &keyval, NULL);
    keyval_copy = keyval;
    MPI_Type_set_attr(type, keyval, NULL);
    MPI_Type_set_attr(type_dup, keyval, NULL);

    MPI_Type_free(&type);       /* first MPI_Type_free_keyval */
    MPI_Type_free_keyval(&keyval);      /* second MPI_Type_free_keyval */
    MPI_Type_free_keyval(&keyval_copy); /* third MPI_Type_free_keyval */
    MPI_Type_free(&type_dup);   /* fourth MPI_Type_free_keyval */
    return errs;
}
