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

/*
static char MTEST_Descrip[] = "Test survivability from faults with collective communication";
*/

int main(int argc, char *argv[])
{
    int wrank, wsize, rank, size, color;
    int tmp;
    MPI_Comm newcomm;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* Color is 0 or 1; 1 will be the processes that "fault" */
    color = (wrank > 0) && (wrank <= wsize / 2);
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &newcomm);

    MPI_Barrier(MPI_COMM_WORLD);
    if (color) {
        /* Simulate a fault on some processes */
        exit(1);
    }

    /* Can we still use newcomm? */
    MPI_Comm_size(newcomm, &size);
    MPI_Comm_rank(newcomm, &rank);

    MPI_Allreduce(&rank, &tmp, 1, MPI_INT, MPI_SUM, newcomm);
    if (tmp != (size * (size + 1)) / 2) {
        printf("Allreduce gave %d but expected %d\n", tmp, (size * (size + 1)) / 2);
    }

    MPI_Comm_free(&newcomm);
    MPI_Finalize();

    printf(" No Errors\n");

    return 0;
}
