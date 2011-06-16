/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test asynchronous I/O w/ multiple completion";
*/

#define SIZE (65536)
#define NUMOPS 10

/* Uses asynchronous I/O. Each process writes to separate files and
   reads them back. The file name is taken as a command-line argument,
   and the process rank is appended to it.*/ 

int main(int argc, char **argv)
{
    int *buf, i, rank, nints, len;
    char *filename, *tmp;
    int errs=0;
    MPI_File fh;
    MPI_Status statuses[NUMOPS];
    MPI_Request requests[NUMOPS];

    MTest_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

/* process 0 takes the file name as a command-line argument and 
   broadcasts it to other processes */
    if (!rank) {
	i = 1;
	while ((i < argc) && strcmp("-fname", *argv)) {
	    i++;
	    argv++;
	}
	if (i >= argc) {
	    /* Use a default filename of testfile */
	    len      = 8;
	    filename = (char *)malloc(len + 10);
	    strcpy( filename, "testfile" );
	    /* 
	    fprintf(stderr, "\n*#  Usage: async_any -fname filename\n\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	    */
	}
	else {
	    argv++;
	    len = (int)strlen(*argv);
	    filename = (char *) malloc(len+10);
	    strcpy(filename, *argv);
	}
	MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(filename, len+10, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    else {
	MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
	filename = (char *) malloc(len+10);
	MPI_Bcast(filename, len+10, MPI_CHAR, 0, MPI_COMM_WORLD);
    }


    buf = (int *) malloc(SIZE);
    nints = SIZE/sizeof(int);
    for (i=0; i<nints; i++) buf[i] = rank*100000 + i;

    /* each process opens a separate file called filename.'myrank' */
    tmp = (char *) malloc(len+10);
    strcpy(tmp, filename);
    sprintf(filename, "%s.%d", tmp, rank);

    MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, 
                  MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, (char*)"native", MPI_INFO_NULL);
    for (i=0; i < NUMOPS; i++) {
	    MPI_File_iwrite(fh, buf, nints, MPI_INT, &(requests[i]));
    }
    MPI_Waitall(NUMOPS, requests, statuses);
    MPI_File_close(&fh);

    /* reopen the file and read the data back */

    for (i=0; i<nints; i++) buf[i] = 0;
    MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, 
                  MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, (char*)"native", MPI_INFO_NULL);
    for (i=0; i < NUMOPS; i++) {
	    MPI_File_iread(fh, buf, nints, MPI_INT, &(requests[i]));
    }
    MPI_Waitall(NUMOPS, requests, statuses);
    MPI_File_close(&fh);

    /* check if the data read is correct */
    for (i=0; i<nints; i++) {
	if (buf[i] != (rank*100000 + i)) {
	    errs++;
	    fprintf(stderr, "Process %d: error, read %d, should be %d\n", rank, buf[i], rank*100000+i);
	}
    }

    free(buf);
    free(filename);
    free(tmp);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0; 
}
