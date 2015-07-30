/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test contig asynchronous I/O";
*/

#define DEFAULT_SIZE (65536)

/* Uses asynchronous I/O. Each process writes to separate files and
   reads them back. The file name is taken as a command-line argument,
   and the process rank is appended to it.*/

static void handle_error(int errcode, char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

int main(int argc, char **argv)
{
    int *buf, i, rank, nints, len, err;
    char *filename = 0, *tmp;
    int errs = 0;
    int SIZE = DEFAULT_SIZE;
    MPI_File fh;
    MPI_Status status;
#ifdef MPIO_USES_MPI_REQUEST
    MPI_Request request;
#else
    MPIO_Request request;
#endif

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


/* process 0 takes the file name as a command-line argument and
   broadcasts it to other processes */
    if (!rank) {
        i = 1;
        argv++;
        /* Skip unrecognized arguments */
        while (i < argc) {
            if (strcmp(*argv, "-fname") == 0) {
                argv++;
                i++;
                len = (int) strlen(*argv);
                filename = (char *) malloc(len + 10);
                strcpy(filename, *argv);
            }
            else if (strcmp(*argv, "-size") == 0) {
                argv++;
                i++;
                SIZE = strtol(*argv, 0, 10);
                if (errno) {
                    fprintf(stderr, "-size requires a numeric argument\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                else if (SIZE <= 0) {
                    fprintf(stderr, "-size requires a positive value\n");
                }
            }
            else {
                i++;
                argv++;
            }
        }

        if (!filename) {
            /* Use a default filename of testfile */
            len = 8;
            filename = (char *) malloc(len + 10);
            strcpy(filename, "testfile");
        }
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(filename, len + 10, MPI_CHAR, 0, MPI_COMM_WORLD);
        MPI_Bcast(&SIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        filename = (char *) malloc(len + 10);
        MPI_Bcast(filename, len + 10, MPI_CHAR, 0, MPI_COMM_WORLD);
        MPI_Bcast(&SIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }


    /*     printf("Starting (size=%d, file=%s)...\n", SIZE, filename); fflush(stdout); */

    buf = (int *) malloc(SIZE);
    nints = SIZE / sizeof(int);
    for (i = 0; i < nints; i++)
        buf[i] = rank * 100000 + i;

    /* each process opens a separate file called filename.'myrank' */
    tmp = (char *) malloc(len + 10);
    strcpy(tmp, filename);
    sprintf(filename, "%s.%d", tmp, rank);
    free(tmp);

    err = MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,
                        MPI_INFO_NULL, &fh);
    if (err != MPI_SUCCESS)
        handle_error(err, "MPI_File_open");

    err = MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, (char *) "native", MPI_INFO_NULL);
    if (err != MPI_SUCCESS)
        handle_error(err, "MPI_File_set_view");
    err = MPI_File_iwrite(fh, buf, nints, MPI_INT, &request);
    if (err != MPI_SUCCESS)
        handle_error(err, "MPI_File_iwrtie");
#ifdef MPIO_USES_MPI_REQUEST
    MPI_Wait(&request, &status);
#else
    MPIO_Wait(&request, &status);
#endif
    MPI_File_close(&fh);

    /* reopen the file and read the data back */

    for (i = 0; i < nints; i++)
        buf[i] = 0;
    err = MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,
                        MPI_INFO_NULL, &fh);
    if (err != MPI_SUCCESS)
        handle_error(err, "MPI_File_open (read)");
    err = MPI_File_set_view(fh, 0, MPI_INT, MPI_INT, (char *) "native", MPI_INFO_NULL);
    if (err != MPI_SUCCESS)
        handle_error(err, "MPI_File_set_view (read)");
    err = MPI_File_iread(fh, buf, nints, MPI_INT, &request);
    if (err != MPI_SUCCESS)
        handle_error(err, "MPI_File_iread");
#ifdef MPIO_USES_MPI_REQUEST
    MPI_Wait(&request, &status);
#else
    MPIO_Wait(&request, &status);
#endif
    MPI_File_close(&fh);

    /* check if the data read is correct */
    for (i = 0; i < nints; i++) {
        if (buf[i] != (rank * 100000 + i)) {
            errs++;
            if (errs < 25) {
                fprintf(stderr, "Process %d: error, read %d, should be %d\n", rank, buf[i],
                        rank * 100000 + i);
            }
            else if (errs == 25) {
                fprintf(stderr, "Reached maximum number of errors to report\n");
            }
        }
    }

    free(buf);
    free(filename);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
