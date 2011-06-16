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
 *
 * Use nonblocking sends, and have a single thread complete all I/O.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"
#include "mpithreadtest.h"

/* This is the master test routine */
#define MAX_CNT 660000
/* #define MAX_LOOP 200 */
#define MAX_LOOP 10
#define MAX_NTHREAD 128

static int ownerWaits = 0;
static int nthreads = -1;

MTEST_THREAD_RETURN_TYPE run_test_sendrecv(void *arg);
MTEST_THREAD_RETURN_TYPE run_test_sendrecv(void *arg)
{
    int    cnt, j, *buf, wsize;
    int    thread_num = (int)(long)arg;
    double t;
    int    myrloc = 2*thread_num;
    static MPI_Request r[2*MAX_NTHREAD];

    MPI_Comm_size( MPI_COMM_WORLD, &wsize );
    if (wsize >= MAX_NTHREAD) wsize = MAX_NTHREAD;

    /* Sanity check */
    if (nthreads != wsize)
	fprintf( stderr, "Panic wsize = %d nthreads = %d\n",
		 wsize, nthreads );

    for (cnt=1; cnt < MAX_CNT; cnt = 2*cnt) {
	buf = (int *)malloc( 2*cnt * sizeof(int) );

	/* Wait for all senders to be ready */
	MTest_thread_barrier(nthreads);

	t = MPI_Wtime();
	for (j=0; j<MAX_LOOP; j++) {
	    MTest_thread_barrier(nthreads);
	    MPI_Isend( buf, cnt, MPI_INT, thread_num, cnt, MPI_COMM_WORLD,
		       &r[myrloc]);
	    MPI_Irecv( buf+cnt, cnt, MPI_INT, thread_num, cnt, MPI_COMM_WORLD,
		       &r[myrloc+1]);
	    /* Wait for all threads to start the sends */
	    if (ownerWaits) {
		MPI_Waitall( 2, &r[myrloc], MPI_STATUSES_IGNORE );
	    }
	    else {
		/* Wait for all threads to create their requests */
		MTest_thread_barrier(nthreads);
		if (thread_num == 1)
		    MPI_Waitall( 2*wsize, r, MPI_STATUSES_IGNORE );

	    }
	}
	t = MPI_Wtime() - t;
	/* can't free the buffers until the requests are completed */
	MTest_thread_barrier(nthreads);
	free( buf );
	if (thread_num == 1)
	    MTestPrintfMsg( 1, "buf size %d: time %f\n", cnt, t / MAX_LOOP );
    }
    return (MTEST_THREAD_RETURN_TYPE)NULL;
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
    if (nprocs > MAX_NTHREAD) nprocs = MAX_NTHREAD;

    err = MTest_thread_barrier_init();
    if (err) {
	fprintf( stderr, "Could not create thread barrier\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
    MPI_Barrier( MPI_COMM_WORLD );
    if (rank < MAX_NTHREAD) {
	int nt = nprocs;
	if (nt > MAX_NTHREAD) nt = MAX_NTHREAD;
	nthreads = nt;
	for (i=0; i<nt; i++) 
	    MTest_Start_thread( run_test_sendrecv,  (void *)(long)i );
    }
    MTest_Join_threads( );
    MTest_thread_barrier_free();

    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}
