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
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*
static char MTEST_Descrip[] = "A simple test of Comm_spawn";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI_Comm parentcomm, intercomm;
    MPI_Status status;
    int can_spawn;

    MTest_Init(&argc, &argv);

    errs += MTestSpawnPossible(&can_spawn);
    if (can_spawn) {
        MPI_Comm_get_parent(&parentcomm);

        if (parentcomm == MPI_COMM_NULL) {
            /* Create 2 more processes */
            MPI_Comm_spawn((char *) "./spawn1", MPI_ARGV_NULL, np,
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
            /* Child */
            char cname[MPI_MAX_OBJECT_NAME];
            int rlen;

            if (size != np) {
                errs++;
                printf("(Child) Did not create %d processes (got %d)\n", np, size);
            }
            /* Check the name of the parent */
            cname[0] = 0;
            MPI_Comm_get_name(intercomm, cname, &rlen);
            /* MPI-2 section 8.4 requires that the parent have this
             * default name */
            if (strcmp(cname, "MPI_COMM_PARENT") != 0) {
                errs++;
                printf("Name of parent is not correct\n");
                if (rlen > 0 && cname[0]) {
                    printf(" Got %s but expected MPI_COMM_PARENT\n", cname);
                }
                else {
                    printf(" Expected MPI_COMM_PARENT but no name set\n");
                }
            }
            MPI_Recv(&i, 1, MPI_INT, 0, 0, intercomm, &status);
            if (i != rank) {
                errs++;
                printf("Unexpected rank on child %d (%d)\n", rank, i);
            }
            /* Send the errs back to the master process */
            MPI_Ssend(&errs, 1, MPI_INT, 0, 1, intercomm);
        }

        /* It isn't necessary to free the intercomm, but it should not hurt */
        /* Using Comm_disconnect instead of free should provide a stronger
         * test, as a high-quality MPI implementation will be able to
         * recover some resources that it should hold on to in the case
         * of MPI_Comm_free */
        /*     MPI_Comm_free(&intercomm); */
        MPI_Comm_disconnect(&intercomm);

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
