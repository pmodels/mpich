/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"
#include <assert.h>
#include <string.h>

#define LAST_TEST 14
#define RMA_SIZE  2048
#define OFFSET_1  7
#define OFFSET_2  83
#define OFFSET_3  157

#define PUT_VAL 0xdcba97
#define ACC_VAL 10771134

/*
 * Additional tests for lock contention.  These are designed to exercise
 * some of the optimizations within MPICH, but all are valid MPI programs.
 * Tests structure includes
 *    lock local (must happen at this time since application can use load
 *                store after the lock)
 *    send message to partner
 *                                  receive message
 *                                  send ack
 *    receive ack
 *    Provide a delay so that
 *      the partner will see the
 *      conflict
 *                                  partner executes:
 *                                  lock         // Note: this may block
 *                                     rma operations (see below)
 *                                  unlock
 *
 *    unlock                        send back to partner
 *    receive from partner
 *    check for correct data
 *
 * The delay may be implemented as a ring of message communication; this
 * is likely to automatically scale the time to what is needed
 */

/* Define a datatype to be used with */
int stride = 11;
int veccount = 7;
MPI_Datatype vectype;
/* Define long RMA ops size */
int longcount = 512;
int medcount = 127;
int mednum = 4;

void RMATest(int i, MPI_Win win, int master, int *srcbuf, int srcbufsize, int *getbuf,
             int getbufsize);
