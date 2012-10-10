/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* define TEST_RECV_VECTOR to receive the data using the vector datatype.
   undefine TEST_RECV_VECTOR to receive the data into a contiguous array. */
#define TEST_RECV_VECTOR

#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#include <limits.h>

int MPID_Progress_test(void);

int main(int argc, char **argv)
{
    int size;
    int rank;
    int niter = 1;
    int msg_count = 0;
    int msg_blocklength = 1;
    int msg_stride = 1;
    int msg_sz;
    int * buf;
    int buf_sz;
    int iter;
    int i;
    MPI_Datatype dt;
    
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

    printf("srvec: size %d rank %d\n", size, rank); fflush(stdout);
    
    if (size < 2)
    {
	if (rank == 0)
	{
	    printf("ERROR: needs to be run with at least 2 procs\n");
	    fflush(stdout);
	}
	goto main_exit;
    }

    if (argc > 1)
    {
	sscanf(argv[1], "%d", &niter);
    }

    if (argc > 2)
    {
	sscanf(argv[2], "%d", &msg_count);
    }
    
    if (argc > 3)
    {
	sscanf(argv[3], "%d", &msg_blocklength);
    }
    
    if (argc > 4)
    {
	sscanf(argv[4], "%d", &msg_stride);
    }

    if (msg_stride < msg_blocklength)
    {
	if (rank == 0)
	{
	    printf("ERROR: stride < blocklength\n");
	    fflush(stdout);
	}
	goto main_exit;
    }
    

    msg_sz = msg_count * msg_blocklength;
    buf_sz = msg_count * msg_stride;
    
    if (rank == 0)
    {
	printf("niter=%d, msg_count=%d, msg_blocklength=%d, msg_stride=%d\n",
	    niter, msg_count, msg_blocklength, msg_stride);
	printf("msg_sz=%d, buf_sz=%d\n", msg_sz, buf_sz);
	fflush(stdout);
    }

    if (buf_sz > 0)
    {
	buf = (int *) malloc(buf_sz * sizeof(int));
	/* printf("%d: buf=%p\n", rank, buf); fflush(stdout); */
    }
    else
    {
	buf = NULL;
    }

    MPI_Type_vector(msg_count, msg_blocklength, msg_stride, MPI_INT, &dt);
    MPI_Type_commit(&dt);

    if (rank == 0)
    {
	/* usleep(10000); */

	for (i = 0; i < buf_sz; i++)
	{
	    buf[i] = INT_MAX;
	}
	
	for (iter = 0; iter < niter; iter++)
	{
	    for (i = 0; i < buf_sz; i++)
	    {
		buf[i] = iter * buf_sz + i;
	    }
	    
	    if (MPI_Send(buf, 1, dt, 1, iter, MPI_COMM_WORLD) != MPI_SUCCESS)
	    {
		printf("ERROR: problem with MPI_Send\n"); fflush(stdout);
	    }
	}
	
    }
    else if (rank == 1)
    {
	MPI_Status status;
	    
	for (i = 0; i < buf_sz; i++)
	{
	    buf[i] = INT_MIN;
	}

	for (iter = 0; iter < niter; iter++)
	{
#	    if defined(TEST_RECV_VECTOR)
	    {
		if (MPI_Recv(buf, msg_sz, MPI_INT, 0, iter, MPI_COMM_WORLD,
		    &status) != MPI_SUCCESS)
		{
		    printf("ERROR: problem with MPI_Recv\n"); fflush(stdout);
		}
		
		for (i = 0; i < msg_sz; i++)
		{
		    const int expected = iter * buf_sz + i / msg_blocklength *
			msg_stride + i % msg_blocklength;
		    if (buf[i] != expected)
		    {
			printf("ERROR: %d != %d, i=%d iter=%d\n", buf[i],
			    expected, i, iter);
			fflush(stdout);
			abort();
		    }
		}
	    }
#	    else
	    {
		if (MPI_Recv(buf, 1, dt, 0, iter, MPI_COMM_WORLD, &status)
		    != MPI_SUCCESS)
		{
		    printf("ERROR: problem with MPI_Recv\n"); fflush(stdout);
		}
		
		for (i = 0; i < buf_sz; i++)
		{
		    if (i % msg_stride < msg_blocklength && buf[i] != iter *
			buf_sz + i)
		    {
			printf("ERROR: %d != %d, i=%d iter=%d\n", buf[i],
			    iter * buf_sz + i, i, iter);
			fflush(stdout);
			abort();
		    }
		}
	    }
#	    endif
	}
	
	printf("All messages successfully received!\n");
	fflush(stdout);
    }

  main_exit:
    MPI_Barrier(MPI_COMM_WORLD);
    printf("srvec: process %d finished\n", rank); fflush(stdout);
    
    if (MPI_Finalize() != MPI_SUCCESS)
    {
        printf("ERROR: problem with MPI_Finalize\n"); fflush(stdout);
    }

    return 0;
}
