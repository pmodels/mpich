/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * A simple test of the intercomm create routine, with a communication test
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Comm intercomm;
    int remote_rank, rank, size, errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) {
        printf("Size must be at least 2\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Make an intercomm of the first two elements of comm_world */
    if (rank < 2) {
        int lrank = rank, rrank = -1;
        MPI_Status status;

        remote_rank = 1 - rank;
        MPI_Intercomm_create(MPI_COMM_SELF, 0, MPI_COMM_WORLD, remote_rank, 27, &intercomm);

        /* Now, communicate between them */
        MPI_Sendrecv(&lrank, 1, MPI_INT, 0, 13, &rrank, 1, MPI_INT, 0, 13, intercomm, &status);

        if (rrank != remote_rank) {
            errs++;
            printf("%d Expected %d but received %d\n", rank, remote_rank, rrank);
        }

        MPI_Comm_free(&intercomm);

#if MPI_VERSION >= 4
        /* Test MPI_Intercomm_create_from_groups */
        MPI_Group world_group, local_group, remote_group;
        MPI_Comm_group(MPI_COMM_WORLD, &world_group);
        MPI_Group_incl(world_group, 1, &rank, &local_group);
        MPI_Group_incl(world_group, 1, &remote_rank, &remote_group);
        MPI_Group_free(&world_group);
        MPI_Intercomm_create_from_groups(local_group, 0, remote_group, 0, "tag",
                                         MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &intercomm);

        /* Now, communicate between them */
        MPI_Sendrecv(&lrank, 1, MPI_INT, 0, 13, &rrank, 1, MPI_INT, 0, 13, intercomm, &status);

        if (rrank != remote_rank) {
            errs++;
            printf("%d Expected %d but received %d\n", rank, remote_rank, rrank);
        }

        MPI_Group_free(&local_group);
        MPI_Group_free(&remote_group);
        MPI_Comm_free(&intercomm);
#endif
    }

    /* The next test should create an intercomm with groups of different
     * sizes FIXME */

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
