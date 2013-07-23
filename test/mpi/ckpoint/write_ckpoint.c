/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>

int main(int argc,char *argv[])
{
    int numprocs, myid, i;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    MPI_Get_processor_name(processor_name,&namelen);

    for ( i = 0 ; i < 22; i++ ) {
        MPI_Barrier(MPI_COMM_WORLD);
        MTestSleep(1);
    }

    if (myid == 0) {
        printf("No Errors\n");
    }

    MPI_Finalize();
    return 0;
}
