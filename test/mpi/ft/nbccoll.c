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
 * This test checks if a non-blocking collective failing impacts the result
 * of another operation going on at the same time.
 */
int main(int argc, char **argv)
{
    int rank, size;
    int err;
    int excl;
    MPI_Comm small_comm;
    MPI_Group orig_grp, small_grp;
    MPI_Request req;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_group(MPI_COMM_WORLD, &orig_grp);
    MPI_Group_size(orig_grp, &excl);
    excl--;
    MPI_Group_excl(orig_grp, 1, &excl, &small_grp);
    MPI_Comm_create_group(MPI_COMM_WORLD, small_grp, 0, &small_comm);
    MPI_Group_free(&orig_grp);
    MPI_Group_free(&small_grp);

    if (size < 4) {
        fprintf(stderr, "Must run with at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == excl) {
        exit(EXIT_FAILURE);
    }

    MPI_Ibarrier(MPI_COMM_WORLD, &req);

    err = MPI_Barrier(small_comm);
    if (err != MPI_SUCCESS) {
        int ec;
        MPI_Error_class(err, &ec);
        fprintf(stderr, "Result != MPI_SUCCESS: %d\n", ec);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MPI_Wait(&req, &status);
    if (err == MPI_SUCCESS) {
        int ec, ec2;
        MPI_Error_class(err, &ec);
        MPI_Error_class(status.MPI_ERROR, &ec2);
        fprintf(stderr, "Result != MPIX_ERR_PROC_FAILED: %d Status: %d\n", ec, ec2);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_free(&small_comm);

    if (rank == 0) {
        printf(" No Errors\n");
        fflush(stdout);
    }

    MPI_Finalize();

    return 0;
}
