/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/* This program is an updated version of the mclient program found on the MPICH-G2 home page. */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

const int num_iter = 2;

int main(int argc, char **argv)
{
    int i;
    int my_rank;
    int local_size;
    char port_name[MPI_MAX_PORT_NAME];
    MPI_Comm newcomm;
    int remote_rank;
    int remote_size;
    int passed_num;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &local_size);

    if (my_rank == 0)
    {
	MPI_Open_port(MPI_INFO_NULL, port_name);
	printf("\n%s\n\n", port_name); fflush(stdout);
    }

    for (i = 0; i < num_iter; i++)
    {
	MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &newcomm); 
	MPI_Comm_remote_size(newcomm, &remote_size);
	printf("server %d: remote size is %d\n", my_rank, remote_size);

	if (my_rank == (i + 4) % local_size)
	{
	    remote_rank = (i + 2) % remote_size;
	    passed_num = i;
	    MPI_Send(&passed_num, 1, MPI_INT, remote_rank, i, newcomm);
	    printf("server %d: after sending passed_num=%d to %d\n", my_rank, passed_num, remote_rank);
	    fflush(stdout);
	}

	remote_rank = (i + 4) % remote_size;
	MPI_Bcast(&passed_num, 1, MPI_INT, remote_rank, newcomm);
	printf("server %d: after receiving broadcast of passed_num=%d from %d\n", my_rank, passed_num, remote_rank);
	fflush(stdout);

	if (my_rank == (i + 1) %local_size)
	{
	    MPI_Status status;
	    remote_rank = i % remote_size;
	    MPI_Recv(&passed_num, 1, MPI_INT, remote_rank, i, newcomm, &status);
	    printf("server %d: after receiving passed_num=%d to %d\n", my_rank, passed_num, remote_rank);
	    fflush(stdout);
	}

	MPI_Comm_disconnect(&newcomm);
    }
    
    if (my_rank == 0)
    {
	MPI_Close_port(port_name);
    }
    
    MPI_Finalize();

    exit(0);
}
