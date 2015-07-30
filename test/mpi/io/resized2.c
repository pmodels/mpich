/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test file views with MPI_Type_create_resized";
*/

int main(int argc, char **argv)
{
    int i, nprocs, len, mpi_errno, buf[2], newbuf[4];
    int errs = 0;
    MPI_Offset size;
    MPI_Aint lb, extent;
    MPI_File fh;
    char *filename;
    MPI_Datatype newtype, newtype1;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs != 1) {
        fprintf(stderr, "Run this program on 1 process\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    i = 1;
    while ((i < argc) && strcmp("-fname", *argv)) {
        i++;
        argv++;
    }
    if (i >= argc) {
        len = 8;
        filename = (char *) malloc(len + 10);
        strcpy(filename, "testfile");
        /*
         * fprintf(stderr, "\n*#  Usage: resized -fname filename\n\n");
         * MPI_Abort(MPI_COMM_WORLD, 1);
         */
    }
    else {
        argv++;
        len = (int) strlen(*argv);
        filename = (char *) malloc(len + 1);
        strcpy(filename, *argv);
    }

    MPI_File_delete(filename, MPI_INFO_NULL);

    /* create a resized type comprising an integer with an lb at sizeof(int) and extent = 3*sizeof(int) */
    lb = sizeof(int);
    extent = 3 * sizeof(int);
    MPI_Type_create_resized(MPI_INT, lb, extent, &newtype1);

    MPI_Type_commit(&newtype1);
    MPI_Type_create_resized(newtype1, lb, extent, &newtype);
    MPI_Type_commit(&newtype);

    /* initialize the file */
    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    for (i = 0; i < 4; i++)
        newbuf[i] = 55;
    MPI_File_write(fh, newbuf, 4, MPI_INT, MPI_STATUS_IGNORE);
    MPI_File_close(&fh);

    /* write 2 ints into file view with resized type */

    buf[0] = 10;
    buf[1] = 20;

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);

    mpi_errno = MPI_File_set_view(fh, 0, MPI_INT, newtype, (char *) "native", MPI_INFO_NULL);
    if (mpi_errno != MPI_SUCCESS)
        errs++;

    MPI_File_write(fh, buf, 2, MPI_INT, MPI_STATUS_IGNORE);

    MPI_File_close(&fh);


    /* read back file view with resized type  and verify */

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    mpi_errno = MPI_File_set_view(fh, 0, MPI_INT, newtype, (char *) "native", MPI_INFO_NULL);
    if (mpi_errno != MPI_SUCCESS)
        errs++;

    for (i = 0; i < 4; i++)
        newbuf[i] = 100;
    MPI_File_read(fh, newbuf, 2, MPI_INT, MPI_STATUS_IGNORE);
    if ((newbuf[0] != 10) || (newbuf[1] != 20) || (newbuf[2] != 100) || (newbuf[3] != 100)) {
        errs++;
        fprintf(stderr,
                "newbuf[0] is %d, should be 10,\n newbuf[1] is %d, should be 20\n newbuf[2] is %d, should be 100,\n newbuf[3] is %d, should be 100,\n",
                newbuf[0], newbuf[1], newbuf[2], newbuf[3]);
    }

    MPI_File_close(&fh);

    /* read file back and verify */

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    MPI_File_get_size(fh, &size);
    if (size != 4 * sizeof(int)) {
        errs++;
        fprintf(stderr, "file size is %lld, should be %d\n", size, (int) (4 * sizeof(int)));
    }

    for (i = 0; i < 4; i++)
        newbuf[i] = 100;
    MPI_File_read(fh, newbuf, 4, MPI_INT, MPI_STATUS_IGNORE);
    if ((newbuf[0] != 10) || (newbuf[3] != 20) || (newbuf[1] != 55) || (newbuf[2] != 55)) {
        errs++;
        fprintf(stderr,
                "newbuf[0] is %d, should be 10,\n newbuf[1] is %d, should be 55,\n newbuf[2] is %d, should be 55,\n newbuf[3] is %d, should be 20\n",
                newbuf[0], newbuf[1], newbuf[2], newbuf[3]);
    }

    MPI_File_close(&fh);

    MPI_Type_free(&newtype1);
    MPI_Type_free(&newtype);
    free(filename);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