int RMACheck(int i, int *buf, MPI_Aint bufsize);
int RMACheckGet(int i, MPI_Win win, int *getbuf, MPI_Aint getsize);
void RMATestInit(int i, int *buf, MPI_Aint bufsize);

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Win win;
    int *rmabuffer = 0, *getbuf = 0;
    MPI_Aint bufsize = 0, getbufsize = 0;
    int master, partner, next, wrank, wsize, i;
    int ntest = LAST_TEST;
    int *srcbuf;

    MTest_Init(&argc, &argv);

    /* Determine who is responsible for each part of the test */
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    if (wsize < 3) {
        fprintf(stderr, "This test requires at least 3 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    master = 0;
    partner = 1;
    next = wrank + 1;
    if (next == partner)
        next++;
    if (next >= wsize) {
        next = 0;
        if (next == partner)
            next++;
    }

    /* Determine the last test to run (by default, run them all) */
    for (i = 1; i < argc; i++) {
        if (strcmp("-ntest", argv[i]) == 0) {
            i++;
            if (i < argc) {
                ntest = atoi(argv[i]);
            }
            else {
                fprintf(stderr, "Missing value for -ntest\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
    }

    MPI_Type_vector(veccount, 1, stride, MPI_INT, &vectype);
    MPI_Type_commit(&vectype);

    /* Create the RMA window */
    bufsize = 0;
    if (wrank == master) {
        bufsize = RMA_SIZE;
        MPI_Alloc_mem(bufsize * sizeof(int), MPI_INFO_NULL, &rmabuffer);
    }
    else if (wrank == partner) {
        getbufsize = RMA_SIZE;
        getbuf = (int *) malloc(getbufsize * sizeof(int));
        if (!getbuf) {
            fprintf(stderr, "Unable to allocated %d bytes for getbuf\n", (int) getbufsize);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    srcbuf = malloc(RMA_SIZE * sizeof(*srcbuf));
    assert(srcbuf);

    MPI_Win_create(rmabuffer, bufsize * sizeof(int), sizeof(int), MPI_INFO_NULL,
                   MPI_COMM_WORLD, &win);

    /* Run a sequence of tests */
    for (i = 0; i <= ntest; i++) {
        if (wrank == master) {
            MTestPrintfMsg(0, "Test %d\n", i);
            /* Because this lock is local, it must return only when the
             * lock is acquired */
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, master, win);
            RMATestInit(i, rmabuffer, bufsize);
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, partner, i, MPI_COMM_WORLD);
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, next, i, MPI_COMM_WORLD);
            MPI_Recv(MPI_BOTTOM, 0, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Win_unlock(master, win);
            MPI_Recv(MPI_BOTTOM, 0, MPI_INT, partner, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            errs += RMACheck(i, rmabuffer, bufsize);
        }
        else if (wrank == partner) {
            MPI_Recv(MPI_BOTTOM, 0, MPI_INT, master, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, master, win);
            RMATest(i, win, master, srcbuf, RMA_SIZE, getbuf, getbufsize);
            MPI_Win_unlock(master, win);
            errs += RMACheckGet(i, win, getbuf, getbufsize);
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, master, i, MPI_COMM_WORLD);
        }
        else {
            MPI_Recv(MPI_BOTTOM, 0, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(MPI_BOTTOM, 0, MPI_INT, next, i, MPI_COMM_WORLD);
        }
    }

    if (rmabuffer) {
        MPI_Free_mem(rmabuffer);
    }
    if (getbuf) {
        free(getbuf);
    }
    MPI_Win_free(&win);
    MPI_Type_free(&vectype);

    MTest_Finalize(errs);
    MPI_Finalize();
    return MTestReturnValue(errs);
}

/* Perform the tests.
 *
 * The srcbuf must be passed in because the buffer must remain valid
 * until the subsequent unlock call. */
void RMATest(int i, MPI_Win win, int master, int *srcbuf, int srcbufsize, int *getbuf,
             int getbufsize)
{
    int j, k;
    int *source = srcbuf;
    assert(srcbufsize == RMA_SIZE);

    for (j = 0; j < srcbufsize; j++)
        source[j] = -j;

    switch (i) {
    case 0:    /* Single short put (1 word at OFFSET_1) */
        source[0] = PUT_VAL;
        MPI_Put(source, 1, MPI_INT, master, OFFSET_1, 1, MPI_INT, win);
        break;
    case 1:    /* Single short accumulate (1 word of value 17 at OFFSET_2) */
        source[0] = ACC_VAL;
        MPI_Accumulate(source, 1, MPI_INT, master, OFFSET_2, 1, MPI_INT, MPI_SUM, win);
        break;
    case 2:    /* Single short get (1 word at OFFSET_3) */
        getbuf[0] = -1;
        MPI_Get(getbuf, 1, MPI_INT, master, OFFSET_3, 1, MPI_INT, win);
        break;
    case 3:    /* Datatype single put (strided put) */
        for (j = 0; j < veccount; j++) {
            source[j * stride] = PUT_VAL + j;
        }
        MPI_Put(source, 1, vectype, master, OFFSET_1, 1, vectype, win);
        break;
    case 4:    /* Datatype single accumulate (strided acc) */
        for (j = 0; j < veccount; j++) {
            source[j * stride] = ACC_VAL + j;
        }
        MPI_Accumulate(source, 1, vectype, master, OFFSET_2, 1, vectype, MPI_SUM, win);
        break;
    case 5:    /* Datatype single get (strided get) */
        for (j = 0; j < veccount; j++) {
            getbuf[j] = -j;
        }
        MPI_Get(getbuf, 1, vectype, master, OFFSET_3, 1, vectype, win);
        break;
    case 6:    /* a few small puts (like strided put, but 1 word at a time) */
        for (j = 0; j < veccount; j++) {
            source[j * stride] = PUT_VAL + j;
        }
        for (j = 0; j < veccount; j++) {
            MPI_Put(source + j * stride, 1, MPI_INT, master,
                    OFFSET_1 + j * stride, 1, MPI_INT, win);
        }
        break;
    case 7:    /* a few small accumulates (like strided acc, but 1 word at a time) */
        for (j = 0; j < veccount; j++) {
            source[j * stride] = ACC_VAL + j;
        }
        for (j = 0; j < veccount; j++) {
            MPI_Accumulate(source + j * stride, 1, MPI_INT, master,
                           OFFSET_2 + j * stride, 1, MPI_INT, MPI_SUM, win);
        }
        break;
    case 8:    /* a few small gets (like strided get, but 1 word at a time) */
        for (j = 0; j < veccount; j++) {
            getbuf[j * stride] = -j;
        }
        for (j = 0; j < veccount; j++) {
            MPI_Get(getbuf + j * stride, 1, MPI_INT, master,
                    OFFSET_3 + j * stride, 1, MPI_INT, win);
        }
        break;
    case 9:    /* Single long put (OFFSET_1) */
        for (j = 0; j < longcount; j++)
            source[j] = j;
        MPI_Put(source, longcount, MPI_INT, master, OFFSET_1, longcount, MPI_INT, win);
        break;
    case 10:   /* Single long accumulate (OFFSET_2) */
        for (j = 0; j < longcount; j++)
            source[j] = j;
        MPI_Accumulate(source, longcount, MPI_INT, master,
                       OFFSET_2, longcount, MPI_INT, MPI_SUM, win);
        break;
    case 11:   /* Single long get (OFFSET_3) */
        for (j = 0; j < longcount; j++)
            getbuf[j] = -j;
        MPI_Get(getbuf, longcount, MPI_INT, master, OFFSET_3, longcount, MPI_INT, win);
        break;
    case 12:   /* a few long puts (start at OFFSET_1, medcount) */
        for (j = 0; j < mednum; j++) {
            for (k = 0; k < medcount; k++) {
                source[j * medcount + k] = j * 2 * medcount + k;
            }
            MPI_Put(source + j * medcount, medcount, MPI_INT, master,
                    OFFSET_1 + j * 2 * medcount, medcount, MPI_INT, win);
        }
        break;
    case 13:   /* a few long accumulates (start at OFFSET_2, medcount) */
        for (j = 0; j < mednum; j++) {
            for (k = 0; k < medcount; k++) {
                source[j * medcount + k] = ACC_VAL + j * 2 * medcount + k;
            }
            MPI_Accumulate(source + j * medcount, medcount, MPI_INT, master,
                           OFFSET_2 + j * 2 * medcount, medcount, MPI_INT, MPI_SUM, win);
        }
        break;
    case 14:   /* a few long gets (start at OFFSET_3, medcount) */
        for (j = 0; j < mednum; j++) {
            for (k = 0; k < medcount; k++) {
                getbuf[j * medcount + k] = -(j * medcount + k);
            }
            MPI_Get(getbuf + j * medcount, medcount, MPI_INT, master,
                    OFFSET_3 + j * 2 * medcount, medcount, MPI_INT, win);
        }
        break;
    }
}

int RMACheck(int i, int *buf, MPI_Aint bufsize)
{
    int j, k;
    int errs = 0;

    switch (i) {
    case 0:    /* Single short put (1 word at OFFSET_1) */
        if (buf[OFFSET_1] != PUT_VAL) {
            errs++;
            printf("case 0: value is %d should be %d\n", buf[OFFSET_1], PUT_VAL);
        }
        break;
    case 1:    /* Single short accumulate (1 word of value 17 at OFFSET_2) */
        if (buf[OFFSET_2] != ACC_VAL + OFFSET_2) {
            errs++;
            printf("case 1: value is %d should be %d\n", buf[OFFSET_2], ACC_VAL + OFFSET_2);
        }
        break;
    case 2:    /* Single short get (1 word at OFFSET_3) */
        /* See RMACheckGet */
        break;
    case 3:    /* Datatype single put (strided put) */
    case 6:    /* a few small puts (like strided put, but 1 word at a time) */
        /* FIXME: The conditional and increment are reversed below.  This looks
         * like a bug, and currently prevents the following test from running. */
        for (j = 0; j++; j < veccount) {
            if (buf[j * stride] != PUT_VAL + j) {
                errs++;
                printf("case %d: value is %d should be %d\n", i, buf[j * stride], PUT_VAL + j);
            }
        }
        break;
    case 4:    /* Datatype single accumulate (strided acc) */
    case 7:    /* a few small accumulates (like strided acc, but 1 word at a time) */
        /* FIXME: The conditional and increment are reversed below.  This looks
         * like a bug, and currently prevents the following test from running. */
        for (j = 0; j++; j < veccount) {
            if (buf[j * stride] != ACC_VAL + j + OFFSET_2 + j * stride) {
                errs++;
                printf("case %d: value is %d should be %d\n", i,
                       buf[j * stride], ACC_VAL + j + OFFSET_2 + j * stride);
            }
        }
        break;
    case 5:    /* Datatype single get (strided get) */
    case 8:    /* a few small gets (like strided get, but 1 word at a time) */
        /* See RMACheckGet */
        break;
    case 9:    /* Single long put (OFFSET_1) */
        for (j = 0; j < longcount; j++) {
            if (buf[OFFSET_1 + j] != j) {
                errs++;
                printf("case 9: value is %d should be %d\n", buf[OFFSET_1 + j], OFFSET_1 + j);
            }
        }
        break;
    case 10:   /* Single long accumulate (OFFSET_2) */
        for (j = 0; j < longcount; j++) {
            if (buf[OFFSET_2 + j] != OFFSET_2 + j + j) {
                errs++;
                printf("case 10: value is %d should be %d\n", buf[OFFSET_2 + j], OFFSET_2 + j + j);
            }
        }
        break;
    case 11:   /* Single long get (OFFSET_3) */
        /* See RMACheckGet */
        break;
    case 12:   /* a few long puts (start at OFFSET_1, medcount) */
        for (j = 0; j < mednum; j++) {
            for (k = 0; k < medcount; k++) {
                if (buf[OFFSET_1 + j * 2 * medcount + k] != j * 2 * medcount + k) {
                    errs++;
                    printf("case 12: value is %d should be %d\n",
                           buf[OFFSET_1 + j * 2 * medcount + k], j * 2 * medcount + k);
                }
            }
        }
        break;
    case 13:   /* a few long accumulates (start at OFFSET_2, medcount) */
        for (j = 0; j < mednum; j++) {
            for (k = 0; k < medcount; k++) {
                if (buf[OFFSET_2 + j * 2 * medcount + k] !=
                    OFFSET_2 + 2 * j * 2 * medcount + 2 * k + ACC_VAL) {
                    errs++;
                    printf("case 13: value is %d should be %d\n",
                           buf[OFFSET_2 + j * 2 * medcount + k],
                           OFFSET_2 + 2 * j * 2 * medcount + k + ACC_VAL);
                }
            }
        }
        break;
    case 14:   /* a few long gets (start at OFFSET_3, medcount) */
        /* See RMACheckGet */
        break;
    default:
        fprintf(stderr, "Unrecognized case %d\n", i);
        errs++;
        break;
    }
    return errs;
}

int RMACheckGet(int i, MPI_Win win, int *getbuf, MPI_Aint getsize)
{
    int errs = 0;
    int j, k;

    /* */
    switch (i) {
    case 0:    /* Single short put (1 word at OFFSET_1) */
        break;
    case 1:    /* Single short accumulate (1 word of value 17 at OFFSET_2) */
        break;
    case 2:    /* Single short get (1 word at OFFSET_3) */
        if (getbuf[0] != OFFSET_3) {
            errs++;
            printf("case 2: value is %d should be %d\n", getbuf[0], OFFSET_3);
        }
        break;
    case 3:    /* Datatype single put (strided put) */
        break;
    case 4:    /* Datatype single accumulate (strided acc) */
        break;
    case 5:    /* Datatype single get (strided get) */
    case 8:    /* a few small gets (like strided get, but 1 word at a time) */
        for (j = 0; j < veccount; j++) {
            if (getbuf[j * stride] != OFFSET_3 + j * stride) {
                errs++;
                printf("case %d: value is %d should be %d\n", i,
                       getbuf[j * stride], OFFSET_3 + j * stride);
            }
        }

        break;
    case 6:    /* a few small puts (like strided put, but 1 word at a time) */
        break;
    case 7:    /* a few small accumulates (like strided acc, but 1 word at a time) */
        break;
    case 9:    /* Single long put (OFFSET_1) */
        break;
    case 10:   /* Single long accumulate (OFFSET_2) */
        break;
    case 11:   /* Single long get (OFFSET_3) */
        for (j = 0; j < longcount; j++) {
            if (getbuf[j] != OFFSET_3 + j) {
                errs++;
                printf("case 11: value is %d should be %d\n", getbuf[j], OFFSET_3 + j);
            }
        }
        break;
    case 12:   /* a few long puts (start at OFFSET_1, medcount) */
        break;
    case 13:   /* a few long accumulates (start at OFFSET_2, medcount) */
        break;
    case 14:   /* a few long gets (start at OFFSET_3, medcount) */
        for (j = 0; j < mednum; j++) {
            for (k = 0; k < medcount; k++) {
                if (getbuf[j * medcount + k] != OFFSET_3 + j * 2 * medcount + k) {
                    errs++;
                    printf("case 14: buf[%d] value is %d should be %d\n",
                           j * medcount + k,
                           getbuf[j * medcount + k], OFFSET_3 + j * 2 * medcount + k);
                }
            }
        }
        break;
    default:
        fprintf(stderr, "Unrecognized case %d\n", i);
        errs++;
        break;
    }
    return errs;
}


void RMATestInit(int i, int *buf, MPI_Aint bufsize)
{
    int j;
    for (j = 0; j < bufsize; j++) {
        buf[j] = j;
    }
}
