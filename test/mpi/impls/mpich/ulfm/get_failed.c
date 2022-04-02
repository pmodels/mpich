/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This test makes sure that after a failure, the correct group of failed
 * processes is returned from MPIX_Comm_get_failed.
 *
 * A bug will likely make test timeout.
 */

#define CHECK_ERR(mpi_errno, name) do { \
    if (mpi_errno != MPI_SUCCESS) { \
        int ec, size; \
        char error[MPI_MAX_ERROR_STRING]; \
        MPI_Error_class(mpi_errno, &ec); \
        MPI_Error_string(mpi_errno, error, &size); \
        printf("%s returned an error: %d - %s\n", name, ec, error); \
        MPI_Abort(MPI_COMM_WORLD, 1); \
    } \
} while (0)

static void wait_for_fail(MPI_Group expected_fail_group)
{
    while (1) {
        MPI_Group failed_grp;
        int err = MPIX_Comm_get_failed(MPI_COMM_WORLD, &failed_grp);
        CHECK_ERR(err, "MPIX_Comm_get_failed");

        if (failed_grp != MPI_GROUP_EMPTY) {
            int result;
            MPI_Group_compare(expected_fail_group, failed_grp, &result);
            MPI_Group_free(&failed_grp);
            if (result == MPI_IDENT) {
                break;
            }
        }
    }
}

char buf[10] = " No errors";

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 3) {
        fprintf(stderr, "Must run with at least 3 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Group world_grp;
    MPI_Comm_group(MPI_COMM_WORLD, &world_grp);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (rank == 0) {
        MPI_Group one_grp;
        int one[] = { 1 };
        MPI_Group_incl(world_grp, 1, one, &one_grp);
        wait_for_fail(one_grp);
        MPI_Group_free(&one_grp);

        int err = MPI_Recv(buf, 10, MPI_CHAR, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        CHECK_ERR(err, "MPI_Recv from rank 2");

        MPI_Group two_grp;
        int two[] = { 1, 2 };
        MPI_Group_incl(world_grp, 2, two, &two_grp);
        wait_for_fail(two_grp);
        MPI_Group_free(&two_grp);

        fprintf(stdout, " No errors\n");

    } else if (rank == 1) {
        exit(EXIT_FAILURE);

    } else if (rank == 2) {
        MPI_Ssend(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        exit(EXIT_FAILURE);
    }

    MPI_Group_free(&world_grp);
    /* This test does not test MPI_Finalize */
}
