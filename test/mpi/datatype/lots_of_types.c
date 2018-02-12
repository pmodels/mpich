/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

/*
   The default behavior of the test routines should be to briefly indicate
   the cause of any errors - in this test, that means that verbose needs
   to be set. Verbose should turn on output that is independent of error
   levels.
*/
static int verbose = 1;

int main(int argc, char *argv[]);
int parse_args(int argc, char **argv);
int lots_of_types_test(void);

struct test_struct_1 {
    int a, b, c, d;
};

int main(int argc, char *argv[])
{
    int err, errs = 0;

    /* Initialize MPI */
    MTest_Init(&argc, &argv);
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    err = lots_of_types_test();
    if (verbose && err)
        fprintf(stderr, "error in lots_of_types_test\n");
    errs += err;

    /* print message and exit */
    MTest_Finalize(errs);

    MPI_Finalize();
    return 0;
}

/* this test allocates 1024 indexed datatypes with 1024 distinct blocks
 * each.  it's possible that a low memory machine will run out of memory
 * running this test; it appears to take ~25MB of memory at this time.
 * -- Rob Ross, 11/2/2005
 */
#define NUM_DTYPES 1024
#define NUM_BLOCKS 1024
int lots_of_types_test(void)
{
    int err, errs = 0;
    int i;
    MPI_Datatype mytypes[NUM_DTYPES];

    int sendbuf[4] = { 1, 2, 3, 4 };

    int count, elements;
    MPI_Request request;
    MPI_Status status;

    /* note: first element of struct has zero blklen and should be dropped */
    int disps[NUM_BLOCKS];
    int blks[NUM_BLOCKS];

    for (i = 0; i < NUM_DTYPES; i++)
        mytypes[i] = MPI_DATATYPE_NULL;

    for (i = 0; i < NUM_DTYPES; i++) {
        int j;

        disps[0] = 0;
        blks[0] = 4;

        for (j = 1; j < NUM_BLOCKS; j++) {
            disps[j] = 4 * j;
            blks[j] = (j % 3) + 1;
        }

        err = MPI_Type_indexed(NUM_BLOCKS, blks, disps, MPI_INT, &mytypes[i]);
        if (err != MPI_SUCCESS) {
            errs++;
            if (verbose) {
                fprintf(stderr, "MPI_Type_indexed returned error on type %d\n", i);
            }
            mytypes[i] = MPI_DATATYPE_NULL;
            goto fn_exit;
        }

        MPI_Type_commit(&mytypes[i]);
    }

    for (i = 0; i < NUM_DTYPES; i++) {
        int j;
        int recvbuf[4] = { -1, -1, -1, -1 };

        /* we will only receive 4 ints, so short buffer is ok */
        err = MPI_Irecv(recvbuf, 1, mytypes[i], 0, 0, MPI_COMM_SELF, &request);
        if (err != MPI_SUCCESS) {
            errs++;
            if (verbose) {
                fprintf(stderr, "MPI_Irecv returned error\n");
            }
        }

        err = MPI_Send(sendbuf, 4, MPI_INT, 0, 0, MPI_COMM_SELF);
        if (err != MPI_SUCCESS) {
            errs++;
            if (verbose) {
                fprintf(stderr, "MPI_Send returned error\n");
            }
        }

        err = MPI_Wait(&request, &status);
        if (err != MPI_SUCCESS) {
            errs++;
            if (verbose) {
                fprintf(stderr, "MPI_Wait returned error\n");
            }
        }

        /* verify data */
        for (j = 0; j < 4; j++) {
            if (recvbuf[j] != sendbuf[j]) {
                errs++;
                if (verbose) {
                    fprintf(stderr, "recvbuf[%d] = %d; should be %d\n", j, recvbuf[j], sendbuf[j]);
                }
            }
        }

        /* verify count and elements */
        err = MPI_Get_count(&status, mytypes[i], &count);
        if (err != MPI_SUCCESS) {
            errs++;
            if (verbose) {
                fprintf(stderr, "MPI_Get_count returned error\n");
            }
        }
        if (count != MPI_UNDEFINED) {
            errs++;
            if (verbose) {
                fprintf(stderr, "count = %d; should be MPI_UNDEFINED (%d)\n", count, MPI_UNDEFINED);
            }
        }

        err = MPI_Get_elements(&status, mytypes[i], &elements);
        if (err != MPI_SUCCESS) {
            errs++;
            if (verbose) {
                fprintf(stderr, "MPI_Get_elements returned error\n");
            }
        }
        if (elements != 4) {
            errs++;
            if (verbose) {
                fprintf(stderr, "elements = %d; should be 4\n", elements);
            }
        }
    }

  fn_exit:
    for (i = 0; i < NUM_DTYPES; i++) {
        if (mytypes[i] != MPI_DATATYPE_NULL)
            MPI_Type_free(&mytypes[i]);
    }

    return errs;
}


int parse_args(int argc, char **argv)
{
    /*
     * int ret;
     *
     * while ((ret = getopt(argc, argv, "v")) >= 0)
     * {
     * switch (ret) {
     * case 'v':
     * verbose = 1;
     * break;
     * }
     * }
     */
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
        verbose = 1;
    return 0;
}
