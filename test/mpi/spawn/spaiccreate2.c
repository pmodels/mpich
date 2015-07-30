/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Use Spawn to create an intercomm, then create a new intercomm between processes from mixed worlds.";
*/

/*
 * This test is a regression test for ticket#114 based on a bug reported by Brad
 * Penoff.  The specific bug only occurred when MPI_Intercomm_create was called
 * with a local communicator argument that contained one or more processes from
 * a different MPI_COMM_WORLD.
 */

int main(int argc, char *argv[])
{
    int errs = 0;
    int wrank, wsize, mrank, msize, inter_rank;
    int np = 2;
    int errcodes[2];
    int rrank = -1;
    MPI_Comm parentcomm, intercomm, intercomm2, even_odd_comm, merged_world;
    int can_spawn;

    MTest_Init(&argc, &argv);

    errs += MTestSpawnPossible(&can_spawn);

    if (can_spawn) {
        MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
        MPI_Comm_size(MPI_COMM_WORLD, &wsize);

        if (wsize != 2) {
            printf("world size != 2, this test will not work correctly\n");
            errs++;
        }

        MPI_Comm_get_parent(&parentcomm);

        if (parentcomm == MPI_COMM_NULL) {
            MPI_Comm_spawn((char *) "./spaiccreate2", MPI_ARGV_NULL, np,
                           MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes);
        }
        else {
            intercomm = parentcomm;
        }

        MPI_Intercomm_merge(intercomm, (parentcomm == MPI_COMM_NULL ? 0 : 1), &merged_world);
        MPI_Comm_rank(merged_world, &mrank);
        MPI_Comm_size(merged_world, &msize);

        MPI_Comm_split(merged_world, mrank % 2, wrank, &even_odd_comm);

        MPI_Intercomm_create(even_odd_comm, 0, merged_world, (mrank + 1) % 2, 123, &intercomm2);
        MPI_Comm_rank(intercomm2, &inter_rank);

        /* odds receive from evens */
        MPI_Sendrecv(&inter_rank, 1, MPI_INT, inter_rank, 456,
                     &rrank, 1, MPI_INT, inter_rank, 456, intercomm2, MPI_STATUS_IGNORE);
        if (rrank != inter_rank) {
            printf("Received %d from %d; expected %d\n", rrank, inter_rank, inter_rank);
            errs++;
        }

        MPI_Barrier(intercomm2);

        MPI_Comm_free(&intercomm);
        MPI_Comm_free(&intercomm2);
        MPI_Comm_free(&merged_world);
        MPI_Comm_free(&even_odd_comm);

        /* Note that the MTest_Finalize get errs only over COMM_WORLD */
        /* Note also that both the parent and child will generate "No Errors"
         * if both call MTest_Finalize */
        if (parentcomm == MPI_COMM_NULL) {
            MTest_Finalize(errs);
        }
    }
    else {
        MTest_Finalize(errs);
    }

    MPI_Finalize();
    return 0;
}
