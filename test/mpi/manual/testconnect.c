/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Test from Edric Ellis
   To launch,

   Run multiple copies of "testconnect" should be run something like this:

   Path/to/testconnect /full/path/to/shared/file N

   where each instance has one process and N instances are run
*/

#include "mpi.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

char *fname;
int cachedRank = -1;
MPI_Comm comm;

void term_handler(int);
void segv_handler(int);

void term_handler(int sig)
{
    if (sig) {
        printf("Saw signal %d\n", sig);
    }
    printf("removing file: %s\n", fname);
    fflush(stdout);
    unlink(fname);
    if (sig != 0) {
        MPI_Abort(comm, 42);
        exit(0);
    }
}

void segv_handler(int sig)
{
    printf("SEGV detected!\n");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    MPI_Comm tmp;
    int size = 0;
    char portName[MPI_MAX_PORT_NAME];
    char *port = &portName[0];
    int doPrint = 1;
    int totalSize = 0;
    int myNum = -1;
    FILE *fh;

    if (argc < 4) {
        printf("Please call with a filename for the port\n");
        exit(1);
    }
    fname = argv[1];
    totalSize = atoi(argv[2]);
    if (argv[3])
        myNum = atoi(argv[3]);
    printf("[%d] Waiting for: %d\n", myNum, totalSize);

    MPI_Init(0, 0);

    /* the existance of the file is used to decide which processes
     * first do a connect to the root process.  */
    fh = fopen(fname, "rt");
    if (fh == NULL) {
        fh = fopen(fname, "wt");
        MPI_Open_port(MPI_INFO_NULL, portName);
        port = portName;
        fprintf(fh, "%s\n", portName);
        fclose(fh);

        if (doPrint) {
            printf("[%d] Wrote port %s to %s\n", myNum, port, fname);
            fflush(stdout);
        }
        comm = MPI_COMM_WORLD;
    }
    else {
        char *cerr;
        cerr = fgets(port, MPI_MAX_PORT_NAME, fh);
        fclose(fh);
        if (doPrint) {
            printf("[%d] about to connect: Port from %s is: %s\n", myNum, fname, port);
            fflush(stdout);
        }
        MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &tmp);
        if (doPrint) {
            printf("[%d] connect-side: about to perform intercomm merge\n", myNum);
            fflush(stdout);
        }
        MPI_Intercomm_merge(tmp, 1, &comm);
        if (doPrint) {
            printf("[%d] connect-side: intercomm merge complete\n", myNum);
            fflush(stdout);
        }
        MPI_Comm_free(&tmp);
    }

    MPI_Comm_size(comm, &size);

    while (size < totalSize) {
        MPI_Comm_accept(port, MPI_INFO_NULL, 0, comm, &tmp);
        if (doPrint) {
            printf("[%d] accept-side: about to perform intercomm merge\n", myNum);
            fflush(stdout);
        }
        MPI_Intercomm_merge(tmp, 0, &comm);
        if (doPrint) {
            printf("[%d] accept-side: about to perform intercomm merge\n", myNum);
            fflush(stdout);
        }
        MPI_Comm_rank(comm, &cachedRank);
        MPI_Comm_free(&tmp);
        MPI_Comm_size(comm, &size);
        if (doPrint) {
            printf("[%d] Size of comm is %d\n", myNum, size);
            fflush(stdout);
        }
    }

    printf("[%d] All done.\n", myNum);
    fflush(stdout);
    term_handler(0);

    MPI_Finalize();

    return 0;
}
