/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* regression test for ticket #1574
 *
 * Based on test code from N. Radclif @ Cray. */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    MPI_Comm c0, c1, ic;
    MPI_Group g0, g1, gworld;
    int a, b, c, d;
    int rank, size, remote_leader, tag;
    int ranks[2];
    int errs = 0;

    tag = 5;
    c0 = c1 = ic = MPI_COMM_NULL;
    g0 = g1 = gworld = MPI_GROUP_NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 33) {
        printf("ERROR: this test requires at least 33 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    /* group of c0
     * NOTE: a>=32 is essential for exercising the loop bounds bug from tt#1574 */
    a = 32;
    b = 24;

    /* group of c1 */
    c = 25;
    d = 26;

    MPI_Comm_group(MPI_COMM_WORLD, &gworld);

    ranks[0] = a;
    ranks[1] = b;
    MPI_Group_incl(gworld, 2, ranks, &g0);
    MPI_Comm_create(MPI_COMM_WORLD, g0, &c0);

    ranks[0] = c;
    ranks[1] = d;
    MPI_Group_incl(gworld, 2, ranks, &g1);
    MPI_Comm_create(MPI_COMM_WORLD, g1, &c1);

    if (rank == a || rank == b) {
        remote_leader = c;
        MPI_Intercomm_create(c0, 0, MPI_COMM_WORLD, remote_leader, tag, &ic);
    }
    else if (rank == c || rank == d) {
        remote_leader = a;
        MPI_Intercomm_create(c1, 0, MPI_COMM_WORLD, remote_leader, tag, &ic);
    }

    MPI_Group_free(&g0);
    MPI_Group_free(&g1);
    MPI_Group_free(&gworld);

    if (c0 != MPI_COMM_NULL)
        MPI_Comm_free(&c0);
    if (c1 != MPI_COMM_NULL)
        MPI_Comm_free(&c1);
    if (ic != MPI_COMM_NULL)
        MPI_Comm_free(&ic);


    MPI_Reduce((rank == 0 ? MPI_IN_PLACE : &errs), &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }
    MPI_Finalize();

    return 0;
}
