
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
 * This test ensures that MPI_Comm_revoke works when a process failure has not
 * occurred yet.
 */
int main(int argc, char **argv)
{
    int rank, size;
    int rc, ec;
    char error[MPI_MAX_ERROR_STRING];
    MPI_Comm world_dup;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        fprintf(stderr, "Must run with at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_dup(MPI_COMM_WORLD, &world_dup);

    if (rank == 1) {
        MPIX_Comm_revoke(world_dup);
    }

    rc = MPI_Barrier(world_dup);
    MPI_Error_class(rc, &ec);
    if (ec != MPIX_ERR_REVOKED) {
        MPI_Error_string(ec, error, &size);
        fprintf(stderr,
                "[%d] MPI_Barrier should have returned MPIX_ERR_REVOKED (%d), but it actually returned: %d\n%s\n",
                rank, MPIX_ERR_REVOKED, ec, error);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_free(&world_dup);

    if (rank == 0)
        fprintf(stdout, " No errors\n");

    MPI_Finalize();

    return 0;
}
