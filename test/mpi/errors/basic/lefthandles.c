/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Comm comm;
    MPI_Datatype ntype;
    int i;

    MPI_Init(&argc, &argv);

    for (i = 0; i < 20; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    }

    MPI_Type_contiguous(27, MPI_INT, &ntype);
    MPI_Type_contiguous(27, MPI_INT, &ntype);
    MPI_Type_contiguous(27, MPI_INT, &ntype);
    MPI_Type_contiguous(27, MPI_INT, &ntype);
    MPI_Type_contiguous(27, MPI_INT, &ntype);
    MPI_Finalize();

    return 0;
}
