/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int size;
    int rank;
    int * msg = NULL;
    int msg_sz = 0;
    int niter = 1;
    int iter;
    int i;
    
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
    {
        printf("ERROR: problem with MPI_Init\n"); fflush(stdout);
    }

    if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS)
    {
	printf("ERROR: problem with MPI_Comm_size\n"); fflush(stdout);
    }
    
    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)
    {
	printf("ERROR: problem with MPI_Comm_rank\n"); fflush(stdout);
    }

    printf("sr: size %d rank %d\n", size, rank); fflush(stdout);
    
    if (size < 2)
    {
	printf("ERROR: needs to be run with at least 2 procs\n");
	fflush(stdout);
    }

    if (argc > 1)
    {
	sscanf(argv[1], "%d", &msg_sz);
    }
    
    if (argc > 2)
    {
	sscanf(argv[2], "%d", &niter);
    }

    if (rank == 0)
    {
	printf("msg_sz=%d, niter=%d\n", msg_sz, niter);
	fflush(stdout);
    }
    
    if (msg_sz > 0)
    {
	msg = (int *) malloc(msg_sz * sizeof(int));
    }

    if (rank == 0)
    {
	/* usleep(10000); */
	    
	for (iter = 0; iter < niter; iter++)
	{
	    for (i = 0; i < msg_sz; i++)
	    {
		msg[i] = iter * msg_sz + i;
	    }
	    
	    if (MPI_Send(msg, msg_sz, MPI_INT, 1, iter, MPI_COMM_WORLD)
		!= MPI_SUCCESS)
	    {
		printf("ERROR: problem with MPI_Send\n"); fflush(stdout);
	    }
	}
	
    }
    else if (rank == 1)
    {
	MPI_Status status;
	    
	for (iter = 0; iter < niter; iter++)
	{
	    if (MPI_Recv(msg, msg_sz, MPI_INT, 0, iter, MPI_COMM_WORLD,
			 &status) != MPI_SUCCESS)
	    {
		printf("ERROR: problem with MPI_Recv\n"); fflush(stdout);
	    }
	
	    for (i = 0; i < msg_sz; i++)
	    {
		if (msg[i] != iter * msg_sz + i)
		{
		    printf("ERROR: %d != %d, i=%d iter=%d\n", msg[i],
			   iter * msg_sz + i, i, iter);
		    fflush(stdout);
		    abort();
		}
	    }
	}
	
	printf("All messages successfully received!\n");
	fflush(stdout);
    }

    printf("sr: process %d finished\n", rank); fflush(stdout);
    
    if (MPI_Finalize() != MPI_SUCCESS)
    {
        printf("ERROR: problem with MPI_Finalize\n"); fflush(stdout);
    }

    return 0;
}
