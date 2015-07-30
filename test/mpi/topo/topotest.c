/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;
    int topo_type, size, dims[1], periods[1];
    MPI_Comm comm;

    MTest_Init(&argc, &argv);

    /* Check that topo test returns the correct type, including
     * MPI_UNDEFINED */

    MPI_Topo_test(MPI_COMM_WORLD, &topo_type);
    if (topo_type != MPI_UNDEFINED) {
        errs++;
        printf("Topo type of comm world is not UNDEFINED\n");
    }

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    dims[0] = size;
    periods[0] = 0;
    MPI_Cart_create(MPI_COMM_WORLD, 1, dims, periods, 0, &comm);
    MPI_Topo_test(comm, &topo_type);
    if (topo_type != MPI_CART) {
        errs++;
        printf("Topo type of cart comm is not CART\n");
    }

    MPI_Comm_free(&comm);
    /* FIXME: still need graph example */

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
