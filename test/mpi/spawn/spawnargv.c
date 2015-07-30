/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*
static char MTEST_Descrip[] = "A simple test of Comm_spawn, with complex arguments";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI_Comm parentcomm, intercomm;
    MPI_Status status;
    char *inargv[] =
        { (char *) "a", (char *) "b=c", (char *) "d e", (char *) "-pf", (char *) " Ss", 0 };
    char *outargv[] =
        { (char *) "a", (char *) "b=c", (char *) "d e", (char *) "-pf", (char *) " Ss", 0 };
    int can_spawn;

    MTest_Init(&argc, &argv);

    errs += MTestSpawnPossible(&can_spawn);

    if (can_spawn) {
        MPI_Comm_get_parent(&parentcomm);

        if (parentcomm == MPI_COMM_NULL) {
            /* Create 2 more processes */
            /* ./ is unix specific .
             * The more generic approach would be to specify "spawnargv" as the
             * executable and pass an info with ("path", ".") */
            MPI_Comm_spawn((char *) "./spawnargv", inargv, np,
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
        else {
            /* Child */
            /* FIXME: This assumes that stdout is handled for the children
             * (the error count will still be reported to the parent) */
            if (size != np) {
                errs++;
                printf("(Child) Did not create %d processes (got %d)\n", np, size);
            }
            MPI_Recv(&i, 1, MPI_INT, 0, 0, intercomm, &status);
            if (i != rank) {
                errs++;
                printf("Unexpected rank on child %d (%d)\n", rank, i);
            }
            /* Check the command line */
            for (i = 1; i < argc; i++) {
                if (!outargv[i - 1]) {
                    errs++;
                    printf("Wrong number of arguments (%d)\n", argc);
                    break;
                }
                if (strcmp(argv[i], outargv[i - 1]) != 0) {
                    errs++;
                    printf("Found arg %s but expected %s\n", argv[i], outargv[i - 1]);
                }
            }
            if (outargv[i - 1]) {
                /* We had too few args in the spawned command */
                errs++;
                printf("Too few arguments to spawned command\n");
            }
            /* Send the errs back to the master process */
            MPI_Ssend(&errs, 1, MPI_INT, 0, 1, intercomm);
        }

        /* It isn't necessary to free the intercomm, but it should not hurt */
        MPI_Comm_free(&intercomm);

        /* Note that the MTest_Finalize get errs only over COMM_WORLD */
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
