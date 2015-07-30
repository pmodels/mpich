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
    char buf[10];
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
        char buf[10];
        err = MPI_Recv(buf, 10, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Error_class(err, &ec);
        if (MPIX_ERR_PROC_FAILED != ec) {
            fprintf(stderr, "Expected MPIX_ERR_PROC_FAILED for receive from ANY_SOURCE: %d\n", ec);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        /* Make sure that new ANY_SOURCE operations don't work yet */
        err = MPI_Irecv(buf, 10, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        if (request == MPI_REQUEST_NULL) {
            fprintf(stderr, "Request for ANY_SOURCE receive is NULL\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        MPI_Error_class(err, &ec);
        if (ec != MPI_SUCCESS && ec != MPIX_ERR_PROC_FAILED_PENDING) {
            fprintf(stderr, "Expected SUCCESS or MPIX_ERR_PROC_FAILED_PENDING: %d\n", ec);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        err = MPI_Wait(&request, &status);
        MPI_Error_class(err, &ec);
        if (MPIX_ERR_PROC_FAILED_PENDING != ec) {
            fprintf(stderr,
                    "Expected a MPIX_ERR_PROC_FAILED_PENDING (%d) for receive from ANY_SOURCE: %d\n",
                    MPIX_ERR_PROC_FAILED_PENDING, ec);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        err = MPI_Send(NULL, 0, MPI_INT, 2, 0, MPI_COMM_WORLD);
        if (MPI_SUCCESS != err) {
            MPI_Error_class(err, &ec);
            fprintf(stderr, "MPI_SEND failed: %d", ec);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        /* Make sure that ANY_SOURCE works after failures are acknowledged */
        MPIX_Comm_failure_ack(MPI_COMM_WORLD);
        err = MPI_Wait(&request, &status);
        if (MPI_SUCCESS != err) {
            MPI_Error_class(err, &ec);
            fprintf(stderr, "Unexpected failure after acknowledged failure (%d)\n", ec);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fprintf(stdout, " %s\n", buf);
    }
    else if (rank == 2) {
        char buf[10] = "No errors";
        err = MPI_Recv(NULL, 0, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (MPI_SUCCESS != err) {
            MPI_Error_class(err, &ec);
            fprintf(stderr, "MPI_RECV failed: %d\n", ec);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        err = MPI_Send(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        if (MPI_SUCCESS != err) {
            MPI_Error_class(err, &ec);
            fprintf(stderr, "Unexpected failure from MPI_Send (%d)\n", ec);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Finalize();
}
