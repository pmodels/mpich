/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpi.h"

int main(int argc, char **argv)
{
    int rc, flag, vval, size;
    void *v;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    rc = MPI_Attr_get(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &v, &flag);
    if (rc) {
        fprintf(stderr, "MPI_UNIVERSE_SIZE missing\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    else {
        /* MPI_UNIVERSE_SIZE need not be set */
        if (flag) {
            vval = *(int *) v;
            if (vval < 5) {
                fprintf(stderr, "MPI_UNIVERSE_SIZE = %d, less than expected (%d)\n", vval,
                        size);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
    }

    MPI_Finalize();

    printf(" No errors\n");

    return 0;
}
