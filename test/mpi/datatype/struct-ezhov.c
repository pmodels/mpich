/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <string.h>

#define COUNT		14
#define SIZE		340
#define EL_COUNT	1131

char s_buf[EL_COUNT*SIZE];
char r_buf[EL_COUNT*SIZE];

int main( int argc, char **argv )
{
    int 		rank, size, ret; 
    MPI_Status 		Status;
    MPI_Request		request;
    MPI_Datatype 	struct_type, type1[COUNT];
    MPI_Aint 		disp1[COUNT] = {0, 0, 332, 340};
    int			block1[COUNT] = {1, 56, 2, 1};

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    type1[0] = MPI_LB;
    type1[1] = MPI_FLOAT;
    type1[2] = MPI_FLOAT;
    type1[3] = MPI_UB;
    
    MPI_Type_struct(4, block1, disp1, type1, &struct_type);
    
    ret = MPI_Type_commit(&struct_type);
    if (ret != MPI_SUCCESS) 
    {
        fprintf(stderr, "Could not make struct type."), fflush(stderr); 
        MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    
    memset(s_buf, 0, EL_COUNT*SIZE);
    memset(r_buf, 0, EL_COUNT*SIZE);

    MPI_Isend(s_buf, EL_COUNT, struct_type, 0, 4, MPI_COMM_WORLD, &request);
    MPI_Recv(r_buf, EL_COUNT, struct_type, 0, 4, MPI_COMM_WORLD, &Status );
    MPI_Wait(&request, &Status);
    
    MPI_Type_free(&struct_type);
    
    MPI_Finalize();

    printf(" No Errors\n");

    return 0;
}
