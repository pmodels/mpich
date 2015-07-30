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

/* #include <fcntl.h> */

/* This test tests the ability to handle multiple simultaneous calls to
 * MPI_Comm_spawn.
 * The first process spawns MAX_NUM_SPAWNS processes and tells them to spawn
 * MAX_NUM_SPAWNS -1 processes.
 * This repeats until no processes are spawned.  For MAX_NUM_SPAWNS = 4 this
 * results in 64 processes
 * created.  Hopefully this will result in concurrent handling of
 * MPI_Comm_spawn calls from multiple processes.
 */

#define IF_VERBOSE(a) if (verbose) { printf a ; fflush(stdout); }

/*
static char MTEST_Descrip[] = "A test of concurrent MPI_Comm_spawn calls";
*/

#define MAX_NUM_SPAWNS 4

int main(int argc, char *argv[])
{
    int errs = 0;
    int child_errs = 0;
    int size, rsize, i, num_spawns = 0;
    char description[100];
    char child_spawns[10];
    char *argv1[3] = { child_spawns, description, NULL };
    MPI_Comm parentcomm, intercomm[MAX_NUM_SPAWNS];
    MPI_Status status;
    int verbose = 0;
    char *env;
    int can_spawn;

    env = getenv("MPITEST_VERBOSE");
    if (env) {
        if (*env != '0')
            verbose = 1;
    }

    MTest_Init(&argc, &argv);

    errs += MTestSpawnPossible(&can_spawn);

    if (can_spawn) {
        /* Set the num_spawns for the first process to MAX_NUM_SPAWNS */
        MPI_Comm_get_parent(&parentcomm);
        if (parentcomm == MPI_COMM_NULL) {
            num_spawns = MAX_NUM_SPAWNS;
        }

        /* If an argument is passed in use it for num_spawns */
        /* This is the case for all spawned processes and optionally the first
         * process as well */
        if (argc > 1) {
            num_spawns = atoi(argv[1]);
            if (num_spawns < 0)
                num_spawns = 0;
            if (num_spawns > MAX_NUM_SPAWNS)
                num_spawns = MAX_NUM_SPAWNS;
        }

        /* Send num_spawns - 1 on the command line to the spawned children */
        sprintf(child_spawns, "%d", num_spawns - 1 > 0 ? num_spawns - 1 : 0);

        /* Spawn the children */
        IF_VERBOSE(("spawning %d\n", num_spawns));
        for (i = 0; i < num_spawns; i++) {
            if (argc > 2) {
                sprintf(description, "%s:%d", argv[2], i);
            }
            else {
                sprintf(description, "%d", i);
            }
            IF_VERBOSE(("spawning %s\n", description));
            MPI_Comm_spawn((char *) "./concurrent_spawns", argv1, 1,
                           MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm[i], MPI_ERRCODES_IGNORE);

            MPI_Comm_remote_size(intercomm[i], &rsize);
            MPI_Comm_size(intercomm[i], &size);
            if (rsize != 1) {
                errs++;
                printf("Did not create 1 process (got %d)\n", rsize);
                fflush(stdout);
            }
        }

        /* Receive the error count from each of your children and add it to your
         * error count */
        for (i = 0; i < num_spawns; i++) {
            MPI_Recv(&child_errs, 1, MPI_INT, 0, 0, intercomm[i], &status);
            errs += child_errs;
            MPI_Comm_disconnect(&intercomm[i]);
        }

        /* If you are a spawned process send your errors to your parent */
        if (parentcomm != MPI_COMM_NULL) {
            MPI_Send(&errs, 1, MPI_INT, 0, 0, parentcomm);
            MPI_Comm_disconnect(&parentcomm);
        }
        else {
            /* Note that the MTest_Finalize get errs only over COMM_WORLD */
            /* Note also that both the parent and child will generate "No Errors"
             * if both call MTest_Finalize */
            MTest_Finalize(errs);
        }
    }
    else {
        MTest_Finalize(errs);
    }

    IF_VERBOSE(("calling finalize\n"));
    MPI_Finalize();
    return 0;
}
