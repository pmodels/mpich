/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void check_error( int, const char * );
void check_error(int error, const char *fcname)
{
    char err_string[MPI_MAX_ERROR_STRING] = "";
    int length;
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("%s failed: %s\n", fcname, err_string);
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
    }
}

int main( int argc, char *argv[] )
{
    int error;
    int rank, size;
    char port[MPI_MAX_PORT_NAME];
    MPI_Status status;
    MPI_Comm comm;
    int verbose = 0;

    if (getenv("MPITEST_VERBOSE"))
    {
	verbose = 1;
    }

    if (verbose) { printf("init.\n");fflush(stdout); }
    error = MPI_Init(&argc, &argv);
    check_error(error, "MPI_Init");

    /* To improve reporting of problems about operations, we
       change the error handler to errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    MPI_Comm_set_errhandler( MPI_COMM_SELF, MPI_ERRORS_RETURN );

    if (verbose) { printf("size.\n");fflush(stdout); }
    error = MPI_Comm_size(MPI_COMM_WORLD, &size);
    check_error(error, "MPI_Comm_size");

    if (verbose) { printf("rank.\n");fflush(stdout); }
    error = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    check_error(error, "MPI_Comm_rank");

    if (size < 2)
    {
	printf("Two processes needed.\n");
	MPI_Finalize();
	return 0;
    }

    if (rank == 0)
    {
	if (verbose) { printf("open_port.\n");fflush(stdout); }
	error = MPI_Open_port(MPI_INFO_NULL, port);
	check_error(error, "MPI_Open_port");

	if (verbose) { printf("0: opened port: <%s>\n", port);fflush(stdout); }
	if (verbose) { printf("send.\n");fflush(stdout); }
	error = MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
	check_error(error, "MPI_Send");

	if (verbose) { printf("accept.\n");fflush(stdout); }
	error = MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
	check_error(error, "MPI_Comm_accept");

	if (verbose) { printf("close_port.\n");fflush(stdout); }
	error = MPI_Close_port(port);
	check_error(error, "MPI_Close_port");

	if (verbose) { printf("disconnect.\n");fflush(stdout); }
	error = MPI_Comm_disconnect(&comm);
	check_error(error, "MPI_Comm_disconnect");
    }
    else if (rank == 1)
    {
	if (verbose) { printf("recv.\n");fflush(stdout); }
	error = MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
	check_error(error, "MPI_Recv");

	if (verbose) { printf("1: received port: <%s>\n", port);fflush(stdout); }
	if (verbose) { printf("connect.\n");fflush(stdout); }
	error = MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
	check_error(error, "MPI_Comm_connect");

	MPI_Comm_set_errhandler( comm, MPI_ERRORS_RETURN );

	if (verbose) { printf("disconnect.\n");fflush(stdout); }
	error = MPI_Comm_disconnect(&comm);
	check_error(error, "MPI_Comm_disconnect");
    }

    error = MPI_Barrier(MPI_COMM_WORLD);
    check_error(error, "MPI_Barrier");

    if (rank == 0)
    {
	printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}
