/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

/* This is a temporary #ifdef to control whether we test this functionality.  A
 * configure-test or similar would be better.  Eventually the MPI-3 standard
 * will be released and this can be gated on a MPI_VERSION check */
#if !defined(USE_STRICT_MPI) && defined(MPICH2)
#define TEST_MPROBE_ROUTINES 1
#endif

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__); \
            }                                                                     \
        }                                                                         \
    } while (0)

int main(int argc, char **argv)
{
    int errs = 0;
    int found;
    int rank, size;
    int sendbuf[8], recvbuf[8];
    int count;
    MPIX_Message msg;
    MPI_Request rreq;
    MPI_Status s1, s2;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("this test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* all processes besides ranks 0 & 1 aren't used by this test */
    if (rank >= 2) {
        goto epilogue;
    }

#ifdef TEST_MPROBE_ROUTINES
    /* test 0: simple send & mprobe+mrecv */
    if (rank == 0) {
        sendbuf[0] = 0xdeadbeef;
        sendbuf[1] = 0xfeedface;
        MPI_Send(sendbuf, 2, MPI_INT, 1, 5, MPI_COMM_WORLD);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPIX_MESSAGE_NULL;
        MPIX_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        MPIX_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPIX_MESSAGE_NULL);
    }

    /* test 1: simple send & mprobe+imrecv */
    if (rank == 0) {
        sendbuf[0] = 0xdeadbeef;
        sendbuf[1] = 0xfeedface;
        MPI_Send(sendbuf, 2, MPI_INT, 1, 5, MPI_COMM_WORLD);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPIX_MESSAGE_NULL;
        MPIX_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        rreq = MPI_REQUEST_NULL;
        MPIX_Imrecv(recvbuf, count, MPI_INT, &msg, &rreq);
        check(rreq != MPI_REQUEST_NULL);
        MPI_Wait(&rreq, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPIX_MESSAGE_NULL);
    }

    /* test 2: simple send & improbe+mrecv */
    if (rank == 0) {
        sendbuf[0] = 0xdeadbeef;
        sendbuf[1] = 0xfeedface;
        MPI_Send(sendbuf, 2, MPI_INT, 1, 5, MPI_COMM_WORLD);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPIX_MESSAGE_NULL;
        do {
            check(msg == MPIX_MESSAGE_NULL);
            MPIX_Improbe(0, 5, MPI_COMM_WORLD, &found, &msg, &s1);
        } while (!found);
        check(msg != MPIX_MESSAGE_NULL);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        MPIX_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPIX_MESSAGE_NULL);
    }

    /* test 3: simple send & improbe+imrecv */
    if (rank == 0) {
        sendbuf[0] = 0xdeadbeef;
        sendbuf[1] = 0xfeedface;
        MPI_Send(sendbuf, 2, MPI_INT, 1, 5, MPI_COMM_WORLD);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPIX_MESSAGE_NULL;
        do {
            check(msg == MPIX_MESSAGE_NULL);
            MPIX_Improbe(0, 5, MPI_COMM_WORLD, &found, &msg, &s1);
        } while (!found);
        check(msg != MPIX_MESSAGE_NULL);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        rreq = MPI_REQUEST_NULL;
        MPIX_Imrecv(recvbuf, count, MPI_INT, &msg, &rreq);
        check(rreq != MPI_REQUEST_NULL);
        MPI_Wait(&rreq, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPIX_MESSAGE_NULL);
    }

    /* TODO MPI_ANY_SOURCE and MPI_ANY_TAG should be tested as well */
    /* TODO a full range of message sizes should be tested too */
    /* TODO threaded tests are also needed, but they should go in a separate
     * program */

#endif /* TEST_MPROBE_ROUTINES */

epilogue:
    MPI_Reduce((rank == 0 ? MPI_IN_PLACE : &errs), &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }

    MPI_Finalize();

    return 0;
}

