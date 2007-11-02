/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include "mpitest.h"
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#include <stdio.h>

#ifdef DO_DEBUG
#define DEBUG(_a){ _a ;fflush(stdout);}
#else
#define DEBUG(_a)
#endif

const int REQ_TAG = 111;
const int ANS_TAG = 222;

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define THREAD_RETURN_TYPE DWORD
/* HANDLE to listener thread */
HANDLE hThread;
int start_send_thread(THREAD_RETURN_TYPE (*fn)(void *p))
{
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fn, NULL, 0, NULL);
    if (hThread == NULL){
        DEBUG(printf("Error CreateThread() \n"));
	    return GetLastError();
    }
    return 0;
}

int join_thread( void ){
    int err = 0;
    if(WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED){
        DEBUG(printf("Error WaitForSingleObject() \n"));
        err = GetLastError();
    }
    CloseHandle(hThread);
    return err;
}

#else
#include <pthread.h>
#define THREAD_RETURN_TYPE void *
pthread_t thread;
int start_send_thread(THREAD_RETURN_TYPE (*fn)(void *p));

int start_send_thread(THREAD_RETURN_TYPE (*fn)(void *p))
{
    int err;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    err = pthread_create(&thread, &attr, fn, NULL);
    pthread_attr_destroy(&attr);
    return err;
}
int join_thread( void )
{
    return pthread_join(thread, 0);
}
#endif

/* MPI environment description */
int rank, size, provided;

void* listener(void*);

/* 
   LISTENER THREAD
   it waits for communication from any source (including calling thread)
   messages which it receives have tag REQ_TAG
   thread runs infinite loop which will stop only if every node in the
   MPI_COMM_WORLD send request containing -1 
*/
void* listener(void*extra) {
    int req;
    int source;
    MPI_Status stat;

    int no_fins = 0; /* this must be equal to size to break loop below */

    while (1) {
	/* wait for request */
	MPI_Recv(&req, 1, MPI_INT, 
	 	 MPI_ANY_SOURCE, REQ_TAG, MPI_COMM_WORLD, &stat);

	/* get request source */
	source = stat.MPI_SOURCE;

	DEBUG(printf( "node %d got request %d from %d\n", rank, req, source ));

	if (req == -1) ++no_fins; /* one more node finished requesting */
	
	/* no more requests can arrive */
	if (no_fins == size) break;
	
    }

    DEBUG(printf( "node %d has stopped listener\n", rank ));
    return 0;
} 


int main(int argc, char* argv[]) {
    /*
    pthread_t thr;
    pthread_attr_t attr;
    */
    int buf = 0;
    long int i;
    
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided != MPI_THREAD_MULTIPLE) {
	printf( "This test requires MPI_THREAD_MULTIPLE\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

#ifdef USE_BARRIER
    MPI_Barrier( MPI_COMM_WORLD );
#endif

    /* create listener thread */
    start_send_thread(listener);

    /* no more requests to send
       inform other in the group that we have finished */
    buf = -1;
    for (i = 0; i < size; ++i) {
	MPI_Send(&buf, 1, MPI_INT, i, REQ_TAG, MPI_COMM_WORLD);
    }

    /* and wait for others to do the same */
    join_thread();

    MPI_Finalize();

    /* This program works if it gets here */
    if (rank == 0) {
	printf( " No Errors\n" );
    }

    return 0;
} 
