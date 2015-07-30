/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * This tests spawn_mult by using the same executable and no command-line
 * options.  The attribute MPI_APPNUM is used to determine which
 * executable is running.
 */

int main(int argc, char *argv[])
{
    MPI_Comm parentcomm, intercomm;
    int i, size, rsize, rank;
    int err, errs = 0;
    MPI_Status status;
    int flag;
    static int np[2] = { 1, 1 };
    int *appnum_ptr;
    int can_spawn;

    MTest_Init(&argc, &argv);

    MTestSpawnPossible(&can_spawn);

    if (can_spawn) {
        MPI_Comm_get_parent(&parentcomm);
        if (parentcomm == MPI_COMM_NULL) {
            /* Create 2 more processes */
            static char *cmds[2] = { (char *) "./spawnmult2", (char *) "./spawnmult2" };
            static MPI_Info infos[2] = { MPI_INFO_NULL, MPI_INFO_NULL };
            int errcodes[2];

            MPI_Comm_spawn_multiple(2, cmds, MPI_ARGVS_NULL, np, infos, 0,
                                    MPI_COMM_WORLD, &intercomm, errcodes);

        }
        else {
            intercomm = parentcomm;
        }

        /* We now have a valid intercomm */
        MPI_Comm_remote_size(intercomm, &rsize);
        MPI_Comm_size(intercomm, &size);
        MPI_Comm_rank(intercomm, &rank);

        if (parentcomm == MPI_COMM_NULL) {
            /* This is the master process */
            if (rsize != np[0] + np[1]) {
                errs++;
                printf("Did not create %d processes (got %d)\n", np[0] + np[1], rsize);
            }
            if (rank == 0) {
                /* Tell each child process what rank we think they are */
                for (i = 0; i < rsize; i++) {
                    MPI_Send(&i, 1, MPI_INT, i, 0, intercomm);
                }
                /* We could use intercomm reduce to get the errors from the
                 * children, but we'll use a simpler loop to make sure that
                 * we get valid data */
                for (i = 0; i < rsize; i++) {
                    MPI_Recv(&err, 1, MPI_INT, i, 1, intercomm, MPI_STATUS_IGNORE);
                    errs += err;
                }
            }
        }
        else {
            /* Child process */
            /* FIXME: This assumes that stdout is handled for the children
             * (the error count will still be reported to the parent) */
            if (size != 2) {
                int wsize;
                errs++;
                printf("(Child) Did not create 2 processes (got %d)\n", size);
                MPI_Comm_size(MPI_COMM_WORLD, &wsize);
                if (wsize == 2) {
                    errs++;
                    printf("(Child) world size is 2 but local intercomm size is not 2\n");
                }
            }

            MPI_Recv(&i, 1, MPI_INT, 0, 0, intercomm, &status);
            if (i != rank) {
                errs++;
                printf("Unexpected rank on child %d (%d)\n", rank, i);
            }

            /* Check for correct APPNUM */
            MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_APPNUM, &appnum_ptr, &flag);
            /* My appnum should be my rank in comm world */
            if (flag) {
                if (*appnum_ptr != rank) {
                    errs++;
                    printf("appnum is %d but should be %d\n", *appnum_ptr, rank);
                }
            }
            else {
                errs++;
                printf("appnum was not set\n");
            }

            /* Send the errs back to the master process */
            MPI_Ssend(&errs, 1, MPI_INT, 0, 1, intercomm);
        }

        /* It isn't necessary to free the intercomm, but it should not hurt */
        MPI_Comm_free(&intercomm);

        /* Note that the MTest_Finalize get errs only over COMM_WORLD  */
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
