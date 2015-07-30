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
 * This test attempts collective communication after a process in
 * the communicator has failed. Since all processes contribute to
 * the result of the operation, all process will receive an error.
 */
int main(int argc, char **argv)
{
    int rank, size;
    int err, errclass;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (size < 2) {
        fprintf(stderr, "Must run with at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 1) {
        exit(EXIT_FAILURE);
    }

    err = MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
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
