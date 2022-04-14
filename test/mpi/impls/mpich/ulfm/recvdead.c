/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * This test attempts MPI_Recv with the source being a dead process. It should fail
 * and return an error. If we are testing sufficiently new MPICH, we look for the
 * MPIX_ERR_PROC_FAILED error code. These should be converted to look for the
 * standardized error code once it is finalized.
 */

enum {
    RECV,
    IRECV,
};

int test_op = RECV;

int main(int argc, char **argv)
{
    int err, toterrs = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-op=recv") == 0) {
            test_op = RECV;
        } else if (strcmp(argv[i], "-op=irecv") == 0) {
            test_op = IRECV;
        }
    }

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        fprintf(stderr, "Must run with at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 1) {
        exit(EXIT_FAILURE);
    }

    if (rank == 0) {
        char buf[10];
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
        if (test_op == RECV) {
            err = MPI_Recv(buf, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (test_op = IRECV) {
            MPI_Request request;
            err = MPI_Irecv(buf, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &request);
            if (err) {
                fprintf(stderr, "MPI_Irecv returned an error");
                toterrs++;
            }

            err = MPI_Wait(&request, MPI_STATUS_IGNORE);
        }

        int errclass;
        MPI_Error_class(err, &errclass);
        if (errclass == MPIX_ERR_PROC_FAILED) {
            printf(" No Errors\n");
            fflush(stdout);
        } else {
            fprintf(stderr, "Wrong error code (%d) returned. Expected MPIX_ERR_PROC_FAILED\n",
                    errclass);
            toterrs++;
        }
    }

    MPI_Finalize();

    return MTestReturnValue(toterrs);
}
