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

/*
static char MTEST_Descrip[] = "Test the handling of BSend operations when a detach occurs before the bsend data has been sent.";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, source, dest;
    unsigned char *buf, *bufp;
    int minsize = 2;
    int i, msgsize, bufsize, outsize;
    unsigned char *msg1, *msg2, *msg3;
    MPI_Comm comm;
    MPI_Status status1, status2, status3;

    MTest_Init(&argc, &argv);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    msgsize = 128 * 1024;
    msg1 = (unsigned char *) malloc(3 * msgsize);
    msg2 = msg1 + msgsize;
    msg3 = msg2 + msgsize;
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        source = 0;
        dest = size - 1;

        /* Here is the test:  The sender */
        if (rank == source) {
            /* Get a bsend buffer.  Make it large enough that the Bsend
             * internals will (probably) not use a eager send for the data.
             * Have three such messages */
            bufsize = 3 * (MPI_BSEND_OVERHEAD + msgsize);
            buf = (unsigned char *) malloc(bufsize);
            if (!buf) {
                fprintf(stderr, "Unable to allocate a buffer of %d bytes\n", bufsize);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Buffer_attach(buf, bufsize);

            /* Initialize the buffers */
            for (i = 0; i < msgsize; i++) {
                msg1[i] = 0xff ^ (i & 0xff);
                msg2[i] = 0xff ^ (3 * i & 0xff);
                msg3[i] = 0xff ^ (5 * i & 0xff);
            }

            /* Initiate the bsends */
            MPI_Bsend(msg1, msgsize, MPI_UNSIGNED_CHAR, dest, 0, comm);
            MPI_Bsend(msg2, msgsize, MPI_UNSIGNED_CHAR, dest, 0, comm);
            MPI_Bsend(msg3, msgsize, MPI_UNSIGNED_CHAR, dest, 0, comm);

            /* Synchronize with our partner */
            MPI_Sendrecv(NULL, 0, MPI_UNSIGNED_CHAR, dest, 10,
                         NULL, 0, MPI_UNSIGNED_CHAR, dest, 10, comm, MPI_STATUS_IGNORE);

            /* Detach the buffers.  There should be pending operations */
            MPI_Buffer_detach(&bufp, &outsize);
            if (bufp != buf) {
                fprintf(stderr, "Wrong buffer returned\n");
                errs++;
            }
            if (outsize != bufsize) {
                fprintf(stderr, "Wrong buffer size returned\n");
                errs++;
            }
        }
        else if (rank == dest) {
            double tstart;

            /* Clear the message buffers */
            for (i = 0; i < msgsize; i++) {
                msg1[i] = 0;
                msg2[i] = 0;
                msg3[i] = 0;
            }

            /* Wait for the synchronize */
            MPI_Sendrecv(NULL, 0, MPI_UNSIGNED_CHAR, source, 10,
                         NULL, 0, MPI_UNSIGNED_CHAR, source, 10, comm, MPI_STATUS_IGNORE);

            /* Wait 2 seconds */
            tstart = MPI_Wtime();
            while (MPI_Wtime() - tstart < 2.0);

            /* Now receive the messages */
            MPI_Recv(msg1, msgsize, MPI_UNSIGNED_CHAR, source, 0, comm, &status1);
            MPI_Recv(msg2, msgsize, MPI_UNSIGNED_CHAR, source, 0, comm, &status2);
            MPI_Recv(msg3, msgsize, MPI_UNSIGNED_CHAR, source, 0, comm, &status3);

            /* Check that we have the correct data */
            for (i = 0; i < msgsize; i++) {
                if (msg1[i] != (0xff ^ (i & 0xff))) {
                    if (errs < 10) {
                        fprintf(stderr, "msg1[%d] = %d\n", i, msg1[i]);
                    }
                    errs++;
                }
                if (msg2[i] != (0xff ^ (3 * i & 0xff))) {
                    if (errs < 10) {
                        fprintf(stderr, "msg2[%d] = %d\n", i, msg2[i]);
                    }
                    errs++;
                }
                if (msg3[i] != (0xff ^ (5 * i & 0xff))) {
                    if (errs < 10) {
                        fprintf(stderr, "msg2[%d] = %d\n", i, msg2[i]);
                    }
                    errs++;
                }
            }

        }


        MTestFreeComm(&comm);
    }
    free(msg1);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
