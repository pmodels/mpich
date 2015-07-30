/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test reading and writing ordered output";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int size, rank, i, *buf, rc;
    MPI_File fh;
    MPI_Comm comm;
    MPI_Status status;
    MPI_Request request;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_File_open(comm, (char *) "test.ord",
                  MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh);

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    buf = (int *) malloc(size * sizeof(int));
    buf[0] = rank;
    rc = MPI_File_write_ordered(fh, buf, 1, MPI_INT, &status);
    if (rc != MPI_SUCCESS) {
        MTestPrintErrorMsg("File_write_ordered", rc);
        errs++;
    }
    /* make sure all writes finish before we seek/read */
    MPI_Barrier(comm);

    /* Set the individual pointer to 0, since we want to use a iread_all */
    MPI_File_seek(fh, 0, MPI_SEEK_SET);
    rc = MPI_File_iread_all(fh, buf, size, MPI_INT, &request);
    if (rc != MPI_SUCCESS) {
        MTestPrintErrorMsg("File_iread_all", rc);
        errs++;
    }
    MPI_Wait(&request, &status);

    for (i = 0; i < size; i++) {
        if (buf[i] != i) {
            errs++;
            fprintf(stderr, "%d: buf[%d] = %d\n", rank, i, buf[i]);
        }
    }

    MPI_File_seek_shared(fh, 0, MPI_SEEK_SET);
    for (i = 0; i < size; i++)
        buf[i] = -1;
    MPI_File_read_ordered(fh, buf, 1, MPI_INT, &status);
    if (buf[0] != rank) {
        errs++;
        fprintf(stderr, "%d: buf[0] = %d\n", rank, buf[0]);
    }

    free(buf);
    MPI_File_close(&fh);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
