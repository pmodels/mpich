/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */     

#include <mpi.h>
#include <stdio.h>
int main(int argc, char* argv[])
{
	int iam, np;
	int m = 2, n = 0, lda = 1;
	double A[2];
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Datatype type = MPI_DOUBLE, vtype;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(comm, &np);
	MPI_Comm_rank(comm, &iam);
	if (np < 2) {
		printf( "Should be at least 2 processes for the test\n");
        } else {
		MPI_Type_vector(n, m, lda, type, &vtype);
		MPI_Type_commit(&vtype);
		A[0] = -1.0-0.1*iam;
		A[1] = 0.5+0.1*iam;
		printf("In process %i of %i before Bcast: A = %f,%f\n",
		       iam, np, A[0], A[1] );
		MPI_Bcast(A, 1, vtype, 0, comm);
		printf("In process %i of %i after Bcast: A = %f,%f\n",
		       iam, np, A[0], A[1]);
		MPI_Type_free(&vtype);
	}

	MPI_Finalize();
}
