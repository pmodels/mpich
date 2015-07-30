/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "mpitest.h"

#include "connectstuff.h"

int main(int argc, char **argv)
{
    MPI_Comm tmp, comm, startComm;
    char *fname;
    char *actualFname = NULL;
    char *globalFname = NULL;
    int totalSize, expectedRank, size, cachedRank;
    char portName[MPI_MAX_PORT_NAME];
    int rankToAccept = 1;

    /* Debug - print out where we picked up the MPICH build from */
#ifdef MPICHLIBSTR
    msg("MPICH library taken from: %s\n", MPICHLIBSTR);
#endif

    if (argc != 4) {
        printf("Usage: %s <fname> <totalSize> <idx-1-based>\n", argv[0]);
        exit(1);
    }

    /* This is the base name of the file into which we write the port */
    fname = argv[1];
    /* This is the total number of processes launched */
    totalSize = atoi(argv[2]);
    /* Each process knows its expected rank */
    expectedRank = atoi(argv[3]) - 1;

    /* Start a watchdog thread which will abort after 120 seconds, and will
     * print stack traces using GDB every 5 seconds if you don't call
     * strokeWatchdog() */
    startWatchdog(120);

    /* Print a debug header */
    msg("Waiting for: %d - my rank is %d\n", totalSize, expectedRank);

    /* Singleton init */
    MPI_Init(0, 0);

    /* Duplicate from MPI_COMM_SELF the starting point */
    MPI_Comm_dup(MPI_COMM_SELF, &startComm);


    if (expectedRank == 0) {
        /* This process opens the port, and writes the information to the file */
        MPI_Open_port(MPI_INFO_NULL, portName);

        /* Write the port to fname.<rank> so that the connecting processes can
         * wait their turn by checking for the correct file to show up */
        actualFname = writePortToFile(portName, "%s.%d", fname, rankToAccept++);

        /* The wrapper script I'm using checks for the existance of "fname", so
         * create that - even though it isn't used  */
        globalFname = writePortToFile(portName, fname);
        installExitHandler(globalFname);

        comm = startComm;
    }
    else {
        char *readPort;
        readPort = getPortFromFile("%s.%d", fname, expectedRank);
        strncpy(portName, readPort, MPI_MAX_PORT_NAME);
        free(readPort);
        msg("Read port <%s>\n", portName);

        MPI_Comm_connect(portName, MPI_INFO_NULL, 0, startComm, &comm);
        MPI_Intercomm_merge(comm, 1, &tmp);
        comm = tmp;
        MPI_Comm_size(comm, &size);
        msg("After my first merge, size is now: %d\n", size);
    }
    while (size < totalSize) {
        /* Make sure we don't print a stack until we stall */
        strokeWatchdog();

        /* Accept the connection */
        MPI_Comm_accept(portName, MPI_INFO_NULL, 0, comm, &tmp);

        /* Merge into intracomm */
        MPI_Intercomm_merge(tmp, 0, &comm);

        /* Free the intercomm */
        MPI_Comm_free(&tmp);

        /* See where we're up to */
        MPI_Comm_rank(comm, &cachedRank);
        MPI_Comm_size(comm, &size);

        if (expectedRank == 0) {
            msg("Up to size: %d\n", size);

            /* Delete the old file, create the new one */
            unlink(actualFname);
            free(actualFname);

            /* Allow the next rank to connect */
            actualFname = writePortToFile(portName, "%s.%d", fname, rankToAccept++);
        }
    }
    MPI_Comm_rank(comm, &cachedRank);

    msg("All done - I got rank: %d.\n", cachedRank);

    MPI_Barrier(comm);

    if (expectedRank == 0) {

        /* Cleanup on rank zero - delete some files */
        MTestSleep(4);
        unlink(actualFname);
        free(actualFname);
        unlink(globalFname);
        free(globalFname);

        /* This lets my wrapper script know that we did everything correctly */
        indicateConnectSucceeded();
    }
    MPI_Finalize();

    return 0;
}
