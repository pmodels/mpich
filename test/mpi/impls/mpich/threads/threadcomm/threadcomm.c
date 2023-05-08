/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

static int do_restart = 0;
static int do_commdup = 0;

static MTEST_THREAD_RETURN_TYPE test_thread(void *arg)
{
    MPI_Comm comm = *(MPI_Comm *) arg;

    for (int rep = 0; rep < 1 + do_restart; rep++) {
        MPIX_Threadcomm_start(comm);

        MPI_Comm use_comm = comm;
        if (do_commdup) {
            MPI_Comm_dup(comm, &use_comm);
        }

        int rank, size;
        MPI_Comm_size(use_comm, &size);
        MPI_Comm_rank(use_comm, &rank);
        MTestPrintfMsg(1, "Rank %d / %d\n", rank, size);

        /* TODO: add message tests */

        if (do_commdup) {
            MPI_Comm_free(&use_comm);
        }
        MPIX_Threadcomm_finish(comm);
    }
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int nthreads = 4;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    nthreads = MTestArgListGetInt_with_default(head, "nthreads", 4);
    do_restart = MTestArgListGetInt_with_default(head, "restart", 0);
    do_commdup = MTestArgListGetInt_with_default(head, "commdup", 0);
    MTestArgListDestroy(head);

    MTest_Init(NULL, NULL);

    MTestPrintfMsg(1, "Test Threadcomm: nthreads=%d, restart=%d, commdup=%d\n",
                   nthreads, do_restart, do_commdup);

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
