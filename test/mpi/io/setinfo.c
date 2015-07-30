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

/*
static char MTEST_Descrip[] = "Test file_set_view";
*/

/*
 * access style is explicitly described as modifiable.  values include
 * read_once, read_mostly, write_once, write_mostlye, random
 *
 *
 */
int main(int argc, char *argv[])
{
    int errs = 0, err;
    int buf[10];
    int rank;
    MPI_Comm comm;
    MPI_Status status;
    MPI_File fh;
    MPI_Info infoin, infoout;
    char value[1024];
    int flag, count;

    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Info_create(&infoin);
    MPI_Info_set(infoin, (char *) "access_style", (char *) "write_once,random");
    MPI_File_open(comm, (char *) "testfile", MPI_MODE_RDWR | MPI_MODE_CREATE, infoin, &fh);
    buf[0] = rank;
    err = MPI_File_write_ordered(fh, buf, 1, MPI_INT, &status);
    if (err) {
        errs++;
        MTestPrintError(err);
    }

    MPI_Info_set(infoin, (char *) "access_style", (char *) "read_once");
    err = MPI_File_seek_shared(fh, 0, MPI_SEEK_SET);
    if (err) {
        errs++;
        MTestPrintError(err);
    }

    err = MPI_File_set_info(fh, infoin);
    if (err) {
        errs++;
        MTestPrintError(err);
    }
    MPI_Info_free(&infoin);
    buf[0] = -1;

    err = MPI_File_read_ordered(fh, buf, 1, MPI_INT, &status);
    if (err) {
        errs++;
        MTestPrintError(err);
    }
    MPI_Get_count(&status, MPI_INT, &count);
    if (count != 1) {
        errs++;
        printf("Expected to read one int, read %d\n", count);
    }
    if (buf[0] != rank) {
        errs++;
        printf("Did not read expected value (%d)\n", buf[0]);
    }

    err = MPI_File_get_info(fh, &infoout);
    if (err) {
        errs++;
        MTestPrintError(err);
    }
    MPI_Info_get(infoout, (char *) "access_style", 1024, value, &flag);
    /* Note that an implementation is allowed to ignore the set_info,
     * so we'll accept either the original or the updated version */
    if (!flag) {
        ;
        /*
         * errs++;
         * printf("Access style hint not saved\n");
         */
    }
    else {
        if (strcmp(value, "read_once") != 0 && strcmp(value, "write_once,random") != 0) {
            errs++;
            printf("value for access_style unexpected; is %s\n", value);
        }
    }
    MPI_Info_free(&infoout);
    err = MPI_File_close(&fh);
    if (err) {
        errs++;
        MTestPrintError(err);
    }
    MPI_Barrier(comm);
    MPI_Comm_rank(comm, &rank);
    if (rank == 0) {
        err = MPI_File_delete((char *) "testfile", MPI_INFO_NULL);
        if (err) {
            errs++;
            MTestPrintError(err);
        }
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
