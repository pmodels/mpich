/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  Changes to this example
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This example is taken from MPI-The complete reference, Vol 1, 
 * pages 222-224.
 * 
 * Lines after the "--CUT HERE--" were added to make this into a complete 
 * test program.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 64

MPI_Datatype transpose_type(int m, int n, MPI_Datatype type);
MPI_Datatype submatrix_type(int N, int m, int n, MPI_Datatype type);
void Transpose(float *localA, float *localB, int M, int N, 
	       MPI_Comm comm)
/* transpose MxN matrix A that is block distributed on  
   processes of comm onto block distributed matrix B  */
{
  int i, j, extent, myrank, p, *scounts, *rcounts, n[2], m[2];
  int lasti, lastj;
  int sendcounts[MAX_SIZE], recvcounts[MAX_SIZE];
  int *sdispls, *rdispls;
  MPI_Datatype xtype[2][2], stype[2][2], *sendtypes, *recvtypes;

    printf( "M = %d, N = %d\n", M, N );

  /* compute parameters */
  MPI_Comm_size(comm, &p);
  MPI_Comm_rank(comm, &myrank);
  extent = sizeof(float);

  /* allocate arrays */
  scounts = (int *)malloc(p*sizeof(int));
  sdispls = (int *)malloc(p*sizeof(int));
  rcounts = (int *)malloc(p*sizeof(MPI_Aint));
  rdispls = (int *)malloc(p*sizeof(int));
  sendtypes = (MPI_Datatype *)malloc(p*sizeof(MPI_Datatype));
  recvtypes = (MPI_Datatype *)malloc(p*sizeof(MPI_Datatype));

  /* compute block sizes */
  m[0] = M/p;
  m[1] = M - (p-1)*(M/p);
  n[0] = N/p;
  n[1] = N - (p-1)*(N/p);

  /* compute types */
  for (i=0; i <= 1; i++)
    for (j=0; j <= 1; j++) {
      xtype[i][j] = transpose_type(m[i], n[j], MPI_FLOAT);
      stype[i][j] = submatrix_type(M, m[i], n[j], MPI_FLOAT);
    }

  /* prepare collective operation arguments */
  lasti = myrank == p-1;
  for (j=0;  j < p; j++) {
    lastj	  = j == p-1;
    sendcounts[j] = 1;
    sdispls[j]	  = j*n[0]*extent;
    sendtypes[j]  = xtype[lasti][lastj];
    recvcounts[j] = 1;
    rdispls[j]	  = j*m[0]*extent;
    recvtypes[j]  = stype[lastj][lasti];
  }
  
  /* communicate */
  /* -- Note that the book incorrectly uses &localA and &localB 
     as arguments to MPI_Alltoallw */
  MPI_Alltoallw(localA, sendcounts, sdispls, sendtypes, 
                localB, recvcounts, rdispls, recvtypes, comm);
}


MPI_Datatype submatrix_type(int N, int m, int n, MPI_Datatype type)
/* computes a datatype for an mxn submatrix within an MxN matrix 
   with entries of type type */
{
  MPI_Datatype subrow, submatrix;
  
  MPI_Type_contiguous(n, type, &subrow);
  MPI_Type_vector(m, 1, N, subrow, &submatrix); 
  MPI_Type_commit(&submatrix);
  return(submatrix);
}


MPI_Datatype transpose_type(int m, int n, MPI_Datatype type)
/* computes a datatype for the transpose of an mxn matrix 
   with entries of type type */
{
  MPI_Datatype subrow, subrow1, submatrix;
  MPI_Aint lb, extent;
  
  MPI_Type_vector(m, 1, n, type, &subrow);
  MPI_Type_get_extent(type, &lb, &extent);
  MPI_Type_create_resized(subrow, 0, extent, &subrow1);
  MPI_Type_contiguous(n, subrow1, &submatrix); 
  MPI_Type_commit(&submatrix);
  return(submatrix);
}

/* -- CUT HERE -- */

int main( int argc, char *argv[] )
{
    int gM, gN, lm, lmlast, ln, lnlast, i, j, errs = 0;
    int size, rank;
    float *localA, *localB;
    MPI_Comm comm;

    MTest_Init( &argc, &argv );
    comm = MPI_COMM_WORLD;
    
    MPI_Comm_size( comm, &size );
    MPI_Comm_rank( comm, &rank );

    gM = 20;
    gN = 30;

    /* Each block is lm x ln in size, except for the last process, 
       which has lmlast x lnlast */
    lm     = gM/size;
    lmlast = gM - (size - 1)*lm;
    ln     = gN/size;
    lnlast = gN - (size - 1)*ln;

    /* Create the local matrices */
    if (rank == size - 1) {
	localA = (float *)malloc( gN * lmlast * sizeof(float) );
	localB = (float *)malloc( gM * lnlast * sizeof(float) );
	for (i=0; i<lmlast; i++) {
	    for (j=0; j<gN; j++) {
		localA[i*gN+j] = (float)(i*gN+j + rank * gN * lm);
	    }
	}
	
    }
    else {
	localA = (float *)malloc( gN * lm * sizeof(float) );
	localB = (float *)malloc( gM * ln * sizeof(float) );
	for (i=0; i<lm; i++) {
	    for (j=0; j<gN; j++) {
		localA[i*gN+j] = (float)(i*gN+j + rank * gN * lm);
	    }
	}
    }

    printf( "Allocated local arrays\n" );
    /* Transpose */
    Transpose( localA, localB, gM, gN, comm );

    /* check the transposed matrix */


    MTest_Finalize( errs );

    return 0;
}
