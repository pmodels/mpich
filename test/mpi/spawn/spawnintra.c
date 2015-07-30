/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "A simple test of Comm_spawn, followed by intercomm merge";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI_Comm parentcomm, intercomm, intracomm, intracomm2, intracomm3;
    int isChild = 0;
    MPI_Status status;
    int can_spawn;

    MTest_Init(&argc, &argv);

    errs += MTestSpawnPossible(&can_spawn);

    if (can_spawn) {
        MPI_Comm_get_parent(&parentcomm);

        if (parentcomm == MPI_COMM_NULL) {
            /* Create 2 more processes */
            MPI_Comm_spawn((char *) "./spawnintra", MPI_ARGV_NULL, np,
                           MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes);
        }
        else
            intercomm = parentcomm;

        /* We now have a valid intercomm */

        MPI_Comm_remote_size(intercomm, &rsize);
        MPI_Comm_size(intercomm, &size);
        MPI_Comm_rank(intercomm, &rank);

        if (parentcomm == MPI_COMM_NULL) {
            /* Master */
            if (rsize != np) {
                errs++;
                printf("Did not create %d processes (got %d)\n", np, rsize);
            }
            if (rank == 0) {
                for (i = 0; i < rsize; i++) {
                    MPI_Send(&i, 1, MPI_INT, i, 0, intercomm);
                }
            }
        }
        else {
            /* Child */
            isChild = 1;
            if (size != np) {
                errs++;
                printf("(Child) Did not create %d processes (got %d)\n", np, size);
            }
            MPI_Recv(&i, 1, MPI_INT, 0, 0, intercomm, &status);
            if (i != rank) {
                errs++;
                printf("Unexpected rank on child %d (%d)\n", rank, i);
            }
        }

        /* At this point, try to form the intracommunicator */
        MPI_Intercomm_merge(intercomm, isChild, &intracomm);

        /* Check on the intra comm */
        {
            int icsize, icrank, wrank;

            MPI_Comm_size(intracomm, &icsize);
            MPI_Comm_rank(intracomm, &icrank);
            MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

            if (icsize != rsize + size) {
                errs++;
                printf("Intracomm rank %d thinks size is %d, not %d\n",
                       icrank, icsize, rsize + size);
            }
            /* Make sure that the processes are ordered correctly */
            if (isChild) {
                int psize;
                MPI_Comm_remote_size(parentcomm, &psize);
                if (icrank != psize + wrank) {
                    errs++;
                    printf("Intracomm rank %d (from child) should have rank %d\n",
                           icrank, psize + wrank);
                }
            }
            else {
                if (icrank != wrank) {
                    errs++;
                    printf("Intracomm rank %d (from parent) should have rank %d\n", icrank, wrank);
                }
            }
        }

        /* At this point, try to form the intracommunicator, with the other
         * processes first */
        MPI_Intercomm_merge(intercomm, !isChild, &intracomm2);

        /* Check on the intra comm */
        {
            int icsize, icrank, wrank;

            MPI_Comm_size(intracomm2, &icsize);
            MPI_Comm_rank(intracomm2, &icrank);
            MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

            if (icsize != rsize + size) {
                errs++;
                printf("(2)Intracomm rank %d thinks size is %d, not %d\n",
                       icrank, icsize, rsize + size);
            }
            /* Make sure that the processes are ordered correctly */
            if (isChild) {
                if (icrank != wrank) {
                    errs++;
                    printf("(2)Intracomm rank %d (from child) should have rank %d\n",
                           icrank, wrank);
                }
            }
            else {
                int csize;
                MPI_Comm_remote_size(intercomm, &csize);
                if (icrank != wrank + csize) {
                    errs++;
                    printf("(2)Intracomm rank %d (from parent) should have rank %d\n",
                           icrank, wrank + csize);
                }
            }
        }

        /* At this point, try to form the intracommunicator, with an
         * arbitrary choice for the first group of processes */
        MPI_Intercomm_merge(intercomm, 0, &intracomm3);
        /* Check on the intra comm */
        {
            int icsize, icrank, wrank;

            MPI_Comm_size(intracomm3, &icsize);
            MPI_Comm_rank(intracomm3, &icrank);
            MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

            if (icsize != rsize + size) {
                errs++;
                printf("(3)Intracomm rank %d thinks size is %d, not %d\n",
                       icrank, icsize, rsize + size);
            }
            /* Eventually, we should test that the processes are ordered
             * correctly, by groups (must be one of the two cases above) */
        }

        /* Update error count */
        if (isChild) {
            /* Send the errs back to the master process */
            MPI_Ssend(&errs, 1, MPI_INT, 0, 1, intercomm);
        }
        else {
            if (rank == 0) {
                /* We could use intercomm reduce to get the errors from the
                 * children, but we'll use a simpler loop to make sure that
                 * we get valid data */
                for (i = 0; i < rsize; i++) {
                    MPI_Recv(&err, 1, MPI_INT, i, 1, intercomm, MPI_STATUS_IGNORE);
                    errs += err;
                }
            }
        }

        /* It isn't necessary to free the intracomms, but it should not hurt */
        MPI_Comm_free(&intracomm);
        MPI_Comm_free(&intracomm2);
        MPI_Comm_free(&intracomm3);

        /* It isn't necessary to free the intercomm, but it should not hurt */
        MPI_Comm_free(&intercomm);

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
