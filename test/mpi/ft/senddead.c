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
 * This test attempts to MPI_Send with the destination being a dead process.
 * The communication should succeed or report an error. It must not block
 * indefinitely.
 */
int main(int argc, char **argv)
{
    int rank, size, err, errclass;
    char buf[100000];

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
        err = MPI_Send(buf, 100000, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
#if defined (MPICH) && (MPICH_NUMVERSION >= 30100102)
        MPI_Error_class(err, &errclass);
        if ((err) && (errclass != MPIX_ERR_PROC_FAILED)) {
            fprintf(stderr, "Wrong error code (%d) returned. Expected MPIX_ERR_PROC_FAILED\n",
                    errclass);
        }
#endif
        err = MPI_Send(buf, 100000, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
#if defined (MPICH) && (MPICH_NUMVERSION >= 30100102)
        MPI_Error_class(err, &errclass);
        if ((err) && (errclass != MPIX_ERR_PROC_FAILED)) {
            fprintf(stderr, "Wrong error code (%d) returned. Expected MPIX_ERR_PROC_FAILED\n",
                    errclass);
        }
        else {
            printf(" No Errors\n");
            fflush(stdout);
        }
#else
        printf(" No Errors\n");
        fflush(stdout);
#endif
    }

    MPI_Finalize();

    return 0;
}
