/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This is based on an example in the MPI standard and a bug report submitted
   by Alexandr Konovalov of Intel */

#include "mpi.h"
#include <stdio.h>

#define SIZE 100
#define ITER 100

int main(int argc, char* argv[])
{
    int i, j, k;
    static double a[SIZE][SIZE],b[SIZE][SIZE];
    double t1,t2,t,ts,tst;
    int myrank, mysize, errs = 0;
    MPI_Status status;
    MPI_Aint sizeofreal;

    MPI_Datatype row, xpose;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
    MPI_Comm_size( MPI_COMM_WORLD, &mysize );
    if (mysize != 2) {
	fprintf( stderr, "This test must be run with 2 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    MPI_Type_extent(MPI_DOUBLE, &sizeofreal);
 
    MPI_Type_vector(SIZE, 1, SIZE, MPI_DOUBLE, &row);
    MPI_Type_hvector(SIZE, 1, sizeofreal, row, &xpose);
    MPI_Type_commit(&xpose);

    /* Preset the arrays so that they're in memory */
    for (i=0; i<SIZE; i++)
	for (j=0; j<SIZE; j++) {
	    a[i][j]=0;
	    b[i][j]=0;
	    }
    a[SIZE-1][0] = 1;

    /* Time the transpose example */
    MPI_Barrier(MPI_COMM_WORLD);
    t1=MPI_Wtime();
    for(i=0;i< ITER; i++)
	{
	    if(myrank==0)
		MPI_Send(&a[0][0],1,xpose,1,0,MPI_COMM_WORLD);
	    else 
		MPI_Recv(&b[0][0],SIZE*SIZE,MPI_DOUBLE,0,0,MPI_COMM_WORLD,&status);
	}
    t2=MPI_Wtime();
    t=(t2-t1)/ITER;

    /* Time sending the same amount of data, but without the transpose */
    MPI_Barrier(MPI_COMM_WORLD);
    t1=MPI_Wtime();
    for(i=0; i< ITER; i++){
	if(myrank==0)
	    {
		MPI_Send(&a[0][0],sizeof(a),MPI_BYTE,1,0,MPI_COMM_WORLD);
	    }
	else {
		MPI_Recv(&b[0][0],sizeof(b),MPI_BYTE,0,0,MPI_COMM_WORLD,&status);
	}
    }
    t2=MPI_Wtime();
    ts=(t2-t1)/ITER;

    /* Time sending the same amount of data, with the transpose done
       as a separate step */
    MPI_Barrier(MPI_COMM_WORLD);
    t1=MPI_Wtime();
    for(k=0; k< ITER; k++){
	if(myrank==0)
	    {
		MPI_Send(&a[0][0],sizeof(a),MPI_BYTE,1,0,MPI_COMM_WORLD);
	    }
	else {
		MPI_Recv(&b[0][0],sizeof(b),MPI_BYTE,0,0,MPI_COMM_WORLD,&status);
		for(i=0;i<SIZE;i++)
		    for(j=0;j<SIZE;j++)
			b[i][j]=b[j][i]; 
	}
    }
    t2=MPI_Wtime();
    tst=(t2-t1)/ITER;

    /* Print out the results */
    if (myrank == 1) {
	/* if t and tst are too different, then there is a performance
	   problem in the handling of the datatypes */
	
	if (t > 2 * tst) {
	    errs ++;
	    fprintf( stderr, "Transpose time with datatypes is more than twice time without datatypes\n" );
	    fprintf( stderr, "%f\t%f\t%f\n", t, ts, tst );
	}
	if (errs) {
	    printf( " Found %d errors\n", errs );
	}
	else {
	    printf( " No Errors\n" );
	}
    }

    MPI_Type_free(&row);
    MPI_Type_free(&xpose);

    MPI_Finalize();
    return 0;
}
