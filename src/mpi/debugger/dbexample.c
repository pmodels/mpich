/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>

/* style: allow:calloc:1 sig:0 */
/* style: allow:fprintf:2 sig:0 */
/* style: allow:printf:2 sig:0 */

/* 
 * This program provides a convienient way to test some of the debugger
 * interface functionality, including message queues and named communicators
 */

#define MAX_TARGETS 100

int main( int argc, char *argv[] )
{
    int         provided, wrank, wsize, nmsg, i, tag;
    int         *(buf[MAX_TARGETS]), bufsize[MAX_TARGETS];
    MPI_Request r[MAX_TARGETS];
    MPI_Comm    commDup, commEven;

    MPI_Init_thread( &argc, &argv, MPI_THREAD_FUNNELED, &provided );
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( MPI_COMM_WORLD, &wsize );

    if (wsize < 4) {
	fprintf( stderr, "This test requires at least 4 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }
	
    /* Create several communicators */
    MPI_Comm_dup( MPI_COMM_WORLD, &commDup );
    MPI_Comm_set_name( commDup, "User dup of comm world" );
    
    MPI_Comm_split( MPI_COMM_WORLD, wrank & 0x1, wrank, &commEven );
    if (wrank & 0x1) MPI_Comm_free( &commEven );
    else MPI_Comm_set_name( commEven, "User split to even ranks" );

    /* Create a collection of pending sends and receives
       We use tags on the sends and receives (when ANY_TAG isn't used)
       to provide an easy way to check that the proper requests are present.
       TAG values use fields, in decimal (for easy reading):
       0-99: send/recv type:
            0 - other
            1 - irecv
            2 - isend
            3 - issend
            4 - ibsend
            5 - irsend
            6 - persistent recv
            7 - persistent send
            8 - persistent ssend
            9 - persistent rsend
           10 - persistent bsend
        100-999: destination (for send) or source, if receive.  999 = any-source
	         (rank is value/100)
        1000-2G: other values
    */
    /* Create the send/receive buffers */
    nmsg = 10;
    for (i=0; i<nmsg; i++) {
	bufsize[i] = i;
	if (i)  {
	    buf[i] = (int*)calloc(bufsize[i],sizeof(int));
	    if (!buf[i]) {
		fprintf( stderr, "Unable to allocate %d words\n", bufsize[i] );
		MPI_Abort( MPI_COMM_WORLD, 2 );
	    }
	}
	else
	    buf[i] = 0;
    }

    /* Partial implementation */
    if (wrank == 0) {
	nmsg = 0;
	tag = 2 + 1*100;
	MPI_Isend( buf[0], bufsize[0], MPI_INT, 1, tag, MPI_COMM_WORLD, 
		   &r[nmsg++] );
	tag = 3 + 2*100;
	MPI_Issend( buf[1], bufsize[1], MPI_INT, 2, tag, MPI_COMM_WORLD, 
		    &r[nmsg++] );
	tag = 1 + 3*100;
	MPI_Irecv( buf[2], bufsize[2], MPI_INT, 3, tag, MPI_COMM_WORLD, 
		   &r[nmsg++] );
    }
    else if (wrank == 1) {
    }
    else if (wrank == 2) {
    }
    else if (wrank == 3) {
    }

    /* provide a convenient place to wait */
    MPI_Barrier( MPI_COMM_WORLD );
    printf( "Barrier 1 finished\n" );
    
    /* Match up (or cancel) the requests */
    if (wrank == 0) {
	MPI_Waitall( nmsg, r, MPI_STATUSES_IGNORE );
    }
    else if (wrank == 1) {
	tag = 2 + 1*100;
	MPI_Recv( buf[0], bufsize[0], MPI_INT, 0, tag, MPI_COMM_WORLD, 
		  MPI_STATUS_IGNORE );
    }
    else if (wrank == 2) {
	tag = 3 + 2*100;
	MPI_Recv( buf[1], bufsize[1], MPI_INT, 0, tag, MPI_COMM_WORLD, 
		  MPI_STATUS_IGNORE );
    }
    else if (wrank == 3) {
	tag = 1 + 3*100;
	MPI_Send( buf[2], bufsize[2], MPI_INT, 0, tag, MPI_COMM_WORLD );
    }

    MPI_Barrier( MPI_COMM_WORLD );   
    printf( "Barrier 2 finished\n" );

    MPI_Comm_free( &commDup );
    if (commEven != MPI_COMM_NULL) MPI_Comm_free( &commEven );

    MPI_Finalize();
    return 0;
}
