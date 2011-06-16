/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define IF_VERBOSE(a) if (verbose) { printf a ; fflush(stdout); }

/*
static char MTEST_Descrip[] = "A simple test of Comm_connect/accept/disconnect";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, rsize, i, j, data, num_loops = 100;
    int np = 3;
    MPI_Comm      parentcomm, intercomm;
    MPI_Status    status;
    char port[MPI_MAX_PORT_NAME] = {0};
    int verbose = 0;
    int do_messages = 1;
    char *env;

    env = getenv("MPITEST_VERBOSE");
    if (env)
    {
	if (*env != '0')
	    verbose = 1;
    }

    MTest_Init( &argc, &argv );

    /* FIXME: Document arguments */
    if (argc > 1) {
	num_loops = atoi(argv[1]);
	if (num_loops < 0)
	    num_loops = 0;
	if (num_loops > 100)
	    num_loops = 100;
    }
    if (argc > 2)
    {
	do_messages = atoi(argv[2]);
    }
    MPI_Comm_get_parent( &parentcomm );

    if (parentcomm == MPI_COMM_NULL)
    {
	MPI_Comm_rank( MPI_COMM_WORLD, &rank ); /* Get rank for verbose msg */
	IF_VERBOSE(("[%d] spawning %d processes\n", rank, np));
	/* Create 3 more processes */
	MPI_Comm_spawn((char*)"./disconnect_reconnect",
			/*MPI_ARGV_NULL*/&argv[1], np,
			MPI_INFO_NULL, 0, MPI_COMM_WORLD,
			&intercomm, MPI_ERRCODES_IGNORE);
    }
    else
    {
	intercomm = parentcomm;
    }

    /* We now have a valid intercomm */

    MPI_Comm_remote_size(intercomm, &rsize);
    MPI_Comm_size(intercomm, &size);
    MPI_Comm_rank(intercomm, &rank);

    if (parentcomm == MPI_COMM_NULL)
    {
	IF_VERBOSE(("[%d] parent rank %d alive.\n", rank, rank));
	/* Parent */
	if (rsize != np)
	{
	    errs++;
	    printf("Did not create %d processes (got %d)\n", np, rsize);
	    fflush(stdout);
	}
	if (rank == 0 && num_loops > 0)
	{
	    MPI_Open_port(MPI_INFO_NULL, port);
	    IF_VERBOSE(("[%d] port = %s\n", rank, port));
	    MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, intercomm);
	}
	IF_VERBOSE(("[%d] disconnecting child communicator\n",rank));
	MPI_Comm_disconnect(&intercomm);
	for (i=0; i<num_loops; i++)
	{
	    IF_VERBOSE(("[%d] accepting connection\n",rank));
	    MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm);
	    MPI_Comm_remote_size(intercomm, &rsize);
	    if (do_messages && (rank == 0))
	    {
		j = 0;
		for (j=0; j<rsize; j++)
		{
		    data = i;
		    IF_VERBOSE(("[%d]sending int to child process %d\n", rank, j));
		    MPI_Send(&data, 1, MPI_INT, j, 100, intercomm);
		    IF_VERBOSE(("[%d] receiving int from child process %d\n", rank, j));
		    data = i-1;
		    MPI_Recv(&data, 1, MPI_INT, j, 100, intercomm, &status);
		    if (data != i)
		    {
			errs++;
		    }
		}
	    }
	    IF_VERBOSE(("[%d] disconnecting communicator\n", rank));
	    MPI_Comm_disconnect(&intercomm);
	}

	/* Errors cannot be sent back to the parent because there is no 
	   communicator connected to the children
	for (i=0; i<rsize; i++)
	{
	    MPI_Recv( &err, 1, MPI_INT, i, 1, intercomm, MPI_STATUS_IGNORE );
	    errs += err;
	}
	*/
    }
    else
    {
	IF_VERBOSE(("[%d] child rank %d alive.\n", rank, rank));
	/* Child */
	if (size != np)
	{
	    errs++;
	    printf("(Child) Did not create %d processes (got %d)\n", np, size);
	    fflush(stdout);
	}

	if (rank == 0 && num_loops > 0)
	{
	    IF_VERBOSE(("[%d] receiving port\n", rank));
	    MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, intercomm, &status);
	}

	IF_VERBOSE(("[%d] disconnecting communicator\n", rank));
	MPI_Comm_disconnect(&intercomm);
	for (i=0; i<num_loops; i++)
	{
	    IF_VERBOSE(("[%d] connecting to port (loop %d)\n",rank,i));
	    MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm);
	    if (do_messages)
	    {
		IF_VERBOSE(("[%d] receiving int from parent process 0\n",rank));
		MPI_Recv(&data, 1, MPI_INT, 0, 100, intercomm, &status);
		if (data != i)
		{
		    printf("expected %d but received %d\n", i, data);
		    fflush(stdout);
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
		IF_VERBOSE(("[%d] sending int back to parent process 1\n",rank));
		MPI_Send(&data, 1, MPI_INT, 0, 100, intercomm);
	    }
	    IF_VERBOSE(("[%d] disconnecting communicator\n",rank));
	    MPI_Comm_disconnect(&intercomm);
	}

	/* Send the errs back to the master process */
	/* Errors cannot be sent back to the parent because there is no 
	   communicator connected to the parent */
	/*MPI_Ssend( &errs, 1, MPI_INT, 0, 1, intercomm );*/
    }

    /* Note that the MTest_Finalize get errs only over COMM_WORLD */
    /* Note also that both the parent and child will generate "No Errors"
       if both call MTest_Finalize */
    if (parentcomm == MPI_COMM_NULL)
    {
	MTest_Finalize( errs );
    }

    IF_VERBOSE(("[%d] calling finalize\n",rank));
    MPI_Finalize();
    return 0;
}
