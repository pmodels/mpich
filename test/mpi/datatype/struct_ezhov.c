/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <string.h>
#include "mpitest.h"

#define COUNT		2
#define SIZE		340
#define EL_COUNT	1131

char s_buf[EL_COUNT * SIZE];
char r_buf[EL_COUNT * SIZE];

int main(int argc, char **argv)
{
    int rank, size, ret;
    MPI_Status Status;
    MPI_Request request;
    MPI_Datatype tmp_type, struct_type, type1[COUNT];
    MPI_Aint disp1[COUNT] = { 0, 332 };
    int block1[COUNT] = { 56, 2 };

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    type1[0] = MPI_FLOAT;
    type1[1] = MPI_FLOAT;

    MPI_Type_create_struct(2, block1, disp1, type1, &tmp_type);
    MPI_Type_create_resized(tmp_type, 0, 340, &struct_type);
    MPI_Type_free(&tmp_type);

    ret = MPI_Type_commit(&struct_type);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Could not make struct type."), fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    memset(s_buf, 0, EL_COUNT * SIZE);
    memset(r_buf, 0, EL_COUNT * SIZE);

    MPI_Isend(s_buf, EL_COUNT, struct_type, 0, 4, MPI_COMM_WORLD, &request);
    MPI_Recv(r_buf, EL_COUNT, struct_type, 0, 4, MPI_COMM_WORLD, &Status);
    MPI_Wait(&request, &Status);

    MPI_Type_free(&struct_type);

    MTest_Finalize(0);

    return 0;
}
