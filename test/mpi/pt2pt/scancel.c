/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of various send cancel calls";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, source, dest;
    MPI_Comm comm;
    MPI_Status status;
    MPI_Request req;
    static int bufsizes[4] = { 1, 100, 10000, 1000000 };
    char *buf;
#ifdef TEST_IRSEND
    int veryPicky = 0;          /* Set to 1 to test "quality of implementation" in
                                 * a tricky part of cancel */
#endif
    int cs, flag, n;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    source = 0;
    dest = size - 1;

    MTestPrintfMsg(1, "Starting scancel test\n");
    for (cs = 0; cs < 4; cs++) {
        if (rank == 0) {
            n = bufsizes[cs];
            buf = (char *) malloc(n);
            if (!buf) {
                fprintf(stderr, "Unable to allocate %d bytes\n", n);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            MTestPrintfMsg(1, "(%d) About to create isend and cancel\n", cs);
            MPI_Isend(buf, n, MPI_CHAR, dest, cs + n + 1, comm, &req);
            MPI_Cancel(&req);
            MPI_Wait(&req, &status);
            MTestPrintfMsg(1, "Completed wait on isend\n");
            MPI_Test_cancelled(&status, &flag);
            if (!flag) {
                errs++;
                printf("Failed to cancel an Isend request\n");
                fflush(stdout);
            }
            else {
                n = 0;
            }
            /* Send the size, zero for successfully cancelled */
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            /* Send the tag so the message can be received */
            n = cs + n + 1;
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            free(buf);
        }
        else if (rank == dest) {
            int nn, tag;
            char *btemp;
            MPI_Recv(&nn, 1, MPI_INT, 0, 123, comm, &status);
            MPI_Recv(&tag, 1, MPI_INT, 0, 123, comm, &status);
            if (nn > 0) {
                /* If the message was not cancelled, receive it here */
                btemp = (char *) malloc(nn);
                if (!btemp) {
                    fprintf(stderr, "Unable to allocate %d bytes\n", nn);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                MPI_Recv(btemp, nn, MPI_CHAR, 0, tag, comm, &status);
                free(btemp);
            }
        }
        MPI_Barrier(comm);

        if (rank == 0) {
            char *bsendbuf;
            int bsendbufsize;
            int bf, bs;
            n = bufsizes[cs];
            buf = (char *) malloc(n);
            if (!buf) {
                fprintf(stderr, "Unable to allocate %d bytes\n", n);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            bsendbufsize = n + MPI_BSEND_OVERHEAD;
            bsendbuf = (char *) malloc(bsendbufsize);
            if (!bsendbuf) {
                fprintf(stderr, "Unable to allocate %d bytes for bsend\n", n);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            MPI_Buffer_attach(bsendbuf, bsendbufsize);
            MTestPrintfMsg(1, "About to create and cancel ibsend\n");
            MPI_Ibsend(buf, n, MPI_CHAR, dest, cs + n + 2, comm, &req);
            MPI_Cancel(&req);
            MPI_Wait(&req, &status);
            MPI_Test_cancelled(&status, &flag);
            if (!flag) {
                errs++;
                printf("Failed to cancel an Ibsend request\n");
                fflush(stdout);
            }
            else {
                n = 0;
            }
            /* Send the size, zero for successfully cancelled */
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            /* Send the tag so the message can be received */
            n = cs + n + 2;
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            free(buf);
            MPI_Buffer_detach(&bf, &bs);
            free(bsendbuf);
        }
        else if (rank == dest) {
            int nn, tag;
            char *btemp;
            MPI_Recv(&nn, 1, MPI_INT, 0, 123, comm, &status);
            MPI_Recv(&tag, 1, MPI_INT, 0, 123, comm, &status);
            if (nn > 0) {
                /* If the message was not cancelled, receive it here */
                btemp = (char *) malloc(nn);
                if (!btemp) {
                    fprintf(stderr, "Unable to allocate %d bytes\n", nn);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                MPI_Recv(btemp, nn, MPI_CHAR, 0, tag, comm, &status);
                free(btemp);
            }
        }
        MPI_Barrier(comm);

        /* Because this test is erroneous, we do not perform it unless
         * TEST_IRSEND is defined.  */
#ifdef TEST_IRSEND
        /* We avoid ready send to self because an implementation
         * is free to detect the error in delivering a message to
         * itself without a pending receive; we could also check
         * for an error return from the MPI_Irsend */
        if (rank == 0 && dest != rank) {
            n = bufsizes[cs];
            buf = (char *) malloc(n);
            if (!buf) {
                fprintf(stderr, "Unable to allocate %d bytes\n", n);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            MTestPrintfMsg(1, "About to create and cancel irsend\n");
            MPI_Irsend(buf, n, MPI_CHAR, dest, cs + n + 3, comm, &req);
            MPI_Cancel(&req);
            MPI_Wait(&req, &status);
            MPI_Test_cancelled(&status, &flag);
            /* This can be pretty ugly.  The standard is clear (Section 3.8)
             * that either a sent message is received or the
             * sent message is successfully cancelled.  Since this message
             * can never be received, the cancel must complete
             * successfully.
             *
             * However, since there is no matching receive, this
             * program is erroneous.  In this case, we can't really
             * flag this as an error */
            if (!flag && veryPicky) {
                errs++;
                printf("Failed to cancel an Irsend request\n");
                fflush(stdout);
            }
            if (flag) {
                n = 0;
            }
            /* Send the size, zero for successfully cancelled */
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            /* Send the tag so the message can be received */
            n = cs + n + 3;
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            free(buf);
        }
        else if (rank == dest) {
            int n, tag;
            char *btemp;
            MPI_Recv(&n, 1, MPI_INT, 0, 123, comm, &status);
            MPI_Recv(&tag, 1, MPI_INT, 0, 123, comm, &status);
            if (n > 0) {
                /* If the message was not cancelled, receive it here */
                btemp = (char *) malloc(n);
                if (!btemp) {
                    fprintf(stderr, "Unable to allocate %d bytes\n", n);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                MPI_Recv(btemp, n, MPI_CHAR, 0, tag, comm, &status);
                free(btemp);
            }
        }
        MPI_Barrier(comm);
#endif

        if (rank == 0) {
            n = bufsizes[cs];
            buf = (char *) malloc(n);
            if (!buf) {
                fprintf(stderr, "Unable to allocate %d bytes\n", n);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            MTestPrintfMsg(1, "About to create and cancel issend\n");
            MPI_Issend(buf, n, MPI_CHAR, dest, cs + n + 4, comm, &req);
            MPI_Cancel(&req);
            MPI_Wait(&req, &status);
            MPI_Test_cancelled(&status, &flag);
            if (!flag) {
                errs++;
                printf("Failed to cancel an Issend request\n");
                fflush(stdout);
            }
            else {
                n = 0;
            }
            /* Send the size, zero for successfully cancelled */
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            /* Send the tag so the message can be received */
            n = cs + n + 4;
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
            free(buf);
        }
        else if (rank == dest) {
            int nn, tag;
            char *btemp;
            MPI_Recv(&nn, 1, MPI_INT, 0, 123, comm, &status);
            MPI_Recv(&tag, 1, MPI_INT, 0, 123, comm, &status);
            if (nn > 0) {
                /* If the message was not cancelled, receive it here */
                btemp = (char *) malloc(nn);
                if (!btemp) {
                    fprintf(stderr, "Unable to allocate %d bytes\n", nn);
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }
                MPI_Recv(btemp, nn, MPI_CHAR, 0, tag, comm, &status);
                free(btemp);
            }
        }
        MPI_Barrier(comm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
