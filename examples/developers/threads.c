/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#include "mpi.h"

void *thrdsub(void *arg);

int comm_rank, comm_size, thread_level_provided, num_msgs = 50;
pthread_mutex_t mpi_lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
    int i, rc, thrdrank[4] = {0,1,2,3};
    pthread_t thrdid[4];

    setbuf(stdout,NULL);

    rc = MPI_Init_thread(&argc,&argv,MPI_THREAD_MULTIPLE,&thread_level_provided);
    if (rc != MPI_SUCCESS)
    {
	printf("MPI_Init_thread failed with rc=%d\n",rc);
	exit(-1);
    }
    MPI_Comm_rank(MPI_COMM_WORLD,&comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    if (comm_size != 2)
    {
	printf("**** please run me with exactly 2 processes\n");
	exit(-1);
    }

    switch (thread_level_provided)
    {
        case MPI_THREAD_SINGLE:
	    printf("thread level supported: MPI_THREAD_SINGLE\n");
	    break;
        case MPI_THREAD_FUNNELED:
	    printf("thread level supported: MPI_THREAD_FUNNELED\n");
	    break;
        case MPI_THREAD_SERIALIZED:
	    printf("thread level supported: MPI_THREAD_SERIALIZED\n");
	    break;
        case MPI_THREAD_MULTIPLE:
	    printf("thread level supported: MPI_THREAD_MULTIPLE\n");
	    break;
        default:
	    printf("thread level supported: UNRECOGNIZED\n");
	    exit(-1);
    }

    for (i=0; i < 3; i++)
    {
	rc = pthread_create(&thrdid[i],(void*)NULL,(void*)thrdsub,(void*)&thrdrank[i]);
	if (rc < 0)
	{
	    printf("create of thread failed with rc=%d\n",rc);
	    exit(-1);
	}
    }
    for (i=0; i < 3; i++)
    {
	pthread_join(thrdid[i],NULL);
    }

    MPI_Finalize();
    exit(0);
}

void *thrdsub(void *arg)
{
    int mythrdrank, num_sent;
    char s_buff[1024] = "hello", r_buff[1024];
    MPI_Status status;

    mythrdrank = *((int *)arg);
    printf("comm_rank=%d mythrdrank=%d\n",comm_rank,mythrdrank);
    if (thread_level_provided == MPI_THREAD_SINGLE  ||
	thread_level_provided == MPI_THREAD_FUNNELED)
    {
	if (mythrdrank > 0)
	    pthread_exit(NULL);
    }
    for (num_sent=0; num_sent < num_msgs; num_sent++)
    {
	if (comm_rank == 0)
	{
	    if (thread_level_provided == MPI_THREAD_SINGLE  ||
		thread_level_provided == MPI_THREAD_FUNNELED)
	    {
		pthread_mutex_lock(&mpi_lock);
	    }
	    MPI_Send(s_buff,strlen(s_buff)+1,MPI_CHAR,1,mythrdrank,MPI_COMM_WORLD);
	    r_buff[0] = '\0';
	    MPI_Recv(r_buff,1024,MPI_CHAR,1,mythrdrank,MPI_COMM_WORLD,&status);
	    if (strcmp(s_buff,r_buff) != 0)
	    {
		printf("comm_rank=%d mythrdrank=%d bad recv\n",comm_rank,mythrdrank);
		exit(-1);
	    }
	    if (thread_level_provided == MPI_THREAD_SINGLE  ||
		thread_level_provided == MPI_THREAD_FUNNELED)
	    {
		pthread_mutex_unlock(&mpi_lock);
	    }
	}
	else
	{
	    if (thread_level_provided == MPI_THREAD_SINGLE  ||
		thread_level_provided == MPI_THREAD_FUNNELED)
	    {
		pthread_mutex_lock(&mpi_lock);
	    }
	    r_buff[0] = '\0';
	    MPI_Recv(r_buff,1024,MPI_CHAR,0,mythrdrank,MPI_COMM_WORLD,&status);
	    if (strcmp(s_buff,r_buff) != 0)
	    {
		printf("comm_rank=%d mythrdrank=%d bad recv\n",comm_rank,mythrdrank);
		exit(-1);
	    }
	    MPI_Send(s_buff,strlen(s_buff)+1,MPI_CHAR,0,mythrdrank,MPI_COMM_WORLD);
	    if (thread_level_provided == MPI_THREAD_SINGLE  ||
		thread_level_provided == MPI_THREAD_FUNNELED)
	    {
		pthread_mutex_unlock(&mpi_lock);
	    }
	}
	/* printf("sent and recvd\n"); */
    }
    return(NULL);
}
