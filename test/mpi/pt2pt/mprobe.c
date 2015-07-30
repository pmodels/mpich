/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

/* This is a temporary #ifdef to control whether we test this functionality.  A
 * configure-test or similar would be better.  Eventually the MPI-3 standard
 * will be released and this can be gated on a MPI_VERSION check */
#if !defined(USE_STRICT_MPI) && defined(MPICH)
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

#define LARGE_DIM 512
#define LARGE_SZ (LARGE_DIM * LARGE_DIM)

int main(int argc, char **argv)
{
    int errs = 0;
    int found, completed;
    int rank, size;
    int *sendbuf = NULL, *recvbuf = NULL;
    int count, i;
#ifdef TEST_MPROBE_ROUTINES
    MPI_Message msg;
#endif
    MPI_Request rreq;
    MPI_Status s1, s2;
    MPI_Datatype vectype;

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
    sendbuf = (int *) malloc(LARGE_SZ * sizeof(int));
    recvbuf = (int *) malloc(LARGE_SZ * sizeof(int));
    if (sendbuf == NULL || recvbuf == NULL) {
        printf("Error in memory allocation\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

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

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg != MPI_MESSAGE_NULL);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
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

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg != MPI_MESSAGE_NULL);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        rreq = MPI_REQUEST_NULL;
        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Imrecv(recvbuf, count, MPI_INT, &msg, &rreq);
        check(rreq != MPI_REQUEST_NULL);
        MPI_Wait(&rreq, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
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

        msg = MPI_MESSAGE_NULL;
        do {
            check(msg == MPI_MESSAGE_NULL);
            MPI_Improbe(0, 5, MPI_COMM_WORLD, &found, &msg, &s1);
        } while (!found);
        check(msg != MPI_MESSAGE_NULL);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
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

        msg = MPI_MESSAGE_NULL;
        do {
            check(msg == MPI_MESSAGE_NULL);
            MPI_Improbe(0, 5, MPI_COMM_WORLD, &found, &msg, &s1);
        } while (!found);
        check(msg != MPI_MESSAGE_NULL);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        rreq = MPI_REQUEST_NULL;
        MPI_Imrecv(recvbuf, count, MPI_INT, &msg, &rreq);
        check(rreq != MPI_REQUEST_NULL);
        MPI_Wait(&rreq, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
    }

    /* test 4: mprobe+mrecv with MPI_PROC_NULL */
    {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == MPI_PROC_NULL);
        check(s1.MPI_TAG == MPI_ANY_TAG);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg == MPI_MESSAGE_NO_PROC);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 0);

        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        /* recvbuf should remain unmodified */
        check(recvbuf[0] == 0x01234567);
        check(recvbuf[1] == 0x89abcdef);
        /* should get back "proc null status" */
        check(s2.MPI_SOURCE == MPI_PROC_NULL);
        check(s2.MPI_TAG == MPI_ANY_TAG);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
        count = -1;
        MPI_Get_count(&s2, MPI_INT, &count);
        check(count == 0);
    }

    /* test 5: mprobe+imrecv with MPI_PROC_NULL */
    {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == MPI_PROC_NULL);
        check(s1.MPI_TAG == MPI_ANY_TAG);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg == MPI_MESSAGE_NO_PROC);
        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 0);

        rreq = MPI_REQUEST_NULL;
        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Imrecv(recvbuf, count, MPI_INT, &msg, &rreq);
        check(rreq != MPI_REQUEST_NULL);
        completed = 0;
        MPI_Test(&rreq, &completed, &s2);       /* single test should always succeed */
        check(completed);
        /* recvbuf should remain unmodified */
        check(recvbuf[0] == 0x01234567);
        check(recvbuf[1] == 0x89abcdef);
        /* should get back "proc null status" */
        check(s2.MPI_SOURCE == MPI_PROC_NULL);
        check(s2.MPI_TAG == MPI_ANY_TAG);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
        count = -1;
        MPI_Get_count(&s2, MPI_INT, &count);
        check(count == 0);
    }

    /* test 6: improbe+mrecv with MPI_PROC_NULL */
    {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        found = 0;
        MPI_Improbe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &found, &msg, &s1);
        check(found);
        check(msg == MPI_MESSAGE_NO_PROC);
        check(s1.MPI_SOURCE == MPI_PROC_NULL);
        check(s1.MPI_TAG == MPI_ANY_TAG);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 0);

        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        /* recvbuf should remain unmodified */
        check(recvbuf[0] == 0x01234567);
        check(recvbuf[1] == 0x89abcdef);
        /* should get back "proc null status" */
        check(s2.MPI_SOURCE == MPI_PROC_NULL);
        check(s2.MPI_TAG == MPI_ANY_TAG);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
        count = -1;
        MPI_Get_count(&s2, MPI_INT, &count);
        check(count == 0);
    }

    /* test 7: improbe+imrecv */
    {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Improbe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &found, &msg, &s1);
        check(found);
        check(msg == MPI_MESSAGE_NO_PROC);
        check(s1.MPI_SOURCE == MPI_PROC_NULL);
        check(s1.MPI_TAG == MPI_ANY_TAG);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 0);

        rreq = MPI_REQUEST_NULL;
        MPI_Imrecv(recvbuf, count, MPI_INT, &msg, &rreq);
        check(rreq != MPI_REQUEST_NULL);
        completed = 0;
        MPI_Test(&rreq, &completed, &s2);       /* single test should always succeed */
        check(completed);
        /* recvbuf should remain unmodified */
        check(recvbuf[0] == 0x01234567);
        check(recvbuf[1] == 0x89abcdef);
        /* should get back "proc null status" */
        check(s2.MPI_SOURCE == MPI_PROC_NULL);
        check(s2.MPI_TAG == MPI_ANY_TAG);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
        count = -1;
        MPI_Get_count(&s2, MPI_INT, &count);
        check(count == 0);
    }

    /* test 8: simple ssend & mprobe+mrecv */
    if (rank == 0) {
        sendbuf[0] = 0xdeadbeef;
        sendbuf[1] = 0xfeedface;
        MPI_Ssend(sendbuf, 2, MPI_INT, 1, 5, MPI_COMM_WORLD);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg != MPI_MESSAGE_NULL);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 2);

        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
    }

    /* test 9: mprobe+mrecv LARGE */
    if (rank == 0) {
        for (i = 0; i < LARGE_SZ; i++)
            sendbuf[i] = i;
        MPI_Send(sendbuf, LARGE_SZ, MPI_INT, 1, 5, MPI_COMM_WORLD);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg != MPI_MESSAGE_NULL);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == LARGE_SZ);

        memset(recvbuf, 0xFF, LARGE_SZ * sizeof(int));
        MPI_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        for (i = 0; i < LARGE_SZ; i++)
            check(recvbuf[i] == i);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
    }

    /* test 10: mprobe+mrecv noncontiguous datatype */
    MPI_Type_vector(2, 1, 4, MPI_INT, &vectype);
    MPI_Type_commit(&vectype);
    if (rank == 0) {
        memset(sendbuf, 0, 8 * sizeof(int));
        sendbuf[0] = 0xdeadbeef;
        sendbuf[4] = 0xfeedface;
        MPI_Send(sendbuf, 1, vectype, 1, 5, MPI_COMM_WORLD);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg != MPI_MESSAGE_NULL);

        count = -1;
        MPI_Get_count(&s1, vectype, &count);
        check(count == 1);

        memset(recvbuf, 0, 8 * sizeof(int));
        MPI_Mrecv(recvbuf, 1, vectype, &msg, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        for (i = 1; i < 4; i++)
            check(recvbuf[i] == 0);
        check(recvbuf[4] = 0xfeedface);
        for (i = 5; i < 8; i++)
            check(recvbuf[i] == 0);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
    }
    MPI_Type_free(&vectype);

    /* test 11: mprobe+mrecv noncontiguous datatype LARGE */
    MPI_Type_vector(LARGE_DIM, LARGE_DIM - 1, LARGE_DIM, MPI_INT, &vectype);
    MPI_Type_commit(&vectype);
    if (rank == 0) {
        for (i = 0; i < LARGE_SZ; i++)
            sendbuf[i] = i;
        MPI_Send(sendbuf, 1, vectype, 1, 5, MPI_COMM_WORLD);
    }
    else {
        int idx = 0;

        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(0, 5, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 5);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg != MPI_MESSAGE_NULL);

        count = -1;
        MPI_Get_count(&s1, vectype, &count);
        check(count == 1);

        memset(recvbuf, 0, LARGE_SZ * sizeof(int));
        MPI_Mrecv(recvbuf, 1, vectype, &msg, &s2);
        for (i = 0; i < LARGE_DIM; i++) {
            int j;
            for (j = 0; j < LARGE_DIM - 1; j++) {
                check(recvbuf[idx] == idx);
                ++idx;
            }
            check(recvbuf[idx++] == 0);
        }
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 5);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
    }
    MPI_Type_free(&vectype);

    /* test 12: order test */
    if (rank == 0) {
        MPI_Request lrequest[2];
        sendbuf[0] = 0xdeadbeef;
        sendbuf[1] = 0xfeedface;
        sendbuf[2] = 0xdeadbeef;
        sendbuf[3] = 0xfeedface;
        sendbuf[4] = 0xdeadbeef;
        sendbuf[5] = 0xfeedface;
        MPI_Isend(&sendbuf[0], 4, MPI_INT, 1, 6, MPI_COMM_WORLD, &lrequest[0]);
        MPI_Isend(&sendbuf[4], 2, MPI_INT, 1, 6, MPI_COMM_WORLD, &lrequest[1]);
        MPI_Waitall(2, &lrequest[0], MPI_STATUSES_IGNORE);
    }
    else {
        memset(&s1, 0xab, sizeof(MPI_Status));
        memset(&s2, 0xab, sizeof(MPI_Status));
        /* the error field should remain unmodified */
        s1.MPI_ERROR = MPI_ERR_DIMS;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        msg = MPI_MESSAGE_NULL;
        MPI_Mprobe(0, 6, MPI_COMM_WORLD, &msg, &s1);
        check(s1.MPI_SOURCE == 0);
        check(s1.MPI_TAG == 6);
        check(s1.MPI_ERROR == MPI_ERR_DIMS);
        check(msg != MPI_MESSAGE_NULL);

        count = -1;
        MPI_Get_count(&s1, MPI_INT, &count);
        check(count == 4);

        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        MPI_Recv(recvbuf, 2, MPI_INT, 0, 6, MPI_COMM_WORLD, &s2);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 6);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);

        recvbuf[0] = 0x01234567;
        recvbuf[1] = 0x89abcdef;
        recvbuf[2] = 0x01234567;
        recvbuf[3] = 0x89abcdef;
        s2.MPI_ERROR = MPI_ERR_TOPOLOGY;

        MPI_Mrecv(recvbuf, count, MPI_INT, &msg, &s2);
        check(recvbuf[0] == 0xdeadbeef);
        check(recvbuf[1] == 0xfeedface);
        check(recvbuf[2] == 0xdeadbeef);
        check(recvbuf[3] == 0xfeedface);
        check(s2.MPI_SOURCE == 0);
        check(s2.MPI_TAG == 6);
        check(s2.MPI_ERROR == MPI_ERR_TOPOLOGY);
        check(msg == MPI_MESSAGE_NULL);
    }

    free(sendbuf);
    free(recvbuf);

    /* TODO MPI_ANY_SOURCE and MPI_ANY_TAG should be tested as well */
    /* TODO a full range of message sizes should be tested too */
    /* TODO threaded tests are also needed, but they should go in a separate
     * program */

    /* simple test to ensure that c2f/f2c routines are present (initially missed
     * in MPICH impl) */
    {
        MPI_Fint f_handle = 0xdeadbeef;
        f_handle = MPI_Message_c2f(MPI_MESSAGE_NULL);
        msg = MPI_Message_f2c(f_handle);
        check(f_handle != 0xdeadbeef);
        check(msg == MPI_MESSAGE_NULL);

        /* PMPI_ versions should also exists */
        f_handle = 0xdeadbeef;
        f_handle = PMPI_Message_c2f(MPI_MESSAGE_NULL);
        msg = PMPI_Message_f2c(f_handle);
        check(f_handle != 0xdeadbeef);
        check(msg == MPI_MESSAGE_NULL);
    }

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
