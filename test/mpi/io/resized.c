/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Test file views with MPI_Type_create_resized";

/* This doesn't test correctness yet. It is the smallest version that fails. */

int main(int argc, char **argv)
{
    int i, nprocs, len, mpi_errno;
    int errs=1; 
    MPI_File fh;
    char *filename;
    MPI_Datatype newtype;

    MTest_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs != 1) {
        fprintf(stderr, "Run this program on 1 process\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    i = 1;
    while ((i < argc) && strcmp("-fname", *argv)) {
	i++;
	argv++;
    }
    if (i >= argc) {
	len      = 8;
	filename = (char *)malloc(len + 10);
	strcpy( filename, "testfile" );
	/*
	  fprintf(stderr, "\n*#  Usage: resized -fname filename\n\n");
	  MPI_Abort(MPI_COMM_WORLD, 1);
	*/
    }
    else {
	argv++;
	len = (int)strlen(*argv);
	filename = (char *) malloc(len+1);
	strcpy(filename, *argv);
    }

    MPI_Type_create_resized(MPI_INT, 0, 16, &newtype);   

    MPI_Type_commit(&newtype);

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | 
             MPI_MODE_RDWR, MPI_INFO_NULL, &fh);

    mpi_errno = MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", MPI_INFO_NULL);
    if (mpi_errno != MPI_SUCCESS) errs++;

    MPI_File_close(&fh);

    MPI_Type_free(&newtype);
    free(filename);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
