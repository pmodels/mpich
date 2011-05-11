/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpix.h>

#define LOOPS 100

int main(int argc, char **argv)
{
    int rank, size, i, j, count;
    MPI_Group full_group, sub_group;
    int *included, *ranks;
    MPI_Comm comm;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    ranks = malloc(size * sizeof(int));
    included = malloc(size * sizeof(int));
    MPI_Comm_group(MPI_COMM_WORLD, &full_group);

    for (j = 0; j < LOOPS; j++) {
        srand(j); /* Deterministic seed */

        count = 0;
        for (i = 0; i < size; i++) {
            if (rand() % 2) { /* randomly include a rank */
                included[i] = 1;
                ranks[count++] = i;
            }
            else
                included[i] = 0;
        }

        MPI_Group_incl(full_group, count, ranks, &sub_group);

        if (included[rank]) {
            MPIX_Group_comm_create(MPI_COMM_WORLD, sub_group, 0, &comm);
            MPI_Barrier(comm);
            MPI_Comm_free(&comm);
        }

        MPI_Group_free(&sub_group);
    }

    MPI_Group_free(&full_group);

    if (rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
