/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

static int foo_keyval = MPI_KEYVAL_INVALID;

int foo_initialize(void);
void foo_finalize(void);

int foo_copy_attr_function(MPI_Datatype type, int type_keyval,
                           void *extra_state, void *attribute_val_in,
                           void *attribute_val_out, int *flag);
int foo_delete_attr_function(MPI_Datatype type, int type_keyval,
                             void *attribute_val, void *extra_state);
static const char *my_func = 0;
static int verbose = 0;
static int delete_called = 0;
static int copy_called = 0;

int main(int argc, char *argv[])
{
    MPI_Datatype type, duptype;
    int rank;
    int errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    foo_initialize();

    MPI_Type_contiguous(2, MPI_INT, &type);

    MPI_Type_set_attr(type, foo_keyval, NULL);

    MPI_Type_dup(type, &duptype);

    my_func = "Free of type";
    MPI_Type_free(&type);

    my_func = "free of duptype";
    MPI_Type_free(&duptype);

    foo_finalize();

    if (rank == 0) {
        if (copy_called != 1) {
            printf("Copy called %d times; expected once\n", copy_called);
            errs++;
        }
        if (delete_called != 2) {
            printf("Delete called %d times; expected twice\n", delete_called);
            errs++;
        }
        fflush(stdout);
    }

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}

int foo_copy_attr_function(MPI_Datatype type,
                           int type_keyval,
                           void *extra_state,
                           void *attribute_val_in, void *attribute_val_out, int *flag)
{
    if (verbose)
        printf("copy fn. called\n");
    copy_called++;
    *(char **) attribute_val_out = NULL;
    *flag = 1;

    return MPI_SUCCESS;
}

int foo_delete_attr_function(MPI_Datatype type,
                             int type_keyval, void *attribute_val, void *extra_state)
{
    if (verbose)
        printf("delete fn. called in %s\n", my_func);
    delete_called++;

    return MPI_SUCCESS;
}

int foo_initialize(void)
{
    /* create keyval for use later */
    MPI_Type_create_keyval(foo_copy_attr_function, foo_delete_attr_function, &foo_keyval, NULL);
    if (verbose)
        printf("created keyval\n");

    return 0;
}

void foo_finalize(void)
{
    /* remove keyval */
    MPI_Type_free_keyval(&foo_keyval);

    if (verbose)
        printf("freed keyval\n");

    return;
}
