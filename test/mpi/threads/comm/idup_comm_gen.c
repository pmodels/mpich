/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test tries to overlap multiple Comm_idups with other communicator
  creation functions, either intracomm or intercomm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS 4
#define NUM_IDUPS   5

MPI_Comm comms[NUM_THREADS];
MPI_Comm errs[NUM_THREADS] = { 0 };

int verbose = 0;

MTEST_THREAD_RETURN_TYPE test_idup(void *arg)
{
    int i;
    int size, rank;
    int ranges[1][3];
    int rleader, isLeft;
    int *excl = NULL;
    int tid = *(int *) arg;

    MPI_Group ingroup, high_group, even_group;
    MPI_Comm local_comm, inter_comm;
    MPI_Comm idupcomms[NUM_IDUPS];
    MPI_Request reqs[NUM_IDUPS];

    MPI_Comm outcomm;
    MPI_Comm incomm = comms[tid];

    MPI_Comm_size(incomm, &size);
    MPI_Comm_rank(incomm, &rank);
    MPI_Comm_group(incomm, &ingroup);

    /* Idup incomm multiple times */
    for (i = 0; i < NUM_IDUPS; i++) {
        MPI_Comm_idup(incomm, &idupcomms[i], &reqs[i]);
    }

    /* Overlap pending idups with various comm generation functions */
    /* Comm_dup */
    MPI_Comm_dup(incomm, &outcomm);
    errs[tid] += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Comm_split */
    MPI_Comm_split(incomm, rank % 2, size - rank, &outcomm);
    errs[tid] += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Comm_create, high half of incomm */
    ranges[0][0] = size / 2;
    ranges[0][1] = size - 1;
    ranges[0][2] = 1;
    MPI_Group_range_incl(ingroup, 1, ranges, &high_group);
    MPI_Comm_create(incomm, high_group, &outcomm);
    MPI_Group_free(&high_group);
    errs[tid] += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Comm_create_group, even ranks of incomm */
    /* exclude the odd ranks */
    excl = malloc((size / 2) * sizeof(int));
    for (i = 0; i < size / 2; i++)
        excl[i] = (2 * i) + 1;

    MPI_Group_excl(ingroup, size / 2, excl, &even_group);
    free(excl);

    if (rank % 2 == 0) {
        MPI_Comm_create_group(incomm, even_group, 0, &outcomm);
    }
    else {
        outcomm = MPI_COMM_NULL;
    }
    MPI_Group_free(&even_group);
    errs[tid] += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    /* Intercomm_create & Intercomm_merge */
    MPI_Comm_split(incomm, (rank < size / 2), rank, &local_comm);
    if (rank == 0) {
        rleader = size / 2;
    }
    else if (rank == size / 2) {
        rleader = 0;
    }
    else {
        rleader = -1;
    }
    isLeft = rank < size / 2;

    MPI_Intercomm_create(local_comm, 0, incomm, rleader, 99, &inter_comm);
    MPI_Intercomm_merge(inter_comm, isLeft, &outcomm);
    MPI_Comm_free(&local_comm);

    errs[tid] += MTestTestComm(inter_comm);
    MTestFreeComm(&inter_comm);
    errs[tid] += MTestTestComm(outcomm);
    MTestFreeComm(&outcomm);

    MPI_Waitall(NUM_IDUPS, reqs, MPI_STATUSES_IGNORE);
    for (i = 0; i < NUM_IDUPS; i++) {
        errs[tid] += MTestTestComm(idupcomms[i]);
        MPI_Comm_free(&idupcomms[i]);
    }
    MPI_Group_free(&ingroup);
    return NULL;
}

int main(int argc, char **argv)
{
    int thread_args[NUM_THREADS];
    int i, provided;
    int toterrs = 0;
    int size;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (provided < MPI_THREAD_MULTIPLE) {
        printf("MPI_THREAD_MULTIPLE for the test\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (size < 2) {
        printf("This test requires at least two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }
    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        MTest_Start_thread(test_idup, (void *) &thread_args[i]);
    }
    MTest_Join_threads();

    for (i = 0; i < NUM_THREADS; i++) {
        MPI_Comm_free(&comms[i]);
        toterrs += errs[i];
    }
    MTest_Finalize(toterrs);
    MPI_Finalize();
    return 0;
}
