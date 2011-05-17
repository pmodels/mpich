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
#include "mpithreadtest.h"

/*
static char MTEST_Descrip[] = "Spawn jobs from multiple threads";
*/

#define NTHREADS 4
#define NPERTHREAD 4
int activeThreads = 0;

MPI_Comm intercomms[NTHREADS];

MTEST_THREAD_RETURN_TYPE spawnProcess(void *p);

MTEST_THREAD_RETURN_TYPE spawnProcess(void *p)
{
    int rank, i;
    int errcodes[NPERTHREAD];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* The thread number is passed into this routine through the value of the
       argument */
    i = (int)(long)p;

    /* Synchronize */
    MTest_thread_barrier(NTHREADS);

    /* Spawn */
    MPI_Comm_spawn( (char*)"./multispawn", MPI_ARGV_NULL, NPERTHREAD, 
		    MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomms[i], errcodes );

    MPI_Bcast( &i, 1, MPI_INT, MPI_ROOT, intercomms[i] );

    return MTEST_THREAD_RETVAL_IGN;
}

int main( int argc, char *argv[] )
{
    int rank, size, i, wasParent = 0;
    int provided;
    int err;
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

        err = MTest_thread_barrier_init();
        if (err) {
            printf("barrier_init failed\n");
            fflush(stdout);
            MPI_Finalize();
            return 1;
        }

	for (i=0; i<NTHREADS-1; i++) {
            MTest_Start_thread(spawnProcess, (void *)(long)i);
	}

	/* spawn the processes */
	spawnProcess( (void*)(NTHREADS-1) );

	/* Exit the threads (but the spawned processes remain) */
	MTest_Join_threads();

        err = MTest_thread_barrier_free();
        if (err) {
            printf("barrier_free failed\n");
            fflush(stdout);
            MPI_Finalize();
            return 1;
        }

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
