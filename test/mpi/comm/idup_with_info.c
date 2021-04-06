/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static int run_tests(MPI_Comm comm)
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
    } else if (rank == size - 1) {
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
    int errs = 0;
    MPI_Comm newcomm1, newcomm2, newcomm3, newcomm4;
    MPI_Request reqs[2];
    MPI_Info info;
    int rank;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Dup with no info */
    MPI_Comm_idup_with_info(MPI_COMM_WORLD, MPI_INFO_NULL, &newcomm1, &reqs[0]);
    MPI_Wait(&reqs[0], MPI_STATUS_IGNORE);
    errs += run_tests(newcomm1);

    MPI_Comm_idup_with_info(MPI_COMM_WORLD, MPI_INFO_NULL, &newcomm2, &reqs[0]);
    MPI_Wait(&reqs[0], MPI_STATUS_IGNORE);
    errs += run_tests(newcomm2);

    /* Dup with info keys in mixed order */
    MPI_Info_create(&info);
    MPI_Info_set(info, (char *) "host", (char *) "myhost.myorg.org");
    MPI_Info_set(info, (char *) "file", (char *) "runfile.txt");
    MPI_Info_set(info, (char *) "soft", (char *) "2:1000:4,3:1000:7");

    if (rank % 2 == 0) {
        MPI_Comm_idup_with_info(newcomm1, info, &newcomm3, &reqs[0]);
        MPI_Comm_idup_with_info(newcomm2, info, &newcomm4, &reqs[1]);
    } else {
        MPI_Comm_idup_with_info(newcomm2, info, &newcomm4, &reqs[0]);
        MPI_Comm_idup_with_info(newcomm1, info, &newcomm3, &reqs[1]);
    }
    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

    errs += run_tests(newcomm3);
    errs += run_tests(newcomm4);

    MPI_Info_free(&info);
    MPI_Comm_free(&newcomm4);
    MPI_Comm_free(&newcomm3);

    /* Free info keys before the communicator is fully created */
    MPI_Info_create(&info);
    MPI_Info_set(info, (char *) "host", (char *) "myhost.myorg.org");
    MPI_Info_set(info, (char *) "file", (char *) "runfile.txt");
    MPI_Info_set(info, (char *) "soft", (char *) "2:1000:4,3:1000:7");

    if (rank % 2 == 0) {
        MPI_Comm_idup_with_info(newcomm1, info, &newcomm3, &reqs[0]);
        MPI_Comm_idup_with_info(newcomm2, info, &newcomm4, &reqs[1]);
    } else {
        MPI_Comm_idup_with_info(newcomm2, info, &newcomm4, &reqs[0]);
        MPI_Comm_idup_with_info(newcomm1, info, &newcomm3, &reqs[1]);
    }
    MPI_Info_free(&info);
    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

    errs += run_tests(newcomm3);
    errs += run_tests(newcomm4);

    MPI_Comm_free(&newcomm4);
    MPI_Comm_free(&newcomm3);

    MPI_Comm_free(&newcomm2);
    MPI_Comm_free(&newcomm1);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
