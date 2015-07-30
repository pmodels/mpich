/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

void foo(void *sendbuf, MPI_Datatype sendtype, void *recvbuf, MPI_Datatype recvtype);
void foo(void *sendbuf, MPI_Datatype sendtype, void *recvbuf, MPI_Datatype recvtype)
{
    int blocks[2];
    MPI_Aint struct_displs[2];
    MPI_Datatype types[2], tmp_type;

    blocks[0] = 256;
    MPI_Get_address(sendbuf, &struct_displs[0]);
    types[0] = sendtype;
    blocks[1] = 256;
    MPI_Get_address(recvbuf, &struct_displs[1]);
    types[1] = MPI_BYTE;

    MPI_Type_create_struct(2, blocks, struct_displs, types, &tmp_type);
    MPI_Type_commit(&tmp_type);
    MPI_Type_free(&tmp_type);
}

int main(int argc, char **argv)
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    foo((void *) 0x1, MPI_FLOAT_INT, (void *) 0x2, MPI_BYTE);
    foo((void *) 0x1, MPI_DOUBLE_INT, (void *) 0x2, MPI_BYTE);
    foo((void *) 0x1, MPI_LONG_INT, (void *) 0x2, MPI_BYTE);
    foo((void *) 0x1, MPI_SHORT_INT, (void *) 0x2, MPI_BYTE);
    foo((void *) 0x1, MPI_2INT, (void *) 0x2, MPI_BYTE);
#ifdef HAVE_LONG_DOUBLE
    /* Optional type may be NULL */
    if (MPI_LONG_DOUBLE_INT != MPI_DATATYPE_NULL) {
        foo((void *) 0x1, MPI_LONG_DOUBLE_INT, (void *) 0x2, MPI_BYTE);
    }
#endif

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
