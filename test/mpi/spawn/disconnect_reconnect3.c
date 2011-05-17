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
#include <assert.h>

/*
 * This test tests the disconnect code for processes that span process groups.
 *
 * This test spawns a group of processes and then merges them into a single 
 * communicator.
 * Then the single communicator is split into two communicators, one containing
 * the even ranks and the other the odd ranks.
 * Then the two new communicators do MPI_Comm_accept/connect/disconnect calls 
 * in a loop.
 * The even group does the accepting while the odd group does the connecting.
 *
 */

#define IF_VERBOSE(a) if (verbose) { printf a ; fflush(stdout); }

/*
static char MTEST_Descrip[] = "A simple test of Comm_connect/accept/disconnect";
*/

/*
 * Reverse the order of the ranks in a communicator
 *
 * Thanks to Edric Ellis for contributing this portion of the test program.
 */
void checkReverseMergedComm( MPI_Comm comm );
void checkReverseMergedComm( MPI_Comm comm )
{
    MPI_Group origGroup;
    MPI_Group newGroup;
    MPI_Comm newComm;
    int buf = 5;
    int *newGroupRanks = NULL;
    int ii;
    int merged_size;

    assert(comm != MPI_COMM_NULL);
    /* try a bcast as a weak sanity check that the communicator works */
    MPI_Bcast(&buf, 1, MPI_INT, 0, comm);

    MPI_Comm_size(comm, &merged_size);
    newGroupRanks = calloc(merged_size, sizeof(int));
    for ( ii = 0; ii < merged_size; ++ii ) {
        newGroupRanks[ii] = merged_size - ii - 1;
    }
    MPI_Comm_group( comm, &origGroup );
    MPI_Group_incl( origGroup, merged_size, newGroupRanks, &newGroup );
    free( newGroupRanks );
    MPI_Comm_create( comm, newGroup, &newComm );

    assert(newComm != MPI_COMM_NULL);
    /* try a bcast as a weak sanity check that the communicator works */
    MPI_Bcast(&buf, 1, MPI_INT, 0, newComm);

    MPI_Group_free( &origGroup );
    MPI_Group_free( &newGroup );
    MPI_Comm_free( &newComm );
}



int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, rsize, i, j, data, num_loops = 100;
    int np = 4;
    MPI_Comm parentcomm, intercomm, intracomm, comm;
    MPI_Status status;
    char port[MPI_MAX_PORT_NAME] = {0};
    int even_odd;
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

    /* command line arguments can change the number of loop iterations and 
       whether or not messages are sent over the new communicators */
    if (argc > 1)
    {
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

    /* Spawn the child processes and merge them into a single communicator */
    MPI_Comm_get_parent( &parentcomm );
    if (parentcomm == MPI_COMM_NULL)
    {
	IF_VERBOSE(("spawning %d processes\n", np));
	/* Create 4 more processes */
	MPI_Comm_spawn((char*)"./disconnect_reconnect3",
			/*MPI_ARGV_NULL*/&argv[1], np,
			MPI_INFO_NULL, 0, MPI_COMM_WORLD,
			&intercomm, MPI_ERRCODES_IGNORE);
	MPI_Intercomm_merge(intercomm, 0, &intracomm);
    }
    else
    {
	intercomm = parentcomm;
	MPI_Intercomm_merge(intercomm, 1, &intracomm);
    }

    checkReverseMergedComm(intracomm);

    /* We now have a valid intracomm containing all processes */

    MPI_Comm_rank(intracomm, &rank);

    /* Split the communicator so that the even ranks are in one communicator 
       and the odd ranks are in another */
    even_odd = rank % 2;
    MPI_Comm_split(intracomm, even_odd, rank, &comm);

    /* Open a port on rank zero of the even communicator */
    /* rank 0 on intracomm == rank 0 on even communicator */
    if (rank == 0)
    {
	MPI_Open_port(MPI_INFO_NULL, port);
	IF_VERBOSE(("port = %s\n", port));
    }
    /* Broadcast the port to everyone.  This makes the logic easier than 
       trying to figure out which process in the odd communicator to send it 
       to */
    MPI_Bcast(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, intracomm);

    IF_VERBOSE(("disconnecting parent/child communicator\n"));
    MPI_Comm_disconnect(&intercomm);
    MPI_Comm_disconnect(&intracomm);

    MPI_Comm_rank(comm, &rank);

    if (!even_odd)
    {
	/* The even group does the accepting */
	for (i=0; i<num_loops; i++)
	{
	    IF_VERBOSE(("accepting connection\n"));
	    MPI_Comm_accept(port, MPI_INFO_NULL, 0, comm, &intercomm);
	    MPI_Comm_remote_size(intercomm, &rsize);
	    if (do_messages && (rank == 0))
	    {
		j = 0;
		for (j=0; j<rsize; j++)
		{
		    data = i;
		    IF_VERBOSE(("sending int to odd_communicator process %d\n", j));
		    MPI_Send(&data, 1, MPI_INT, j, 100, intercomm);
		    IF_VERBOSE(("receiving int from odd_communicator process %d\n", j));
		    data = i-1;
		    MPI_Recv(&data, 1, MPI_INT, j, 100, intercomm, &status);
		    if (data != i)
		    {
			errs++;
		    }
		}
	    }
	    IF_VERBOSE(("disconnecting communicator\n"));
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
	/* The odd group does the connecting */
	for (i=0; i<num_loops; i++)
	{
	    IF_VERBOSE(("connecting to port\n"));
	    MPI_Comm_connect(port, MPI_INFO_NULL, 0, comm, &intercomm);
	    if (do_messages)
	    {
		IF_VERBOSE(("receiving int from even_communicator process 0\n"));
		MPI_Recv(&data, 1, MPI_INT, 0, 100, intercomm, &status);
		if (data != i)
		{
		    printf("expected %d but received %d\n", i, data);
		    fflush(stdout);
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
		IF_VERBOSE(("sending int back to even_communicator process 0\n"));
		MPI_Send(&data, 1, MPI_INT, 0, 100, intercomm);
	    }
	    IF_VERBOSE(("disconnecting communicator\n"));
	    MPI_Comm_disconnect(&intercomm);
	}

	/* Send the errs back to the master process */
	/* Errors cannot be sent back to the parent because there is no 
	   communicator connected to the parent */
	/*MPI_Ssend( &errs, 1, MPI_INT, 0, 1, intercomm );*/
    }

    MPI_Comm_free( &comm );
    /* Note that the MTest_Finalize get errs only over COMM_WORLD */
    /* Note also that both the parent and child will generate "No Errors"
       if both call MTest_Finalize */
    if (parentcomm == MPI_COMM_NULL)
    {
	MTest_Finalize( errs );
    }

    IF_VERBOSE(("calling finalize\n"));
    MPI_Finalize();
    return 0;
}
