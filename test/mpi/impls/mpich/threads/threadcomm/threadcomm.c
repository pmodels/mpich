/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

static MTEST_THREAD_RETURN_TYPE test_thread(void *arg)
{
    MPI_Comm comm = *(MPI_Comm *) arg;
    MPIX_Threadcomm_start(comm);

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    MTestPrintfMsg(1, "Rank %d / %d\n", rank, size);

    /* TODO: add message tests */

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

    MPI_Comm comm;
    MPIX_Threadcomm_init(MPI_COMM_WORLD, nthreads, &comm);
    for (int i = 0; i < nthreads; i++) {
        MTest_Start_thread(test_thread, &comm);
    }
    MTest_Join_threads();
    MPIX_Threadcomm_free(&comm);

    MTest_Finalize(errs);
    return errs;
}
