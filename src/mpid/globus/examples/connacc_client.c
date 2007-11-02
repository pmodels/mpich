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
#include <string.h>
#include <mpi.h>

const int num_iter = 2;

int main(int argc, char **argv)
{
    int i;
    int my_rank;
    int local_size;
    char * port_name;
    MPI_Comm newcomm;
    int remote_rank;
    int remote_size;
    int passed_num;

    port_name = strdup(argv[1]);
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &local_size);

    for (i = 0; i < num_iter; i++)
    {
	MPI_Comm_connect(argv[1], MPI_INFO_NULL, i % local_size, MPI_COMM_WORLD, &newcomm); 
	MPI_Comm_remote_size(newcomm, &remote_size);
	printf("client %d: remote size is %d\n", my_rank, remote_size);
	
	if (my_rank == (i + 2) % local_size)
	{
	    MPI_Status status;
	    remote_rank = (i + 4) % remote_size;
	    MPI_Recv(&passed_num, 1, MPI_INT, remote_rank, i, newcomm, &status);
	    printf("client %d: after receiving passed_num=%d to %d\n", my_rank, passed_num, remote_rank);
	    fflush(stdout);
	}

	if (my_rank == (i + 4) % local_size)
	{
	    passed_num=100 + i;
	    MPI_Bcast(&passed_num, 1, MPI_INT, MPI_ROOT, newcomm);
	    printf("client %d: after broadcasting passed_num=%d\n", my_rank, passed_num);
	    fflush(stdout);
	}
	else
	{
	    MPI_Bcast(NULL, 0, MPI_INT, MPI_PROC_NULL, newcomm);
	}

	if (my_rank == i % local_size)
	{
	    remote_rank = (i + 1) % remote_size;
	    passed_num = 200 + i;
	    MPI_Send(&passed_num, 1, MPI_INT, remote_rank, i, newcomm);
	    printf("client %d: after sending passed_num=%d to %d\n", my_rank, passed_num, remote_rank);
	    fflush(stdout);
	}

	MPI_Comm_disconnect(&newcomm);
    }

    MPI_Finalize();

    exit(0);
}
