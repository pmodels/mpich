/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    int errs = 0;
    int rank, size, wrank, wsize, dest, a, b;
    MPI_Comm newcomm;
    MPI_Status status;

    MTest_Init(&argc, &argv);

    /* Can we run comm dup at all? */
    MPI_Comm_dup(MPI_COMM_WORLD, &newcomm);

    /* Check basic properties */
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(newcomm, &size);
    MPI_Comm_rank(newcomm, &rank);

    if (size != wsize || rank != wrank) {
        errs++;
        fprintf(stderr, "Size (%d) or rank (%d) wrong\n", size, rank);
        fflush(stderr);
    }

    /* Can we communicate with this new communicator? */
    dest = MPI_PROC_NULL;
    if (rank == 0) {
        dest = size - 1;
        a = rank;
        b = -1;
        MPI_Sendrecv(&a, 1, MPI_INT, dest, 0, &b, 1, MPI_INT, dest, 0, newcomm, &status);
        if (b != dest) {
            errs++;
            fprintf(stderr, "Received %d expected %d on %d\n", b, dest, rank);
            fflush(stderr);
        }
        if (status.MPI_SOURCE != dest) {
            errs++;
            fprintf(stderr, "Source not set correctly in status on %d\n", rank);
            fflush(stderr);
        }
    }
    else if (rank == size - 1) {
        dest = 0;
        a = rank;
        b = -1;
        MPI_Sendrecv(&a, 1, MPI_INT, dest, 0, &b, 1, MPI_INT, dest, 0, newcomm, &status);
        if (b != dest) {
            errs++;
            fprintf(stderr, "Received %d expected %d on %d\n", b, dest, rank);
            fflush(stderr);
        }
        if (status.MPI_SOURCE != dest) {
            errs++;
            fprintf(stderr, "Source not set correctly in status on %d\n", rank);
            fflush(stderr);
        }
    }

    MPI_Comm_free(&newcomm);

    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}
