/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2005 by Argonne National Laboratory.
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
#ifdef HAVE_UNISTD_H
/* Needed for getcwd */
#include <unistd.h>
#endif

/*
static char MTEST_Descrip[] = "A simple test of Comm_spawn with info";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, rsize, i;
    int np = 2;
    int errcodes[2];
    MPI_Comm parentcomm, intercomm;
    MPI_Status status;
    MPI_Info spawninfo;
    char curdir[1024], wd[1024], childwd[1024];
    char *cerr;
    int can_spawn;

    MTest_Init(&argc, &argv);

    errs += MTestSpawnPossible(&can_spawn);

    if (can_spawn) {
        cerr = getcwd(curdir, sizeof(curdir));

        MPI_Comm_get_parent(&parentcomm);

        if (parentcomm == MPI_COMM_NULL) {
            char *p;
            /* Create 2 more processes.  Make the working directory the
             * directory above the current running directory */
            strncpy(wd, curdir, sizeof(wd));
            /* Lop off the last element of the directory */
            p = wd + strlen(wd) - 1;
            while (p > wd && *p != '/' && *p != '\\')
                p--;
            *p = 0;

            MPI_Info_create(&spawninfo);
            MPI_Info_set(spawninfo, (char *) "path", curdir);
            MPI_Info_set(spawninfo, (char *) "wdir", wd);
            MPI_Comm_spawn((char *) "spawninfo1", MPI_ARGV_NULL, np,
                           spawninfo, 0, MPI_COMM_WORLD, &intercomm, errcodes);
            MPI_Info_free(&spawninfo);
        } else
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
                for (i = 0; i < rsize; i++) {
                    MPI_Recv(childwd, sizeof(childwd), MPI_CHAR, i, 2, intercomm,
                             MPI_STATUS_IGNORE);
                    if (strcmp(childwd, wd) != 0) {
                        printf("Expected a working dir of %s but child is in %s\n", wd, childwd);
                        errs++;
                    }
                }
            }
        } else {
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
                } else {
                    printf(" Expected MPI_COMM_PARENT but no name set\n");
                }
            }
            MPI_Recv(&i, 1, MPI_INT, 0, 0, intercomm, &status);
            if (i != rank) {
                errs++;
                printf("Unexpected rank on child %d (%d)\n", rank, i);
            }
            /* Send our notion of the current directory to the parent */
            MPI_Send(curdir, strlen(curdir) + 1, MPI_CHAR, 0, 2, intercomm);

            /* Send the errs back to the master process */
            MPI_Ssend(&errs, 1, MPI_INT, 0, 1, intercomm);
        }

        /* It isn't necessary to free the intercomm, but it should not hurt */
        MPI_Comm_free(&intercomm);

        /* Note that the MTest_Finalize get errs only over COMM_WORLD */
        /* Note also that both the parent and child will generate "No Errors"
         * if both call MTest_Finalize */
        if (parentcomm == MPI_COMM_NULL) {
            MTest_Finalize(errs);
        } else {
            MPI_Finalize();
        }
    } else {
        MTest_Finalize(errs);
    }

    return MTestReturnValue(errs);
}
