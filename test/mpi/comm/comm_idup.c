/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

/* This is a temporary #ifdef to control whether we test this functionality.  A
 * configure-test or similar would be better.  Eventually the MPI-3 standard
 * will be released and this can be gated on a MPI_VERSION check */
#if !defined(USE_STRICT_MPI) && defined(MPICH2)
#define TEST_IDUP 1
#endif

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__); \
            }                                                                     \
        }                                                                         \
    } while (0)

int main(int argc, char **argv)
{
    int errs = 0;
    int i;
    int rank, size, lrank, lsize, rsize;
    int buf[2];
    MPI_Comm newcomm, ic, localcomm;
    MPI_Request rreq;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("this test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

#ifdef TEST_IDUP

    /* test plan: make rank 0 wait in a blocking recv until all other processes
     * have posted their MPIX_Comm_idup ops, then post last.  Should ensure that
     * idup doesn't block on the non-zero ranks, otherwise we'll get a deadlock.
     */

    if (rank == 0) {
        for (i = 1; i < size; ++i) {
            buf[0] = 0x01234567;
            buf[1] = 0x89abcdef;
            MPI_Recv(buf, 2, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        MPIX_Comm_idup(MPI_COMM_WORLD, &newcomm, &rreq);
        MPI_Wait(&rreq, MPI_STATUS_IGNORE);
    }
    else {
        MPIX_Comm_idup(MPI_COMM_WORLD, &newcomm, &rreq);
        buf[0] = rank;
        buf[1] = size + rank;
        MPI_Ssend(buf, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Wait(&rreq, MPI_STATUS_IGNORE);
    }

    /* do some communication to make sure that newcomm works */
    buf[0] = rank;
    buf[1] = 0xfeedface;
    MPI_Allreduce(&buf[0], &buf[1], 1, MPI_INT, MPI_SUM, newcomm);
    check(buf[1] == (size * (size-1) / 2));

    MPI_Comm_free(&newcomm);

    /* now construct an intercomm and make sure we can dup that too */
    MPI_Comm_split(MPI_COMM_WORLD, rank % 2, rank, &localcomm);
    MPI_Intercomm_create(localcomm, 0, MPI_COMM_WORLD, (rank == 0 ? 1 : 0), 1234, &ic);
    MPI_Comm_free(&localcomm);

    MPI_Comm_rank(ic, &lrank);
    MPI_Comm_size(ic, &lsize);
    MPI_Comm_remote_size(ic, &rsize);

    /* Similar to above pattern, but all non-local-rank-0 processes send to
     * remote rank 0.  Both sides participate in this way. */
    if (lrank == 0) {
        for (i = 1; i < rsize; ++i) {
            buf[0] = 0x01234567;
            buf[1] = 0x89abcdef;
            MPI_Recv(buf, 2, MPI_INT, i, 0, ic, MPI_STATUS_IGNORE);
        }
        MPIX_Comm_idup(ic, &newcomm, &rreq);
        MPI_Wait(&rreq, MPI_STATUS_IGNORE);
    }
    else {
        MPIX_Comm_idup(ic, &newcomm, &rreq);
        buf[0] = lrank;
        buf[1] = lsize + lrank;
        MPI_Ssend(buf, 2, MPI_INT, 0, 0, ic);
        MPI_Wait(&rreq, MPI_STATUS_IGNORE);
    }

    /* do some communication to make sure that newcomm works */
    buf[0] = lrank;
    buf[1] = 0xfeedface;
    MPI_Allreduce(&buf[0], &buf[1], 1, MPI_INT, MPI_SUM, newcomm);
    check(buf[1] == (rsize * (rsize-1) / 2));

    MPI_Comm_free(&newcomm);
    MPI_Comm_free(&ic);

#endif /* TEST_IDUP */

    MPI_Reduce((rank == 0 ? MPI_IN_PLACE : &errs), &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }

    MPI_Finalize();

    return 0;
}

