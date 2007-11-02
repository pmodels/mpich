/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h" 
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* This does a transpose with a get operation, fence, and derived
   datatypes. Uses vector and hvector (Example 3.32 from MPI 1.1
   Standard). Run on 2 processes */

#define NROWS 1000
#define NCOLS 1000

int main(int argc, char *argv[]) 
{ 
    int rank, nprocs, **A, *A_data, i, j;
    MPI_Win win;
    MPI_Datatype column, xpose;
    int errs = 0;
 
    MTest_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

    if (nprocs != 2) {
        printf("Run this program with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    A_data = (int *) malloc(NROWS * NCOLS * sizeof(int));
    A = (int **) malloc(NROWS * sizeof(int *));

    A[0] = A_data;
    for (i=1; i<NROWS; i++)
	A[i] = A[i-1] + NCOLS;

    if (rank == 0)
    {
        for (i=0; i<NROWS; i++)
            for (j=0; j<NCOLS; j++)
                A[i][j] = -1;

        /* create datatype for one column */
        MPI_Type_vector(NROWS, 1, NCOLS, MPI_INT, &column);
        /* create datatype for matrix in column-major order */
        MPI_Type_hvector(NCOLS, 1, sizeof(int), column, &xpose);
        MPI_Type_commit(&xpose);
 
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win); 

        MPI_Win_fence(0, win); 

        MPI_Get(&A[0][0], NROWS*NCOLS, MPI_INT, 1, 0, 1, xpose, win);
    
        MPI_Type_free(&column);
        MPI_Type_free(&xpose);

        MPI_Win_fence(0, win); 

        for (j=0; j<NCOLS; j++)
	{
            for (i=0; i<NROWS; i++)
	    {
                if (A[j][i] != i*NCOLS + j)
		{
		    if (errs < 50)
		    {
			printf("Error: A[%d][%d]=%d should be %d\n", j, i,
			    A[j][i], i*NCOLS + j);
		    }
		    errs++;
		}
	    }
	}
	if (errs >= 50)
	{
	    printf("Total number of errors: %d\n", errs);
	}
    }        
    else
    {  /* rank = 1 */
        for (i=0; i<NROWS; i++) 
            for (j=0; j<NCOLS; j++)
                A[i][j] = i*NCOLS + j;
        MPI_Win_create(&A[0][0], NROWS*NCOLS*sizeof(int), sizeof(int), MPI_INFO_NULL, 
                       MPI_COMM_WORLD, &win); 
        MPI_Win_fence(0, win); 

        MPI_Win_fence(0, win); 
    }

    MPI_Win_free(&win); 
    MTest_Finalize(errs);
    MPI_Finalize(); 
    return 0; 
} 
