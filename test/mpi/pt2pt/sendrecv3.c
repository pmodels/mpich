/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Head to head send-recv to test backoff in device when large messages are being transferred";
*/

#define  MAX_NMSGS 100
int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, source, dest, partner;
    int i, testnum;
    double tsend;
    static int msgsizes[] = { 100, 1000, 10000, 100000, -1 };
    static int nmsgs[] = { 100, 10, 10, 4 };
    MPI_Comm comm;
    int use_isendrecv = 0;

    if (argc > 1 && strcmp(argv[1], "-isendrecv") == 0) {
        use_isendrecv = 1;
    }

    /* skip the test if MPI_Isendrecv is not available */
    if (use_isendrecv && !MTEST_HAVE_MIN_MPI_VERSION(4, 0)) {
        printf("Test Skipped\n");
        return 0;
    }

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    source = 0;
    dest = 1;
    if (size < 2) {
        printf("This test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (testnum = 0; msgsizes[testnum] > 0; testnum++) {
        if (rank == source || rank == dest) {
            int nmsg = nmsgs[testnum];
            int msgSize = msgsizes[testnum];
            MPI_Request r[MAX_NMSGS + 1];       /* last one is for MPI_Isendrecv */
            int num_requests;
            int *buf[MAX_NMSGS];

            for (i = 0; i < nmsg; i++) {
                buf[i] = (int *) malloc(msgSize * sizeof(int));
                if (!buf[i]) {
                    fprintf(stderr, "Unable to allocate %d bytes\n", msgSize);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                MTEST_VG_MEM_INIT(buf[i], msgSize * sizeof(int));
            }
            partner = (rank + 1) % size;

#if MTEST_HAVE_MIN_MPI_VERSION(4,0)
            if (use_isendrecv) {
                MPI_Isendrecv(MPI_BOTTOM, 0, MPI_INT, partner, 10,
                              MPI_BOTTOM, 0, MPI_INT, partner, 10, comm, &r[nmsg]);
                num_requests = nmsg + 1;
            } else
#endif
            {
                MPI_Sendrecv(MPI_BOTTOM, 0, MPI_INT, partner, 10,
                             MPI_BOTTOM, 0, MPI_INT, partner, 10, comm, MPI_STATUS_IGNORE);
                num_requests = nmsg;
            }
            /* Try to fill up the outgoing message buffers */
            for (i = 0; i < nmsg; i++) {
                MPI_Isend(buf[i], msgSize, MPI_INT, partner, testnum, comm, &r[i]);
            }
            for (i = 0; i < nmsg; i++) {
                MPI_Recv(buf[i], msgSize, MPI_INT, partner, testnum, comm, MPI_STATUS_IGNORE);
            }
            MPI_Waitall(num_requests, r, MPI_STATUSES_IGNORE);

            /* Repeat the test, but make one of the processes sleep */
#if MTEST_HAVE_MIN_MPI_VERSION(4,0)
            if (use_isendrecv) {
                MPI_Isendrecv(MPI_BOTTOM, 0, MPI_INT, partner, 10,
                              MPI_BOTTOM, 0, MPI_INT, partner, 10, comm, &r[nmsg]);
            } else
#endif
            {
                MPI_Sendrecv(MPI_BOTTOM, 0, MPI_INT, partner, 10,
                             MPI_BOTTOM, 0, MPI_INT, partner, 10, comm, MPI_STATUS_IGNORE);
            }
            if (rank == dest)
                MTestSleep(1);
            /* Try to fill up the outgoing message buffers */
            tsend = MPI_Wtime();
            for (i = 0; i < nmsg; i++) {
                MPI_Isend(buf[i], msgSize, MPI_INT, partner, testnum, comm, &r[i]);
            }
            tsend = MPI_Wtime() - tsend;
            for (i = 0; i < nmsg; i++) {
                MPI_Recv(buf[i], msgSize, MPI_INT, partner, testnum, comm, MPI_STATUS_IGNORE);
            }
            MPI_Waitall(num_requests, r, MPI_STATUSES_IGNORE);

            if (tsend > 0.5) {
                printf("Isends for %d messages of size %d took too long (%f seconds)\n", nmsg,
                       msgSize, tsend);
                errs++;
            }
            MTestPrintfMsg(1, "%d Isends for size = %d took %f seconds\n", nmsg, msgSize, tsend);

            for (i = 0; i < nmsg; i++) {
                free(buf[i]);
            }
        }
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
