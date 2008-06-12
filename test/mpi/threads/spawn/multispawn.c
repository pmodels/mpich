/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* 
 * This (is a placeholder for a) test that creates 4 threads, each of which 
 * does a concurrent spawn of 4 more processes, for a total of 17 MPI processes
 * The resulting intercomms are tested for consistency (to ensure that the
 * spawns didn't get confused among the threads).
 * 
 * As an option, it will time the Spawn calls.  If the spawn calls block the
 * calling thread, this may show up in the timing of the calls
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Spawn jobs from multiple threads";

#define NTHREADS 4
#define NPERTHREAD 4
int activeThreads = 0;

/* We use this flag to permit the master thread to signal the threads
   that it creates */
volatile int flag = 0;

MPI_Comm intercomms[NTHREADS];

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define THREAD_RETURN_TYPE DWORD
/* HANDLE to listener thread */
HANDLE hThread[NTHREADS];

int start_spawn_thread(THREAD_RETURN_TYPE (*fn)(void *p), void *arg)
{
    hThread[activeThreads] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fn, 
					  arg, 0, NULL);
    if (hThread[activeThreads] == NULL)
    {
	return GetLastError();
    }
    activeThreads++;
    return 0;
}

int join_threads( void ){
    int err = 0;
    while (activeThreads > 0) {
	if(WaitForSingleObject(hThread[activeThreads-1], INFINITE) == WAIT_FAILED){
	    DEBUG(printf("Error WaitForSingleObject() \n"));
	    err = GetLastError();
	}
	CloseHandle(hThread[activeThreads-1]);
	activeThreads--;
	return err;
    }
}

#else
#include <pthread.h>
#define THREAD_RETURN_TYPE void *
pthread_t thread[NTHREADS];
int start_spawn_thread(THREAD_RETURN_TYPE (*fn)(void *p), void *arg);

int start_spawn_thread(THREAD_RETURN_TYPE (*fn)(void *p), void *arg)
{
    int err;

    err = pthread_create(&thread[activeThreads], NULL/*&attr*/, fn, arg);
    if (err == 0) activeThreads++;
    return err;
}
int join_threads( void )
{
    int err;
    while (activeThreads > 0) {
	err = pthread_join(thread[activeThreads-1],0);
	activeThreads--;
    }
    return err;
}
#endif

THREAD_RETURN_TYPE spawnProcess(void *p);

THREAD_RETURN_TYPE spawnProcess(void *p)
{
    int rank, i;
    char buffer[100];
    int errcodes[NPERTHREAD];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* The thread number is passed into this routine through the value of the
       argument */
    i = (int)p;
    /* Synchronize */
    while (!flag) ;

    /* Spawn */
    MPI_Comm_spawn( "./multispawn", MPI_ARGV_NULL, NPERTHREAD, 
		    MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomms[i], errcodes );

    MPI_Bcast( &i, 1, MPI_INT, MPI_ROOT, intercomms[i] );

    return (THREAD_RETURN_TYPE)0;
}

int main( int argc, char *argv[] )
{
    int rank, size, i, wasParent = 0;
    int provided;
    char buffer[100];
    MPI_Status status;
    MPI_Comm   parentcomm;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (provided != MPI_THREAD_MULTIPLE)
    {
	if (rank == 0)
	{
	    printf("MPI_Init_thread must return MPI_THREAD_MULTIPLE in order for this test to run.\n");
	    fflush(stdout);
	}
	MPI_Finalize();
	return -1;
    }

    MPI_Comm_get_parent( &parentcomm );
    if (parentcomm == MPI_COMM_NULL) {
	wasParent = 1;
	
	for (i=0; i<NTHREADS-1; i++) {
	    start_spawn_thread( spawnProcess, (void *)i );
	}
	
	/* Synchronize the threads and spawn the processes */
	flag = 1;
	spawnProcess( (void*)(NTHREADS-1) );

	/* Exit the threads (but the spawned processes remain) */
	join_threads();

	/* The master thread (this thread) checks the created communicators */
	for (i=0; i<NTHREADS; i++) {
	    MPI_Bcast( &i, 1, MPI_INT, MPI_ROOT, intercomms[i] );
	}

	/* Free the created processes */
	for (i=0; i<NTHREADS; i++) {
	    MPI_Comm_disconnect( &intercomms[i] );
	}
    }
    else {
	int num, threadnum;

	/* I'm the created process */
	MPI_Bcast( &threadnum, 1, MPI_INT, 0, parentcomm );
	
	/* Form an intra comm with my parent */

	/* receive from my parent */
	MPI_Bcast( &num, 1, MPI_INT, 0, parentcomm );

	if (num != threadnum) {
	    fprintf( stderr, "Unexpected thread num (%d != %d)\n", 
		     threadnum, num );
	}

	/* Let the parent free the intercomms */
	MPI_Comm_disconnect( &parentcomm );
	
    }

    if (wasParent) 
	MTest_Finalize(0);

    MPI_Finalize();
    return 0;
}
