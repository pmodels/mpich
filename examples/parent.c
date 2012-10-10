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
    int err=0, errcodes[256], rank, nprocs;
    MPI_Comm intercomm;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 

    if (nprocs != 4) {
        printf("Run this program with 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    err = MPI_Comm_spawn("child", MPI_ARGV_NULL, 4,
                         MPI_INFO_NULL, 1, MPI_COMM_WORLD,
                         &intercomm, errcodes);  
    if (err) printf("Error in MPI_Comm_spawn\n");

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 3) {
        err = MPI_Recv(str, 3, MPI_CHAR, 3, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Parent received from child: %s\n", str);
        fflush(stdout);
        
        err = MPI_Send("bye", 4, MPI_CHAR, 3, 0, intercomm); 
    }

    MPI_Finalize();

    return 0;
}

