/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"
#include "test_io.h"

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
    int *buf, i, rank, nints;
    int errs = 0;
    MPI_File fh;
    MPI_Status statuses[NUMOPS];
    MPI_Request requests[NUMOPS];
    INIT_FILENAME;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    GET_TEST_FILENAME_PER_RANK;

    buf = (int *) malloc(SIZE);
    nints = SIZE / sizeof(int);
    for (i = 0; i < nints; i++)
        buf[i] = rank * 100000 + i;

    /* each process opens a separate file called filename.'myrank' */
    MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, (char *) "native", MPI_INFO_NULL);
    for (i = 0; i < NUMOPS; i++) {
        MPI_File_iwrite(fh, buf, nints, MPI_INT, &(requests[i]));
    }
    MPI_Waitall(NUMOPS, requests, statuses);
    MPI_File_close(&fh);

    /* reopen the file and read the data back */

    for (i = 0; i < nints; i++)
        buf[i] = 0;
    MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, (char *) "native", MPI_INFO_NULL);
    for (i = 0; i < NUMOPS; i++) {
        MPI_File_iread(fh, buf, nints, MPI_INT, &(requests[i]));
    }
    MPI_Waitall(NUMOPS, requests, statuses);
    MPI_File_close(&fh);

    /* check if the data read is correct */
    for (i = 0; i < nints; i++) {
        if (buf[i] != (rank * 100000 + i)) {
            errs++;
            fprintf(stderr, "Process %d: error, read %d, should be %d\n", rank, buf[i],
                    rank * 100000 + i);
        }
    }

    free(buf);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
