/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
    double t1, t2;
    int errs = 0;

    MTest_Init(&argc,&argv);

    t1 = MPI_Wtime();
    usleep(500);
    t2 = MPI_Wtime();

    if (t1 == t2)
        errs++;

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
