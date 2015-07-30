/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    int numprocs, myid, i;
    int namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    struct stat fileStat;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Get_processor_name(processor_name, &namelen);

    for (i = 0; i < 22; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        MTestSleep(1);
    }

    if (myid == 0) {
        if (stat("/tmp/context-num2-0-0", &fileStat) < 0) {
            printf("failed to find ckpoint file\n");
        }
        else if (fileStat.st_size == 0) {
            printf("ckpoint file is empty\n");
        }
        else {
            printf("No Errors\n");
        }
    }

    MPI_Finalize();
    return 0;
}
