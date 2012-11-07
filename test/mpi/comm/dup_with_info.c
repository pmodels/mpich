/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int run_tests(MPI_Comm comm)
{
    int rank, size, wrank, wsize, dest, a, b, errs = 0;
    MPI_Status status;

    /* Check basic properties */
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    if (size != wsize || rank != wrank) {
        errs++;
        fprintf(stderr, "Size (%d) or rank (%d) wrong\n", size, rank);
        fflush(stderr);
    }

    MPI_Barrier(comm);

    /* Can we communicate with this new communicator? */
    dest = MPI_PROC_NULL;
    if (rank == 0) {
        dest = size - 1;
        a = rank;
        b = -1;
        MPI_Sendrecv(&a, 1, MPI_INT, dest, 0, &b, 1, MPI_INT, dest, 0, comm, &status);
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
        MPI_Sendrecv(&a, 1, MPI_INT, dest, 0, &b, 1, MPI_INT, dest, 0, comm, &status);
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

    MPI_Barrier(comm);

    return errs;
}

int main(int argc, char **argv)
{
    int total_errs = 0;
    MPI_Comm newcomm;
    MPI_Info info;

    MTest_Init(&argc, &argv);

    /* Dup with no info */
    MPI_Comm_dup_with_info(MPI_COMM_WORLD, MPI_INFO_NULL, &newcomm);
    total_errs += run_tests(newcomm);
    MPI_Comm_free(&newcomm);

    /* Dup with info keys */
    MPI_Info_create(&info);
    MPI_Info_set(info, (char *) "host", (char *) "myhost.myorg.org");
    MPI_Info_set(info, (char *) "file", (char *) "runfile.txt");
    MPI_Info_set(info, (char *) "soft", (char *) "2:1000:4,3:1000:7");
    MPI_Comm_dup_with_info(MPI_COMM_WORLD, info, &newcomm);
    total_errs += run_tests(newcomm);
    MPI_Info_free(&info);
    MPI_Comm_free(&newcomm);

    /* Dup with deleted info keys */
    MPI_Info_create(&info);
    MPI_Info_set(info, (char *) "host", (char *) "myhost.myorg.org");
    MPI_Info_set(info, (char *) "file", (char *) "runfile.txt");
    MPI_Info_set(info, (char *) "soft", (char *) "2:1000:4,3:1000:7");
    MPI_Comm_dup_with_info(MPI_COMM_WORLD, info, &newcomm);
    MPI_Info_free(&info);
    total_errs += run_tests(newcomm);
    MPI_Comm_free(&newcomm);

    MTest_Finalize(total_errs);

    MPI_Finalize();

    return 0;
}
