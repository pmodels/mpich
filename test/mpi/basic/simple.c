/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Finalize();

    return 0;
}
