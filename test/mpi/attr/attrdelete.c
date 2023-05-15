/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
  Having attr delete function to delete another attribute.
 */
#include "mpitest.h"
#include <stdio.h>

int key1, key2, key3;

int test_communicator(MPI_Comm comm);

int main(int argc, char **argv)
{
    int errs;
    MTest_Init(&argc, &argv);
    errs = test_communicator(MPI_COMM_WORLD);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

static int key2_delete_fn(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state)
{
    MPI_Comm_delete_attr(comm, key1);
    MPI_Comm_delete_attr(comm, key3);
    return MPI_SUCCESS;
}

int test_communicator(MPI_Comm comm)
{
    int errs = 0;
    int rank, size;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    MPI_Comm_create_keyval(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &key1, NULL);
    MPI_Comm_create_keyval(MPI_NULL_COPY_FN, key2_delete_fn, &key2, NULL);
    MPI_Comm_create_keyval(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN, &key3, NULL);

    MPI_Comm_set_attr(comm, key1, (void *) (MPI_Aint) rank);
    MPI_Comm_set_attr(comm, key2, (void *) (MPI_Aint) (rank + 100));
    MPI_Comm_set_attr(comm, key3, (void *) (MPI_Aint) (rank + 200));

    MPI_Comm_delete_attr(comm, key2);

    MPI_Comm_free_keyval(&key1);
    MPI_Comm_free_keyval(&key2);
    MPI_Comm_free_keyval(&key3);

    return errs;
}
