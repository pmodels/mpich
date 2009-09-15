/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define X 64
#define Y 8
#define Z 512

double array[X][Y][Z];

int main(int argc, char *argv[])
{
    int myrank;
    MPI_Datatype subarray;
    int array_size[] = {X, Y, Z};
    int array_subsize[] = {X/2, Y/2, Z};
    int array_start[] = {0, 0, 0};
 
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    MPI_Type_create_subarray(3, array_size, array_subsize, array_start, MPI_ORDER_C,
                             MPI_DOUBLE, &subarray);
    MPI_Type_commit(&subarray);

    if(myrank == 0)
        MPI_Send(array, 1, subarray, 1, 0, MPI_COMM_WORLD);
    else
        MPI_Recv(array, 1, subarray, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
    MPI_Finalize();

    return 0;
}
