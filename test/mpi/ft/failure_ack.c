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
 * This test makes sure that after a failure, the correct group of failed
 * processes is returned from MPIX_Comm_failure_ack/get_acked.
 */
int main(int argc, char **argv)
{
    int rank, size, err, result, i;
    char buf[10] = " No errors";
    char error[MPI_MAX_ERROR_STRING];
    MPI_Group failed_grp, one_grp, world_grp;
    int one[] = { 1 };
    int world_ranks[] = { 0, 1, 2 };
    int failed_ranks[3];

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

    if (rank == 0) {
        err = MPI_Recv(buf, 10, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (MPI_SUCCESS == err) {
            fprintf(stderr, "Expected a failure for receive from rank 1\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        err = MPIX_Comm_failure_ack(MPI_COMM_WORLD);
        if (MPI_SUCCESS != err) {
            int ec;
            MPI_Error_class(err, &ec);
            MPI_Error_string(err, error, &size);
            fprintf(stderr, "MPIX_Comm_failure_ack returned an error: %d\n%s", ec, error);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        err = MPIX_Comm_failure_get_acked(MPI_COMM_WORLD, &failed_grp);
        if (MPI_SUCCESS != err) {
            int ec;
            MPI_Error_class(err, &ec);
            MPI_Error_string(err, error, &size);
            fprintf(stderr, "MPIX_Comm_failure_get_acked returned an error: %d\n%s", ec, error);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Comm_group(MPI_COMM_WORLD, &world_grp);
        MPI_Group_incl(world_grp, 1, one, &one_grp);
        MPI_Group_compare(one_grp, failed_grp, &result);
        if (MPI_IDENT != result) {
            fprintf(stderr, "First failed group contains incorrect processes\n");
            MPI_Group_size(failed_grp, &size);
            MPI_Group_translate_ranks(failed_grp, size, world_ranks, world_grp, failed_ranks);
            for (i = 0; i < size; i++)
                fprintf(stderr, "DEAD: %d\n", failed_ranks[i]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        MPI_Group_free(&failed_grp);

        err = MPI_Recv(buf, 10, MPI_CHAR, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (MPI_SUCCESS != err) {
            fprintf(stderr, "First receive failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        err = MPI_Recv(buf, 10, MPI_CHAR, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (MPI_SUCCESS == err) {
            fprintf(stderr, "Expected a failure for receive from rank 2\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        err = MPIX_Comm_failure_get_acked(MPI_COMM_WORLD, &failed_grp);
        if (MPI_SUCCESS != err) {
            int ec;
            MPI_Error_class(err, &ec);
            MPI_Error_string(err, error, &size);
            fprintf(stderr, "MPIX_Comm_failure_get_acked returned an error: %d\n%s", ec, error);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Group_compare(one_grp, failed_grp, &result);
        if (MPI_IDENT != result) {
            fprintf(stderr, "Second failed group contains incorrect processes\n");
            MPI_Group_size(failed_grp, &size);
            MPI_Group_translate_ranks(failed_grp, size, world_ranks, world_grp, failed_ranks);
            for (i = 0; i < size; i++)
                fprintf(stderr, "DEAD: %d\n", failed_ranks[i]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fprintf(stdout, " No errors\n");
    }
    else if (rank == 2) {
        MPI_Ssend(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        exit(EXIT_FAILURE);
    }

    MPI_Group_free(&failed_grp);
    MPI_Group_free(&one_grp);
    MPI_Group_free(&world_grp);
    MPI_Finalize();
}
