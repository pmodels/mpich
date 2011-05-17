/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* 
 * Run concurrent sends to different target processes. Stresses an 
 * implementation that permits concurrent sends to different targets.
 *
 * By turning on verbose output, some simple performance data will be output.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"
#include "mpithreadtest.h"

/* This is the master test routine */
#define MAX_CNT 660000
#define MAX_LOOP 200

static int nthreads = -1;

MTEST_THREAD_RETURN_TYPE run_test_send(void *arg);
MTEST_THREAD_RETURN_TYPE run_test_send(void *arg)
{
    int    cnt, j, *buf;
    int    thread_num = (int)(long)arg;
    double t;

    for (cnt=1; cnt < MAX_CNT; cnt = 2*cnt) {
	buf = (int *)malloc( cnt * sizeof(int) );

	/* Wait for all senders to be ready */
	MTest_thread_barrier(nthreads);

	t = MPI_Wtime();
	for (j=0; j<MAX_LOOP; j++)
	    MPI_Send( buf, cnt, MPI_INT, thread_num, cnt, MPI_COMM_WORLD );
	t = MPI_Wtime() - t;
	free( buf );
	if (thread_num == 1)
	    MTestPrintfMsg( 1, "buf size %d: time %f\n", cnt, t / MAX_LOOP );
    }
    return (MTEST_THREAD_RETURN_TYPE)NULL;
}

void run_test_recv( void );
void run_test_recv( void )
{
    int        cnt, j, *buf;
    MPI_Status status;
    double     t;

    for (cnt=1; cnt < MAX_CNT; cnt = 2*cnt) {
	buf = (int *)malloc( cnt * sizeof(int) );
	t = MPI_Wtime();
	for (j=0; j<MAX_LOOP; j++)
	    MPI_Recv( buf, cnt, MPI_INT, 0, cnt, MPI_COMM_WORLD, &status );
	t = MPI_Wtime() - t;
	free( buf );
    }
}

int main(int argc, char ** argv)
{
    int i, pmode, nprocs, rank;
    int errs = 0, err;

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
	fprintf(stderr, "Thread Multiple not supported by the MPI implementation\n");
	MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs < 2) {
	fprintf(stderr, "Need at least two processes\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MTest_thread_barrier_init();
    if (err) {
	fprintf( stderr, "Could not create thread barrier\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank == 0) {
	nthreads = nprocs-1;
	for (i=1; i<nprocs; i++) 
	    MTest_Start_thread( run_test_send,  (void *)(long)i );
	MTest_Join_threads( );
    }
    else {
	run_test_recv();
    }
    MTest_thread_barrier_free();

    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}
