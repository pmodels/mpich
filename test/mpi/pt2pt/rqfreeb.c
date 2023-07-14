/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

#define LARGE 10000
int large_send_buf[LARGE];

/* Test Ibsend and Request_free */
int main(int argc, char *argv[])
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int dest = 1, src = 0, tag = 1;
    int s1;
    char *buf, *bbuf;
    int smsg[5], rmsg[6];
    int errs = 0, rank, size;
    int bufsize, bsize;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (src >= size || dest >= size) {
        int r = src;
        if (dest > r)
            r = dest;
        fprintf(stderr, "This program requires %d processes\n", r - 1);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == src) {
        MPI_Request r;

        MPI_Barrier(MPI_COMM_WORLD);

        /* According to the standard, we must use the PACK_SIZE length of each
         * message in the computation of the message buffer size */
        MPI_Pack_size(5, MPI_INT, comm, &s1);
        bufsize = MPI_BSEND_OVERHEAD + s1 + 2000;
        buf = (char *) malloc(bufsize);
        MPI_Buffer_attach(buf, bufsize);

        MTestPrintfMsg(10, "About create and free Isend request\n");
        smsg[0] = 10;
        MPI_Isend(&smsg[0], 1, MPI_INT, dest, tag, comm, &r);
        MPI_Request_free(&r);
        if (r != MPI_REQUEST_NULL) {
            errs++;
            fprintf(stderr, "Request not set to NULL after request free\n");
        }
        MTestPrintfMsg(10, "About create and free Ibsend request\n");
        smsg[1] = 11;
        MPI_Ibsend(&smsg[1], 1, MPI_INT, dest, tag + 1, comm, &r);
        MPI_Request_free(&r);
        if (r != MPI_REQUEST_NULL) {
            errs++;
            fprintf(stderr, "Request not set to NULL after request free\n");
        }
        MTestPrintfMsg(10, "About create and free Issend request\n");
        smsg[2] = 12;
        MPI_Issend(&smsg[2], 1, MPI_INT, dest, tag + 2, comm, &r);
        MPI_Request_free(&r);
        if (r != MPI_REQUEST_NULL) {
            errs++;
            fprintf(stderr, "Request not set to NULL after request free\n");
        }
        MTestPrintfMsg(10, "About create and free Irsend request\n");
        smsg[3] = 13;
        MPI_Irsend(&smsg[3], 1, MPI_INT, dest, tag + 3, comm, &r);
        MPI_Request_free(&r);
        if (r != MPI_REQUEST_NULL) {
            errs++;
            fprintf(stderr, "Request not set to NULL after request free\n");
        }
        smsg[4] = 14;
        MPI_Isend(&smsg[4], 1, MPI_INT, dest, tag + 4, comm, &r);
        MPI_Wait(&r, MPI_STATUS_IGNORE);

        /* We can't guarantee that messages arrive until the detach */
        MPI_Buffer_detach(&bbuf, &bsize);
        free(buf);

        /* Send a large message that may require handshake, e.g. RNDV in shm,
         * and test receive using truncated buffer and free without waiting. */
        MPI_Isend(large_send_buf, LARGE, MPI_INT, dest, tag + 5, comm, &r);
        MPI_Wait(&r, MPI_STATUS_IGNORE);
    }

    if (rank == dest) {
        MPI_Request r[6];
        int i;

        for (i = 0; i < 6; i++) {
            MPI_Irecv(&rmsg[i], 1, MPI_INT, src, tag + i, comm, &r[i]);
        }
        if (rank != src)        /* Just in case rank == src */
            MPI_Barrier(MPI_COMM_WORLD);

        for (i = 0; i < 4; i++) {
            MPI_Wait(&r[i], MPI_STATUS_IGNORE);
            if (rmsg[i] != 10 + i) {
                errs++;
                fprintf(stderr, "message %d (%d) should be %d\n", i, rmsg[i], 10 + i);
            }
        }
        /* Previously, this test was thought to be non-standard behavior. However,
         * recent MPI Forum discussions have concluded that freeing a request without
         * explicit completion via MPI_TEST/MPI_WAIT is allowed. The implementation is
         * required to complete the request during MPI_FINALIZE. (2021-08-24)
         */
        MTestPrintfMsg(10, "About  free Irecv request\n");
        MPI_Request_free(&r[4]);
        MPI_Request_free(&r[5]);
    }

    if (rank != dest && rank != src) {
        MPI_Barrier(MPI_COMM_WORLD);
    }


    MTest_Finalize(errs);


    return MTestReturnValue(errs);
}
