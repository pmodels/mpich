/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Test MPI_Probe() for an intercomm";
#define MAX_DATA_LEN 100

int main( int argc, char *argv[] )
{
    int errs = 0, recvlen, isLeft;
    MPI_Status status;
    int rank, size;
    MPI_Comm  dupcomm, clustercomm, intercomm;
    char buf[MAX_DATA_LEN];

    MTest_Init( &argc, &argv );


    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    if (size != 2) {
	fprintf( stderr, "This test requires two processes." );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    MPI_Comm_dup(MPI_COMM_WORLD, &dupcomm);
    MPI_Comm_split(dupcomm, 0, rank, &clustercomm);

    if(rank > 0){
	MPI_Comm_free(&clustercomm); 
    }

    isLeft = rank % 2;
    MTestGetIntercomm(&intercomm, &isLeft, 2);

    if(rank == 0){
	MPI_Probe(0, 0, intercomm, &status);
	MPI_Get_count(&status, MPI_CHAR, &recvlen);
	buf[0] = '\0';
	MPI_Recv(buf, recvlen, MPI_CHAR, 0, 0, intercomm, &status);
    }else{
	strncpy(buf, "test\0", 5);
	MPI_Send(buf, strlen(buf)+1, MPI_CHAR, 0, 0, intercomm);
    }

    MPI_Comm_free( &dupcomm );
    if(rank == 0){
        MPI_Comm_free( &clustercomm);
    }
    MPI_Comm_free( &intercomm);
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
