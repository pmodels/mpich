/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * This test attempts MPI_Irecv with the source being a dead process. It should fail
 * and return an error at completion. If we are testing sufficiently new MPICH, we
 * look for the MPIX_ERR_PROC_FAILED error code. These should be converted to look
 * for the standarized error code once it is finalized.
 */
int main(int argc, char **argv)
{
    int rank, size, err, errclass;
    MPI_Request request;
    char buf[10];

    MPI_Init(&argc, &argv);
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
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
        err = MPI_Irecv(buf, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &request);
        if (err)
            fprintf(stderr, "MPI_Irecv returned an error");

        err = MPI_Wait(&request, MPI_STATUS_IGNORE);
#if defined (MPICH) && (MPICH_NUMVERSION >= 30100102)
        MPI_Error_class(err, &errclass);
        if (errclass == MPIX_ERR_PROC_FAILED) {
            printf(" No Errors\n");
            fflush(stdout);
        }
        else {
            fprintf(stderr, "Wrong error code (%d) returned. Expected MPIX_ERR_PROC_FAILED\n",
                    errclass);
        }
#else
        if (err) {
            printf(" No Errors\n");
            fflush(stdout);
        }
        else {
            fprintf(stderr, "Program reported MPI_SUCCESS, but an error code was expected.\n");
        }
#endif
    }

    MPI_Finalize();

    return 0;
}
