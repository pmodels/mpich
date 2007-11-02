/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>

/* FIXME: This test program assumes that MPI_Error_string will work even
   if MPI is not initialized.  That is not guaranteed.  */

/* Normally, when checking for error returns from MPI calls, you must ensure 
   that the error handler on the relevant object (communicator, file, or
   window) has been set to MPI_ERRORS_RETURN.  The tests in this 
   program are a special case, as either a failure or an abort will
   indicate a problem */

int main( int argc, char *argv[] )
{
    int error;
    int flag;
    char err_string[1024];
    int length = 1024;
    int rank;

    flag = 0;
    error = MPI_Finalized(&flag);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalized failed: %s\n", err_string);
	fflush(stdout);
	return error;
    }
    if (flag)
    {
	printf("MPI_Finalized returned true before MPI_Init.\n");
	return -1;
    }

    error = MPI_Init(&argc, &argv);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Init failed: %s\n", err_string);
	fflush(stdout);
	return error;
    }

    error = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Comm_rank failed: %s\n", err_string);
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
	return error;
    }

    flag = 0;
    error = MPI_Finalized(&flag);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalized failed: %s\n", err_string);
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
	return error;
    }
    if (flag)
    {
	printf("MPI_Finalized returned true before MPI_Finalize.\n");
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
	return -1;
    }

    error = MPI_Barrier(MPI_COMM_WORLD);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Barrier failed: %s\n", err_string);
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
	return error;
    }

    error = MPI_Finalize();
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalize failed: %s\n", err_string);
	fflush(stdout);
	return error;
    }

    flag = 0;
    error = MPI_Finalized(&flag);
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("MPI_Finalized failed: %s\n", err_string);
	fflush(stdout);
	return error;
    }
    if (!flag)
    {
	printf("MPI_Finalized returned false after MPI_Finalize.\n");
	return -1;
    }
    if (rank == 0)
    {
	printf(" No Errors\n");
    }
    return 0;  
}
