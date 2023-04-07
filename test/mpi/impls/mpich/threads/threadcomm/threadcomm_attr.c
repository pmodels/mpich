/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

int key1, key2;

int copy_fn(MPI_Comm oldcomm, int keyval, void *extra_state,
            void *attribute_val_in, void *attribute_val_out, int *flag)
{
    *flag = 0;
    return MPI_SUCCESS;
}

int delete_fn(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state)
{
    return MPI_SUCCESS;
}

static MTEST_THREAD_RETURN_TYPE test_thread(void *arg)
{
    MPI_Comm comm = *(MPI_Comm *) arg;
    MPIX_Threadcomm_start(comm);

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    MPI_Comm comm_dup;
    MPI_Comm_dup(comm, &comm_dup);
    MPI_Comm_free(&comm_dup);

    int flag = 0;
    MPI_Aint value;
    MPI_Comm_set_attr(comm, key1, (void *) (MPI_Aint) rank);
    MPI_Comm_set_attr(comm, key2, (void *) (MPI_Aint) rank + 100);
    MPI_Comm_get_attr(comm, key1, &value, &flag);
    MTestPrintfMsg(1, "Rank %d retrieved attribute key1 value %ld\n", rank, value);
    MPI_Comm_get_attr(comm, key2, &value, &flag);
    MTestPrintfMsg(1, "Rank %d retrieved attribute key2 value %ld\n", rank, value);
    assert(value == rank + 100);

    MPIX_Threadcomm_finish(comm);
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int nthreads = 4;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    nthreads = MTestArgListGetInt_with_default(head, "nthreads", 4);

    int provided;
    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    assert(provided == MPI_THREAD_MULTIPLE);

    MPI_Comm_create_keyval(copy_fn, delete_fn, &key1, NULL);
    MPI_Comm_create_keyval(copy_fn, delete_fn, &key2, NULL);

    MPI_Comm comm;
    MPIX_Threadcomm_init(MPI_COMM_WORLD, nthreads, &comm);
    for (int i = 0; i < nthreads; i++) {
        MTest_Start_thread(test_thread, &comm);
    }
    MTest_Join_threads();
    MPIX_Threadcomm_free(&comm);

    MPI_Comm_free_keyval(&key1);
    MPI_Comm_free_keyval(&key2);

    MTest_Finalize(errs);
    return errs;
}
