/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    char str[10];
    int err=0, errcodes[256], rank, rank1, nprocs;
    MPI_Comm intercomm1, intercomm2, intracomm;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank1); 

    if (nprocs != 2) {
        printf("Run this program with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    if (rank1 == 0) {
        printf("Parents spawning 2 children...\n");
        fflush(stdout);
    }

    err = MPI_Comm_spawn("spawn_merge_child1", MPI_ARGV_NULL, 2,
                         MPI_INFO_NULL, 1, MPI_COMM_WORLD,
                         &intercomm1, errcodes);  
    if (err) printf("Error in MPI_Comm_spawn\n");

    if (rank1 == 0) {
        printf("Parents and children merging to form new intracommunicator...\n");
        fflush(stdout);
    }

    MPI_Intercomm_merge(intercomm1, 0, &intracomm);

    if (rank1 == 0) {
        printf("Merged parents spawning 2 more children and communicating with them...\n");
        fflush(stdout);
    }

    err = MPI_Comm_spawn("spawn_merge_child2", MPI_ARGV_NULL, 2,
                         MPI_INFO_NULL, 2, intracomm,
                         &intercomm2, errcodes);  
    if (err) printf("Error in MPI_Comm_spawn\n");

    MPI_Comm_rank(intercomm2, &rank);

    if (rank == 2) {
        err = MPI_Recv(str, 3, MPI_CHAR, 1, 0, intercomm2, MPI_STATUS_IGNORE);
        printf("Parent received from child: %s\n", str);
        fflush(stdout);
        
        err = MPI_Send("bye", 4, MPI_CHAR, 1, 0, intercomm2); 
    }

    MPI_Finalize();

    return 0;
}

