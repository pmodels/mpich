/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int i, rank, size, color;
    MPI_Group group;
    MPI_Comm primary[2], secondary[2], tmp;
    MPI_Request req[2];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Each pair of processes creates a communicator */
    for (i = 0; i < size; i++) {
        if (rank == i)
            MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &primary[0]);
        else if (rank == (i + 1) % size)
            MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &secondary[0]);
        else {
            MPI_Comm_split(MPI_COMM_WORLD, 1, 0, &tmp);
            MPI_Comm_free(&tmp);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    /* Each pair dups the communicator such that the dups are
     * overlapping.  If this were done with MPI_Comm_dup, this should
     * deadlock. */
    MPI_Comm_idup(primary[0], &primary[1], &req[0]);
    MPI_Comm_idup(secondary[0], &secondary[1], &req[1]);
    MPI_Waitall(2, req, MPI_STATUSES_IGNORE);

    for (i = 0; i < 2; i++) {
        MPI_Comm_free(&primary[i]);
        MPI_Comm_free(&secondary[i]);
    }

    if (rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
