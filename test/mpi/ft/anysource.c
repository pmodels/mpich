/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * This test makes sure the MPI_ANY_SOURCE receive operations are handled
 * correctly. */
int main(int argc, char **argv)
{
    int rank, size, err, ec;
    char buf[10] = " No errors";
    char error[MPI_MAX_ERROR_STRING];
    MPI_Request request;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 3) {
        fprintf(stderr, "Must run with at least 3 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (rank == 1) {
        exit(EXIT_FAILURE);
    }

    /* Make sure ANY_SOURCE returns correctly after a failure */
    if (rank == 0) {
        err = MPI_Recv(buf, 10, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (MPI_SUCCESS == err) {
            fprintf(stderr, "Expected a failure for receive from ANY_SOURCE\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        /* Make sure that new ANY_SOURCE operations don't work yet */
        MPI_Irecv(buf, 10, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        err = MPI_Wait(&request, &status);
        if (MPI_SUCCESS == err) {
            fprintf(stderr, "Expected a failure for receive from ANY_SOURCE\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        /* Make sure that ANY_SOURCE works after failures are acknowledged */
        MPIX_Comm_failure_ack(MPI_COMM_WORLD);
        err = MPI_Wait(&request, &status);
        if (MPI_SUCCESS != err) {
            MPI_Error_class(err, &ec);
            MPI_Error_string(err, error, &size);
            fprintf(stderr, "Unexpected failure after acknowledged failure (%d)\n%s", ec, error);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fprintf(stdout, "%s\n", buf);
    } else if (rank == 2) {
        /* Make sure we don't send our first message too early */
        sleep(2);

        err = MPI_Send(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        if (MPI_SUCCESS != err) {
            MPI_Error_class(err, &ec);
            MPI_Error_string(err, error, &size);
            fprintf(stderr, "Unexpected failure from MPI_Send (%d)\n%s", ec, error);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Finalize();
}
