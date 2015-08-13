/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

#define ITERS 10

/* This test uses several scenarios to overlap iallreduce and comm_idup
 * 1.)  Use comm_idup  dublicate the COMM_WORLD and do iallreduce
 *      on the COMM_WORLD
 * 2.)  Do the above test in a loop
 * 3.)  Dublicate  COMM_WORLD, overalp iallreduce on one
 *      communicator with comm_idup on the nother communicator
 * 4.)  Split MPI_COMM_WORLD, communicate on the split communicator
        while dublicating COMM_WORLD
 *  5.) Duplicate the split communicators with comm_idup
 *      while communicating  onCOMM_WORLD
 *  6.) Ceate an inter-communicator and duplicate it with comm_idup while
 *      communicating on the inter-communicator
 *  7.) Dublicate the inter-communicator whil communicate on COMM_WORLD
 *  8.) Merge the inter-communicator to an intra-communicator and idup it,
 *      overlapping with communication on MPI_COMM_WORLD
 *  9.) Communicate on the merge communicator, while duplicating COMM_WORLD
*/

int main(int argc, char **argv)
{
    int errs = 0;
    int i;
    int rank, size, lrank, lsize, rsize, isize;
    int in, out, sol;

    MPI_Comm newcomm, newcomm_v[ITERS], dup_comm, split, ic, merge;
    MPI_Request sreq[ITERS * 2];

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("this test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* set input buffer and compare buffer */
    in = 1;
    sol = size;
    MPI_Comm_idup(MPI_COMM_WORLD, &newcomm, &sreq[0]);
    MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[1]);

    MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    /* Test results of overlapping allreduce */
    if (sol != out)
        errs++;
    /*Test new communicator */
    errs += MTestTestComm(newcomm);
    MPI_Comm_free(&newcomm);

    for (i = 0; i < ITERS; i++) {
        MPI_Comm_idup(MPI_COMM_WORLD, &newcomm_v[i], &sreq[i]);
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[i + ITERS]);
    }
    MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);

    for (i = 0; i < ITERS; i++) {
        errs += MTestTestComm(newcomm_v[i]);
        MPI_Comm_free(&newcomm_v[i]);
    }

    MPI_Comm_dup(MPI_COMM_WORLD, &dup_comm);

    if (rank == 0) {
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[0]);
        MPI_Comm_idup(dup_comm, &newcomm, &sreq[1]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Comm_idup(dup_comm, &newcomm, &sreq[1]);
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[0]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    /* Test Iallreduce */
    if (sol != out)
        errs++;

    /*Test new communicator */
    errs += MTestTestComm(newcomm);

    MPI_Comm_free(&newcomm);
    MPI_Comm_free(&dup_comm);

    MPI_Comm_split(MPI_COMM_WORLD, rank % 2, rank, &split);
    MPI_Comm_rank(split, &lrank);
    MPI_Comm_size(split, &lsize);

    sol = lsize;
    if (lrank == 0) {
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, split, &sreq[0]);
        MPI_Comm_idup(MPI_COMM_WORLD, &newcomm, &sreq[1]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Comm_idup(MPI_COMM_WORLD, &newcomm, &sreq[1]);
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, split, &sreq[0]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    /* Test Iallreduce */
    if (sol != out)
        errs++;;

    /* Test new communicator */
    errs += MTestTestComm(newcomm);
    MPI_Comm_free(&newcomm);
    sol = size;

    if (lrank == 0) {
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[0]);
        MPI_Comm_idup(split, &newcomm, &sreq[1]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Comm_idup(split, &newcomm, &sreq[1]);
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[0]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    /* Test Iallreduce */
    if (sol != out)
        errs++;;

    /* Test new communicator */
    errs += MTestTestComm(newcomm);
    MPI_Comm_free(&newcomm);

    MPI_Intercomm_create(split, 0, MPI_COMM_WORLD, (rank == 0 ? 1 : 0), 1234, &ic);
    MPI_Comm_remote_size(ic, &rsize);

    sol = rsize;

    MPI_Comm_idup(ic, &newcomm, &sreq[1]);
    MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, ic, &sreq[0]);
    MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);


    if (sol != out)
        errs++;;
    /* Test new inter communicator */
    errs += MTestTestComm(newcomm);
    MPI_Comm_free(&newcomm);

    sol = lsize;
    if (lrank == 0) {
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, split, &sreq[0]);
        MPI_Comm_idup(ic, &newcomm, &sreq[1]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Comm_idup(ic, &newcomm, &sreq[1]);
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, split, &sreq[0]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    /* Test Iallreduce resutls for split-communicator */
    if (sol != out)
        errs++;;
    /* Test new inter-communicator */

    errs += MTestTestComm(newcomm);
    MPI_Comm_free(&newcomm);

    MPI_Intercomm_merge(ic, rank % 2, &merge);
    MPI_Comm_size(merge, &isize);

    sol = size;
    if (rank == 0) {
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[0]);
        MPI_Comm_idup(merge, &newcomm, &sreq[1]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Comm_idup(merge, &newcomm, &sreq[1]);
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &sreq[0]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }


    if (sol != out)
        errs++;;
    /* Test new communicator */
    errs += MTestTestComm(newcomm);
    MPI_Comm_free(&newcomm);
    sol = isize;

    if (rank == 0) {
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, merge, &sreq[0]);
        MPI_Comm_idup(MPI_COMM_WORLD, &newcomm, &sreq[1]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Comm_idup(MPI_COMM_WORLD, &newcomm, &sreq[1]);
        MPI_Iallreduce(&in, &out, 1, MPI_INT, MPI_SUM, merge, &sreq[0]);
        MPI_Waitall(2, sreq, MPI_STATUS_IGNORE);
    }

    MPI_Comm_free(&merge);
    MPI_Comm_free(&newcomm);
    MPI_Comm_free(&split);
    MPI_Comm_free(&ic);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
