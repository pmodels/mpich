/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
 * This test checks to see if we can create a simple datatype
 * made from many contiguous copies of a single struct.  The
 * struct is built with monotone decreasing displacements to
 * avoid any struct->contig optimizations.
 */

int main(int argc, char **argv)
{
    int blocklens[8], psize, i, rank;
    MPI_Aint displs[8];
    MPI_Datatype oldtypes[8];
    MPI_Datatype ntype1, ntype2;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i = 0; i < 8; i++) {
        blocklens[i] = 1;
        displs[i] = (7 - i) * sizeof(long);
        oldtypes[i] = MPI_LONG;
    }
    MPI_Type_struct(8, blocklens, displs, oldtypes, &ntype1);
    MPI_Type_contiguous(65536, ntype1, &ntype2);
    MPI_Type_commit(&ntype2);

    MPI_Pack_size(2, ntype2, MPI_COMM_WORLD, &psize);

    MPI_Type_free(&ntype2);
    MPI_Type_free(&ntype1);

    MTest_Finalize(0);
    return 0;
}
