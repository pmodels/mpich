/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This test attempts communication between 2 running processes
 * after another process has failed. The communication should complete
 * successfully.
 */
int main(int argc, char **argv)
{
    int rank, size, err;
    char buf[10];
    MPI_Request request;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (size < 4) {
        fprintf(stderr, "Must run with at least 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 1) {
        exit(EXIT_FAILURE);
    }

    if (rank == 0) {
        err = MPI_Issend("No Errors", 10, MPI_CHAR, 3, 0, MPI_COMM_WORLD, &request);
        err += MPI_Wait(&request, MPI_STATUS_IGNORE);
        if (err) {
            fprintf(stderr, "An error occurred during the send operation\n");
        }
    }

    if (rank == 3) {
        err = MPI_Irecv(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &request);
        err += MPI_Wait(&request, MPI_STATUS_IGNORE);
        if (err) {
            fprintf(stderr, "An error occurred during the recv operation\n");
        }
    }

    if (rank == 3) {
        exit(EXIT_FAILURE);
    }

    if (rank == 0) {
        err = MPI_Issend("No Errors", 10, MPI_CHAR, 2, 0, MPI_COMM_WORLD, &request);
        err += MPI_Wait(&request, MPI_STATUS_IGNORE);
        if (err) {
            fprintf(stderr, "An error occurred during the send operation\n");
        }
    }

    if (rank == 2) {
        err = MPI_Irecv(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &request);
        err += MPI_Wait(&request, MPI_STATUS_IGNORE);
        if (err) {
            fprintf(stderr, "An error occurred during the recv operation\n");
        }
        else {
            printf(" %s\n", buf);
            fflush(stdout);
        }
    }

    if (rank == 2) {
        exit(EXIT_FAILURE);
    }

    MPI_Finalize();

    return 0;
}
