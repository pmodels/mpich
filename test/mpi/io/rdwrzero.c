/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

/*
static char MTEST_Descrip[] = "Test reading and writing zero bytes (set status correctly)";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int size, rank, i, *buf, count, rc;
    MPI_File fh;
    MPI_Comm comm;
    MPI_Status status;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    rc = MPI_File_open(comm, (char *) "test.ord",
                       MPI_MODE_RDWR | MPI_MODE_CREATE |
                       MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh);
    if (rc) {
        MTestPrintErrorMsg("File_open", rc);
        errs++;
        /* If the open fails, there isn't anything else that we can do */
        goto fn_fail;
    }


    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    buf = (int *) malloc(size * sizeof(int));
    buf[0] = rank;
    /* Write to file */
    rc = MPI_File_write_ordered(fh, buf, 1, MPI_INT, &status);
    if (rc) {
        MTestPrintErrorMsg("File_write_ordered", rc);
        errs++;
    }
    else {
        MPI_Get_count(&status, MPI_INT, &count);
        if (count != 1) {
            errs++;
            fprintf(stderr, "Wrong count (%d) on write-ordered\n", count);
        }
    }

    /* Set the individual pointer to 0, since we want to use a read_all */
    MPI_File_seek(fh, 0, MPI_SEEK_SET);

    /* Read nothing (check status) */
    memset(&status, 0xff, sizeof(MPI_Status));
    MPI_File_read(fh, buf, 0, MPI_INT, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    if (count != 0) {
        errs++;
        fprintf(stderr, "Count not zero (%d) on read\n", count);
    }

    /* Write nothing (check status) */
    memset(&status, 0xff, sizeof(MPI_Status));
    MPI_File_write(fh, buf, 0, MPI_INT, &status);
    if (count != 0) {
        errs++;
        fprintf(stderr, "Count not zero (%d) on write\n", count);
    }

    /* Read shared nothing (check status) */
    MPI_File_seek_shared(fh, 0, MPI_SEEK_SET);
    /* Read nothing (check status) */
    memset(&status, 0xff, sizeof(MPI_Status));
    MPI_File_read_shared(fh, buf, 0, MPI_INT, &status);
    MPI_Get_count(&status, MPI_INT, &count);
    if (count != 0) {
        errs++;
        fprintf(stderr, "Count not zero (%d) on read shared\n", count);
    }

    /* Write nothing (check status) */
    memset(&status, 0xff, sizeof(MPI_Status));
    MPI_File_write_shared(fh, buf, 0, MPI_INT, &status);
    if (count != 0) {
        errs++;
        fprintf(stderr, "Count not zero (%d) on write\n", count);
    }

    MPI_Barrier(comm);

    MPI_File_seek_shared(fh, 0, MPI_SEEK_SET);
    for (i = 0; i < size; i++)
        buf[i] = -1;
    MPI_File_read_ordered(fh, buf, 1, MPI_INT, &status);
    if (buf[0] != rank) {
        errs++;
        fprintf(stderr, "%d: buf = %d\n", rank, buf[0]);
    }

    free(buf);

    MPI_File_close(&fh);

  fn_fail:
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
