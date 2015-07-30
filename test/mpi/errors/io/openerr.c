/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpitest.h"

/*
 * openerr - Test that errors on file open are handled correctly and that the
 * returned error message is accessible
 */
#define BUFLEN 10

int main(int argc, char *argv[])
{
    MPI_File fh;
    char emsg[MPI_MAX_ERROR_STRING];
    int emsglen, err, ec, errs = 0;
    int amode, rank;
    char *name = 0;
    MPI_Status st;
    int outbuf[BUFLEN], inbuf[BUFLEN];

    MTest_Init(&argc, &argv);

    name = "not-a-file-to-use";
    /* Try to open a file that does/should not exist */
    /* Note that no error message should be printed by MPI_File_open,
     * even when there is an error */
    err = MPI_File_open(MPI_COMM_WORLD, name, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    if (err == MPI_SUCCESS) {
        errs++;
        printf("Did not return error when opening a file that does not exist\n");
        MPI_File_close(&fh);
        MPI_File_delete(name, MPI_INFO_NULL);
    }
    else {
        MPI_Error_class(err, &ec);
        MPI_Error_string(err, emsg, &emsglen);
        MTestPrintfMsg(2, "Error msg from open: %s\n", emsg);
        if (ec != MPI_ERR_NO_SUCH_FILE && ec != MPI_ERR_IO) {
            errs++;
            printf("Did not return class ERR_NO_SUCH_FILE or ERR_IO\n");
            printf("Returned class %d, message %s\n", ec, emsg);
        }
    }

    /* Now, create a file, write data into it; close, then reopen as
     * read only and try to write to it */

    amode = MPI_MODE_CREATE | MPI_MODE_WRONLY;
    name = "mpio-test-openerrs";

    err = MPI_File_open(MPI_COMM_WORLD, name, amode, MPI_INFO_NULL, &fh);
    if (err) {
        errs++;
        MTestPrintErrorMsg("Unable to open file for writing", err);
    }
    else {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        memset(outbuf, 'A' + rank, BUFLEN);

        err = MPI_File_write_at(fh, rank * BUFLEN, outbuf, BUFLEN, MPI_BYTE, &st);
        if (err) {
            errs++;
            MTestPrintErrorMsg("Unable to write file", err);
        }
        MPI_File_close(&fh);
    }

    /* Now, open for read only, and delete on close */
    amode = MPI_MODE_RDONLY | MPI_MODE_DELETE_ON_CLOSE;

    err = MPI_File_open(MPI_COMM_WORLD, name, amode, MPI_INFO_NULL, &fh);
    if (err) {
        errs++;
        MTestPrintErrorMsg("Unable to reopen file for reading", err);
    }
    else {
        /* Try to read it */

        /* Clear buffer before reading into it */

        memset(inbuf, 0, BUFLEN);

        err = MPI_File_read_at(fh, rank * BUFLEN, inbuf, BUFLEN, MPI_BYTE, &st);
        if (err) {
            errs++;
            MTestPrintErrorMsg("Unable to read file", err);
        }

        /* Try to write it (should fail) */
        err = MPI_File_write_at(fh, rank * BUFLEN, outbuf, BUFLEN, MPI_BYTE, &st);
        if (err == MPI_SUCCESS) {
            errs++;
            printf("Write operation succeeded to read-only file\n");
        }
        else {
            /* Look at error class */
            MPI_Error_class(err, &ec);
            if (ec != MPI_ERR_READ_ONLY && ec != MPI_ERR_ACCESS) {
                errs++;
                printf("Unexpected error class %d when writing to read-only file\n", ec);
                MTestPrintErrorMsg("Error msg is: ", err);
            }
        }
        err = MPI_File_close(&fh);
        if (err) {
            errs++;
            MTestPrintErrorMsg("Failed to close", err);
        }

        /* No MPI_Barrier is required here */

        /*
         *  Test open without CREATE to see if DELETE_ON_CLOSE worked.
         *  This should fail if file was deleted correctly.
         */

        amode = MPI_MODE_RDONLY;
        err = MPI_File_open(MPI_COMM_WORLD, name, amode, MPI_INFO_NULL, &fh);
        if (err == MPI_SUCCESS) {
            errs++;
            printf("File was not deleted!\n");
            MPI_File_close(&fh);
        }
        else {
            MPI_Error_class(err, &ec);
            if (ec != MPI_ERR_NO_SUCH_FILE && ec != MPI_ERR_IO) {
                errs++;
                printf("Did not return class ERR_NO_SUCH_FILE or ERR_IO\n");
                printf("Returned class %d, message %s\n", ec, emsg);
            }
        }
    }
    /* */

    /* Find out how many errors we saw */
    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
