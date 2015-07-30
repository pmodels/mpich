/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
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
static char MTEST_Descrip[] = "A simple test of Comm_spawn, with many arguments";
*/

#define MAX_ARGV 1024
int worker(int argc, char *argv[], MPI_Comm intercomm, char *outargv[], int np);

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI_Comm parentcomm, intercomm;
    char *inargv[MAX_ARGV];
    char *outargv[MAX_ARGV];
    int narg = 40;
    char *saveArgp = 0;
    int can_spawn;

    MTest_Init(&argc, &argv);

    errs += MTestSpawnPossible(&can_spawn);

    if (can_spawn) {
        /* Initialize the argument vectors */
        for (i = 0; i < MAX_ARGV; i++) {
            char *p;
            p = (char *) malloc(9);
            if (!p) {
                fprintf(stderr, "Unable to allocated memory\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            strcpy(p, "01234567");
            inargv[i] = p;
            p = (char *) malloc(9);
            if (!p) {
                fprintf(stderr, "Unable to allocated memory\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            strcpy(p, "01234567");
            outargv[i] = p;
        }

        MPI_Comm_get_parent(&parentcomm);

        if (parentcomm == MPI_COMM_NULL) {
            for (narg = 16; narg < 512; narg += narg) {
                /* Create 2 more processes */
                /* Set a null at argument length narg */
                saveArgp = inargv[narg];
                inargv[narg] = 0;
                /* ./ is unix specific .
                 * The more generic approach would be to specify "spawnmanyarg" as
                 * the executable and pass an info with ("path", ".") */
                MPI_Comm_spawn((char *) "./spawnmanyarg", inargv, np,
                               MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes);
                inargv[narg] = saveArgp;
                /* We now have a valid intercomm */

                /* Master */
                MPI_Comm_remote_size(intercomm, &rsize);
                MPI_Comm_size(intercomm, &size);
                MPI_Comm_rank(intercomm, &rank);

                if (rsize != np) {
                    errs++;
                    printf("Did not create %d processes (got %d)\n", np, rsize);
                }
                /* Send the expected rank */
                for (i = 0; i < rsize; i++) {
                    MPI_Send(&i, 1, MPI_INT, i, 0, intercomm);
                }
                /* Send the number of arguments */
                for (i = 0; i < rsize; i++) {
                    MPI_Send(&narg, 1, MPI_INT, i, 0, intercomm);
                }
                /* We could use intercomm reduce to get the errors from the
                 * children, but we'll use a simpler loop to make sure that
                 * we get valid data */
                for (i = 0; i < rsize; i++) {
                    MPI_Recv(&err, 1, MPI_INT, i, 1, intercomm, MPI_STATUS_IGNORE);
                    errs += err;
                }

                /* Free the intercomm before the next round of spawns */
                MPI_Comm_free(&intercomm);
            }
        }
        else {
            /* Note that worker also send errs to the parent */
            errs += worker(argc, argv, parentcomm, outargv, np);
            MPI_Comm_free(&parentcomm);
            MPI_Finalize();
            return 0;
        }

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

/* Call this routine if this process is the spawned child */
int worker(int argc, char *argv[], MPI_Comm intercomm, char *outargv[], int np)
{
    int i, narg;
    int rsize, size, rank;
    int errs = 0;
    char *saveoutArgp = 0;
    MPI_Status status;

    MPI_Comm_remote_size(intercomm, &rsize);
    MPI_Comm_size(intercomm, &size);
    MPI_Comm_rank(intercomm, &rank);

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
    MPI_Recv(&narg, 1, MPI_INT, 0, 0, intercomm, MPI_STATUS_IGNORE);
    saveoutArgp = outargv[narg];
    outargv[narg] = 0;
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
        printf("Too few arguments to spawned command (only %d)\n", i);
    }
    /* Restore the argument vector (not necessary in this case, since the
     * worker will exit) */
    outargv[narg] = saveoutArgp;
    /* Send the errs back to the master process */
    MPI_Ssend(&errs, 1, MPI_INT, 0, 1, intercomm);

    return errs;
}
