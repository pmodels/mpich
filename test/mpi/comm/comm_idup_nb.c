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

#define ITERS 5

int main(int argc, char **argv)
{
    int errs = 0;
    int i, isleft;
    MPI_Comm test_comm, new_comm[ITERS];
    int in[ITERS], out[ITERS], sol;
    int rank, size, rsize, rrank;
    MPI_Request sreq[ITERS * 2];
    int root;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("this test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    while (MTestGetIntracommGeneral(&test_comm, 1, 1)) {
        if (test_comm == MPI_COMM_NULL)
            continue;
        MPI_Comm_size(test_comm, &size);
        MPI_Comm_rank(test_comm, &rank);

        /* Ibarrier */
        for (i = 0; i < ITERS; i++) {
            MPI_Ibarrier(test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);

        for (i = 0; i < ITERS; i++) {
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        /*Ibcast */
        for (i = 0; i < ITERS; i++) {
            if (rank == 0)
                in[i] = 815;
            else
                in[i] = 10;
            MPI_Ibcast(&in[i], 1, MPI_INT, 0, test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        sol = 815;
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);

        for (i = 0; i < ITERS; i++) {
            if (in[i] != sol)
                errs++;
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        /* Iallreduce */
        for (i = 0; i < ITERS; i++) {
            in[i] = 1;
            MPI_Iallreduce(&in[i], &out[i], 1, MPI_INT, MPI_SUM, test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        sol = size;
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);

        for (i = 0; i < ITERS; i++) {
            if (out[i] != sol)
                errs++;
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        /* Isann */
        for (i = 0; i < ITERS; i++) {
            MPI_Iscan(&rank, &out[i], 1, MPI_INT, MPI_SUM, test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        sol = rank * (rank + 1) / 2;
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);

        for (i = 0; i < ITERS; i++) {
            if (out[i] != sol)
                errs++;
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        /*Ibcast */
        for (i = 0; i < ITERS; i++) {
            if (rank == 0)
                in[i] = 815;
            else
                in[i] = 10;
            MPI_Ibcast(&in[i], 1, MPI_INT, 0, test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);
        sol = 815;
        for (i = 0; i < ITERS; i++) {
            if (in[i] != sol)
                errs++;
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        MTestFreeComm(&test_comm);
    }
/* Now the test for inter-communicators */
    while (MTestGetIntercomm(&test_comm, &isleft, 1)) {
        if (test_comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_size(test_comm, &size);
        MPI_Comm_rank(test_comm, &rank);

        MPI_Comm_remote_size(test_comm, &rsize);
        /* Ibarrier */
        for (i = 0; i < ITERS; i++) {
            MPI_Ibarrier(test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);
        for (i = 0; i < ITERS; i++) {
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        /*Ibcast */
        for (i = 0; i < ITERS; i++) {
            if (isleft) {
                if (rank == 0) {
                    root = MPI_ROOT;
                    in[i] = 815;
                }
                else {
                    root = MPI_PROC_NULL;
                    in[i] = 815;        /* not needed, just to make correctness checking easier */
                }
            }
            else {
                root = 0;
                in[i] = 213;    /* garbage value */
            }
            MPI_Ibcast(&in[i], 1, MPI_INT, root, test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        sol = 815;
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);

        for (i = 0; i < ITERS; i++) {
            if (in[i] != sol)
                errs++;
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        /* Iallreduce */
        for (i = 0; i < ITERS; i++) {
            in[i] = 1;
            MPI_Iallreduce(&in[i], &out[i], 1, MPI_INT, MPI_SUM, test_comm, &sreq[i]);
            MPI_Comm_idup(test_comm, &new_comm[i], &sreq[i + ITERS]);
        }
        sol = rsize;
        MPI_Waitall(ITERS * 2, sreq, MPI_STATUS_IGNORE);

        for (i = 0; i < ITERS; i++) {
            if (out[i] != sol)
                errs++;
            errs += MTestTestComm(new_comm[i]);
            MPI_Comm_free(&new_comm[i]);
        }
        MTestFreeComm(&test_comm);
    }
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
