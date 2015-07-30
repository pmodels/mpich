/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test attempts to execute multiple simultaneous nonblocking collective
 * (NBC) MPI routines at the same time, and manages their completion with a
 * variety of routines (MPI_{Wait,Test}{,_all,_any,_some}).  It also throws a
 * few point-to-point operations into the mix.
 *
 * Possible improvements:
 * - post operations on multiple comms from multiple threads
 */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static int errs = 0;

/* Constants that control the high level test harness behavior. */
/* MAIN_ITERATIONS is how many NBC ops the test will attempt to issue. */
#define MAIN_ITERATIONS (100000)
/* WINDOW is the maximum number of outstanding NBC requests at any given time */
#define WINDOW (20)
/* we sleep with probability 1/CHANCE_OF_SLEEP */
#define CHANCE_OF_SLEEP (1000)
/* JITTER_DELAY is denominated in microseconds (us) */
#define JITTER_DELAY (50000)    /* 0.05 seconds */
/* NUM_COMMS is the number of communicators on which ops will be posted */
#define NUM_COMMS (4)

/* Constants that control behavior of the individual testing operations.
 * Altering these can help to explore the testing space, but increasing them too
 * much can consume too much memory (often O(n^2) usage). */
/* FIXME is COUNT==10 too limiting? should we try a larger count too (~500)? */
#define COUNT (10)
#define PRIME (17)

#define my_assert(cond_)                                                                 \
    do {                                                                                 \
        if (!(cond_)) {                                                                  \
            ++errs;                                                                      \
            if (errs < 10) {                                                             \
                fprintf(stderr, "assertion (%s) failed on line %d\n", #cond_, __LINE__); \
            }                                                                            \
        }                                                                                \
    } while (0)

/* Intended to act like "rand_r", but we can be sure that it will exist and be
 * consistent across all of comm world.  Returns a number in the range
 * [0,GEN_PRN_MAX] */
#define GEN_PRN_MAX (4294967291-1)
static unsigned int gen_prn(unsigned int x)
{
    /* a simple "multiplicative congruential method" PRNG, with parameters:
     *   m=4294967291, largest 32-bit prime
     *   a=279470273, good primitive root of m from "TABLES OF LINEAR
     *                CONGRUENTIAL GENERATORS OF DIFFERENT SIZES AND GOOD
     *                LATTICE STRUCTURE", by Pierre L’Ecuyer */
    return (279470273UL * (unsigned long) x) % 4294967291UL;
}

/* given a random unsigned int value "rndval_" from gen_prn, this evaluates to a
 * value in the range [min_,max_) */
#define rand_range(rndval_,min_,max_) \
    ((unsigned int)((min_) + ((rndval_) * (1.0 / (GEN_PRN_MAX+1.0)) * ((max_) - (min_)))))


static void sum_fn(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype)
{
    int i;
    int *in = invec;
    int *inout = inoutvec;
    for (i = 0; i < *len; ++i) {
        inout[i] = in[i] + inout[i];
    }
}

/* used to keep track of buffers that should be freed after the corresponding
 * operation has completed */
struct laundry {
    int case_num;               /* which test case initiated this req/laundry */
    MPI_Comm comm;
    int *buf;
    int *recvbuf;
    int *sendcounts;
    int *recvcounts;
    int *sdispls;
    int *rdispls;
    int *sendtypes;
    int *recvtypes;
};

static void cleanup_laundry(struct laundry *l)
{
    l->case_num = -1;
    l->comm = MPI_COMM_NULL;
    if (l->buf)
        free(l->buf);
    if (l->recvbuf)
        free(l->recvbuf);
    if (l->sendcounts)
        free(l->sendcounts);
    if (l->recvcounts)
        free(l->recvcounts);
    if (l->sdispls)
        free(l->sdispls);
    if (l->rdispls)
        free(l->rdispls);
    if (l->sendtypes)
        free(l->sendtypes);
    if (l->recvtypes)
        free(l->recvtypes);
}

/* Starts a "random" operation on "comm" corresponding to "rndnum" and returns
 * in (*req) a request handle corresonding to that operation.  This call should
 * be considered collective over comm (with a consistent value for "rndnum"),
 * even though the operation may only be a point-to-point request. */
static void start_random_nonblocking(MPI_Comm comm, unsigned int rndnum, MPI_Request * req,
                                     struct laundry *l)
{
    int i, j;
    int rank, size;
    int *buf = NULL;
    int *recvbuf = NULL;
    int *sendcounts = NULL;
    int *recvcounts = NULL;
    int *sdispls = NULL;
    int *rdispls = NULL;
    int *sendtypes = NULL;
    int *recvtypes = NULL;
    signed char *buf_alias = NULL;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    *req = MPI_REQUEST_NULL;

    l->case_num = -1;
    l->comm = comm;

    l->buf = buf = malloc(COUNT * size * sizeof(int));
    l->recvbuf = recvbuf = malloc(COUNT * size * sizeof(int));
    l->sendcounts = sendcounts = malloc(size * sizeof(int));
    l->recvcounts = recvcounts = malloc(size * sizeof(int));
    l->sdispls = sdispls = malloc(size * sizeof(int));
    l->rdispls = rdispls = malloc(size * sizeof(int));
    l->sendtypes = sendtypes = malloc(size * sizeof(MPI_Datatype));
    l->recvtypes = recvtypes = malloc(size * sizeof(MPI_Datatype));

#define NUM_CASES (21)
    l->case_num = rand_range(rndnum, 0, NUM_CASES);
    switch (l->case_num) {
    case 0:    /* MPI_Ibcast */
        for (i = 0; i < COUNT; ++i) {
            if (rank == 0) {
                buf[i] = i;
            }
            else {
                buf[i] = 0xdeadbeef;
            }
        }
        MPI_Ibcast(buf, COUNT, MPI_INT, 0, comm, req);
        break;

    case 1:    /* MPI_Ibcast (again, but designed to stress scatter/allgather impls) */
        /* FIXME fiddle with PRIME and buffer allocation s.t. PRIME is much larger (1021?) */
        buf_alias = (signed char *) buf;
        my_assert(COUNT * size * sizeof(int) > PRIME);  /* sanity */
        for (i = 0; i < PRIME; ++i) {
            if (rank == 0)
                buf_alias[i] = i;
            else
                buf_alias[i] = 0xdb;
        }
        for (i = PRIME; i < COUNT * size * sizeof(int); ++i) {
            buf_alias[i] = 0xbf;
        }
        MPI_Ibcast(buf_alias, PRIME, MPI_SIGNED_CHAR, 0, comm, req);
        break;

    case 2:    /* MPI_Ibarrier */
        MPI_Ibarrier(comm, req);
        break;

    case 3:    /* MPI_Ireduce */
        for (i = 0; i < COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Ireduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, 0, comm, req);
        break;

    case 4:    /* same again, use a user op and free it before the wait */
        {
            MPI_Op op = MPI_OP_NULL;
            MPI_Op_create(sum_fn, /*commute= */ 1, &op);
            for (i = 0; i < COUNT; ++i) {
                buf[i] = rank + i;
                recvbuf[i] = 0xdeadbeef;
            }
            MPI_Ireduce(buf, recvbuf, COUNT, MPI_INT, op, 0, comm, req);
            MPI_Op_free(&op);
        }
        break;

    case 5:    /* MPI_Iallreduce */
        for (i = 0; i < COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Iallreduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, comm, req);
        break;

    case 6:    /* MPI_Ialltoallv (a weak test, neither irregular nor sparse) */
        for (i = 0; i < size; ++i) {
            sendcounts[i] = COUNT;
            recvcounts[i] = COUNT;
            sdispls[i] = COUNT * i;
            rdispls[i] = COUNT * i;
            for (j = 0; j < COUNT; ++j) {
                buf[i * COUNT + j] = rank + (i * j);
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
        }
        MPI_Ialltoallv(buf, sendcounts, sdispls, MPI_INT, recvbuf, recvcounts, rdispls, MPI_INT,
                       comm, req);
        break;

    case 7:    /* MPI_Igather */
        for (i = 0; i < size * COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Igather(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, 0, comm, req);
        break;

    case 8:    /* same test again, just use a dup'ed datatype and free it before the wait */
        {
            MPI_Datatype type = MPI_DATATYPE_NULL;
            MPI_Type_dup(MPI_INT, &type);
            for (i = 0; i < size * COUNT; ++i) {
                buf[i] = rank + i;
                recvbuf[i] = 0xdeadbeef;
            }
            MPI_Igather(buf, COUNT, MPI_INT, recvbuf, COUNT, type, 0, comm, req);
            MPI_Type_free(&type);       /* should cause implementations that don't refcount
                                         * correctly to blow up or hang in the wait */
        }
        break;

    case 9:    /* MPI_Iscatter */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                if (rank == 0)
                    buf[i * COUNT + j] = i + j;
                else
                    buf[i * COUNT + j] = 0xdeadbeef;
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
        }
        MPI_Iscatter(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, 0, comm, req);
        break;

    case 10:   /* MPI_Iscatterv */
        for (i = 0; i < size; ++i) {
            /* weak test, just test the regular case where all counts are equal */
            sendcounts[i] = COUNT;
            sdispls[i] = i * COUNT;
            for (j = 0; j < COUNT; ++j) {
                if (rank == 0)
                    buf[i * COUNT + j] = i + j;
                else
                    buf[i * COUNT + j] = 0xdeadbeef;
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
        }
        MPI_Iscatterv(buf, sendcounts, sdispls, MPI_INT, recvbuf, COUNT, MPI_INT, 0, comm, req);
        break;

    case 11:   /* MPI_Ireduce_scatter */
        for (i = 0; i < size; ++i) {
            recvcounts[i] = COUNT;
            for (j = 0; j < COUNT; ++j) {
                buf[i * COUNT + j] = rank + i;
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
        }
        MPI_Ireduce_scatter(buf, recvbuf, recvcounts, MPI_INT, MPI_SUM, comm, req);
        break;

    case 12:   /* MPI_Ireduce_scatter_block */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                buf[i * COUNT + j] = rank + i;
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
        }
        MPI_Ireduce_scatter_block(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, comm, req);
        break;

    case 13:   /* MPI_Igatherv */
        for (i = 0; i < size * COUNT; ++i) {
            buf[i] = 0xdeadbeef;
            recvbuf[i] = 0xdeadbeef;
        }
        for (i = 0; i < COUNT; ++i) {
            buf[i] = rank + i;
        }
        for (i = 0; i < size; ++i) {
            recvcounts[i] = COUNT;
            rdispls[i] = i * COUNT;
        }
        MPI_Igatherv(buf, COUNT, MPI_INT, recvbuf, recvcounts, rdispls, MPI_INT, 0, comm, req);
        break;

    case 14:   /* MPI_Ialltoall */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                buf[i * COUNT + j] = rank + (i * j);
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
        }
        MPI_Ialltoall(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, comm, req);
        break;

    case 15:   /* MPI_Iallgather */
        for (i = 0; i < size * COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Iallgather(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, comm, req);
        break;

    case 16:   /* MPI_Iallgatherv */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
            recvcounts[i] = COUNT;
            rdispls[i] = i * COUNT;
        }
        for (i = 0; i < COUNT; ++i)
            buf[i] = rank + i;
        MPI_Iallgatherv(buf, COUNT, MPI_INT, recvbuf, recvcounts, rdispls, MPI_INT, comm, req);
        break;

    case 17:   /* MPI_Iscan */
        for (i = 0; i < COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Iscan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, comm, req);
        break;

    case 18:   /* MPI_Iexscan */
        for (i = 0; i < COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Iexscan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, comm, req);
        break;

    case 19:   /* MPI_Ialltoallw (a weak test, neither irregular nor sparse) */
        for (i = 0; i < size; ++i) {
            sendcounts[i] = COUNT;
            recvcounts[i] = COUNT;
            sdispls[i] = COUNT * i * sizeof(int);
            rdispls[i] = COUNT * i * sizeof(int);
            sendtypes[i] = MPI_INT;
            recvtypes[i] = MPI_INT;
            for (j = 0; j < COUNT; ++j) {
                buf[i * COUNT + j] = rank + (i * j);
                recvbuf[i * COUNT + j] = 0xdeadbeef;
            }
        }
        MPI_Ialltoallw(buf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                       comm, req);
        break;

    case 20:   /* basic pt2pt MPI_Isend/MPI_Irecv pairing */
        /* even ranks send to odd ranks, but only if we have a full pair */
        if ((rank % 2 != 0) || (rank != size - 1)) {
            for (j = 0; j < COUNT; ++j) {
                buf[j] = j;
                recvbuf[j] = 0xdeadbeef;
            }
            if (rank % 2 == 0)
                MPI_Isend(buf, COUNT, MPI_INT, rank + 1, 5, comm, req);
            else
                MPI_Irecv(recvbuf, COUNT, MPI_INT, rank - 1, 5, comm, req);
        }
        break;

    default:
        fprintf(stderr, "unexpected value for l->case_num=%d)\n", (l->case_num));
        MPI_Abort(comm, 1);
        break;
    }
}

static void check_after_completion(struct laundry *l)
{
    int i, j;
    int rank, size;
    MPI_Comm comm = l->comm;
    int *buf = l->buf;
    int *recvbuf = l->recvbuf;
    int *sendcounts = l->sendcounts;
    int *recvcounts = l->recvcounts;
    int *sdispls = l->sdispls;
    int *rdispls = l->rdispls;
    int *sendtypes = l->sendtypes;
    int *recvtypes = l->recvtypes;
    char *buf_alias = (char *) buf;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* these cases all correspond to cases in start_random_nonblocking */
    switch (l->case_num) {
    case 0:    /* MPI_Ibcast */
        for (i = 0; i < COUNT; ++i) {
            if (buf[i] != i)
                printf("buf[%d]=%d i=%d\n", i, buf[i], i);
            my_assert(buf[i] == i);
        }
        break;

    case 1:    /* MPI_Ibcast (again, but designed to stress scatter/allgather impls) */
        for (i = 0; i < PRIME; ++i) {
            if (buf_alias[i] != i)
                printf("buf_alias[%d]=%d i=%d\n", i, buf_alias[i], i);
            my_assert(buf_alias[i] == i);
        }
        break;

    case 2:    /* MPI_Ibarrier */
        /* nothing to check */
        break;

    case 3:    /* MPI_Ireduce */
        if (rank == 0) {
            for (i = 0; i < COUNT; ++i) {
                if (recvbuf[i] != ((size * (size - 1) / 2) + (i * size)))
                    printf("got recvbuf[%d]=%d, expected %d\n", i, recvbuf[i],
                           ((size * (size - 1) / 2) + (i * size)));
                my_assert(recvbuf[i] == ((size * (size - 1) / 2) + (i * size)));
            }
        }
        break;

    case 4:    /* same again, use a user op and free it before the wait */
        if (rank == 0) {
            for (i = 0; i < COUNT; ++i) {
                if (recvbuf[i] != ((size * (size - 1) / 2) + (i * size)))
                    printf("got recvbuf[%d]=%d, expected %d\n", i, recvbuf[i],
                           ((size * (size - 1) / 2) + (i * size)));
                my_assert(recvbuf[i] == ((size * (size - 1) / 2) + (i * size)));
            }
        }
        break;

    case 5:    /* MPI_Iallreduce */
        for (i = 0; i < COUNT; ++i) {
            if (recvbuf[i] != ((size * (size - 1) / 2) + (i * size)))
                printf("got recvbuf[%d]=%d, expected %d\n", i, recvbuf[i],
                       ((size * (size - 1) / 2) + (i * size)));
            my_assert(recvbuf[i] == ((size * (size - 1) / 2) + (i * size)));
        }
        break;

    case 6:    /* MPI_Ialltoallv (a weak test, neither irregular nor sparse) */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                /*printf("recvbuf[%d*COUNT+%d]=%d, expecting %d\n", i, j, recvbuf[i*COUNT+j], (i + (rank * j))); */
                my_assert(recvbuf[i * COUNT + j] == (i + (rank * j)));
            }
        }
        break;

    case 7:    /* MPI_Igather */
        if (rank == 0) {
            for (i = 0; i < size; ++i) {
                for (j = 0; j < COUNT; ++j) {
                    my_assert(recvbuf[i * COUNT + j] == i + j);
                }
            }
        }
        else {
            for (i = 0; i < size * COUNT; ++i) {
                my_assert(recvbuf[i] == 0xdeadbeef);
            }
        }
        break;

    case 8:    /* same test again, just use a dup'ed datatype and free it before the wait */
        if (rank == 0) {
            for (i = 0; i < size; ++i) {
                for (j = 0; j < COUNT; ++j) {
                    my_assert(recvbuf[i * COUNT + j] == i + j);
                }
            }
        }
        else {
            for (i = 0; i < size * COUNT; ++i) {
                my_assert(recvbuf[i] == 0xdeadbeef);
            }
        }
        break;

    case 9:    /* MPI_Iscatter */
        for (j = 0; j < COUNT; ++j) {
            my_assert(recvbuf[j] == rank + j);
        }
        if (rank != 0) {
            for (i = 0; i < size * COUNT; ++i) {
                /* check we didn't corrupt the sendbuf somehow */
                my_assert(buf[i] == 0xdeadbeef);
            }
        }
        break;

    case 10:   /* MPI_Iscatterv */
        for (j = 0; j < COUNT; ++j) {
            my_assert(recvbuf[j] == rank + j);
        }
        if (rank != 0) {
            for (i = 0; i < size * COUNT; ++i) {
                /* check we didn't corrupt the sendbuf somehow */
                my_assert(buf[i] == 0xdeadbeef);
            }
        }
        for (i = 1; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                /* check we didn't corrupt the rest of the recvbuf */
                my_assert(recvbuf[i * COUNT + j] == 0xdeadbeef);
            }
        }
        break;

    case 11:   /* MPI_Ireduce_scatter */
        for (j = 0; j < COUNT; ++j) {
            my_assert(recvbuf[j] == (size * rank + ((size - 1) * size) / 2));
        }
        for (i = 1; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                /* check we didn't corrupt the rest of the recvbuf */
                my_assert(recvbuf[i * COUNT + j] == 0xdeadbeef);
            }
        }
        break;

    case 12:   /* MPI_Ireduce_scatter_block */
        for (j = 0; j < COUNT; ++j) {
            my_assert(recvbuf[j] == (size * rank + ((size - 1) * size) / 2));
        }
        for (i = 1; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                /* check we didn't corrupt the rest of the recvbuf */
                my_assert(recvbuf[i * COUNT + j] == 0xdeadbeef);
            }
        }
        break;

    case 13:   /* MPI_Igatherv */
        if (rank == 0) {
            for (i = 0; i < size; ++i) {
                for (j = 0; j < COUNT; ++j) {
                    my_assert(recvbuf[i * COUNT + j] == i + j);
                }
            }
        }
        else {
            for (i = 0; i < size * COUNT; ++i) {
                my_assert(recvbuf[i] == 0xdeadbeef);
            }
        }
        break;

    case 14:   /* MPI_Ialltoall */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                /*printf("recvbuf[%d*COUNT+%d]=%d, expecting %d\n", i, j, recvbuf[i*COUNT+j], (i + (i * j))); */
                my_assert(recvbuf[i * COUNT + j] == (i + (rank * j)));
            }
        }
        break;

    case 15:   /* MPI_Iallgather */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                my_assert(recvbuf[i * COUNT + j] == i + j);
            }
        }
        break;

    case 16:   /* MPI_Iallgatherv */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                my_assert(recvbuf[i * COUNT + j] == i + j);
            }
        }
        break;

    case 17:   /* MPI_Iscan */
        for (i = 0; i < COUNT; ++i) {
            my_assert(recvbuf[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1))));
        }
        break;

    case 18:   /* MPI_Iexscan */
        for (i = 0; i < COUNT; ++i) {
            if (rank == 0)
                my_assert(recvbuf[i] == 0xdeadbeef);
            else
                my_assert(recvbuf[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1)) - (rank + i)));
        }
        break;

    case 19:   /* MPI_Ialltoallw (a weak test, neither irregular nor sparse) */
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                /*printf("recvbuf[%d*COUNT+%d]=%d, expecting %d\n", i, j, recvbuf[i*COUNT+j], (i + (rank * j))); */
                my_assert(recvbuf[i * COUNT + j] == (i + (rank * j)));
            }
        }
        break;

    case 20:   /* basic pt2pt MPI_Isend/MPI_Irecv pairing */
        /* even ranks send to odd ranks, but only if we have a full pair */
        if ((rank % 2 != 0) || (rank != size - 1)) {
            for (j = 0; j < COUNT; ++j) {
                /* only odd procs did a recv */
                if (rank % 2 == 0) {
                    my_assert(recvbuf[j] == 0xdeadbeef);
                }
                else {
                    if (recvbuf[j] != j)
                        printf("recvbuf[%d]=%d j=%d\n", j, recvbuf[j], j);
                    my_assert(recvbuf[j] == j);
                }
            }
        }
        break;

    default:
        printf("invalid case_num (%d) detected\n", l->case_num);
        assert(0);
        break;
    }
}

#undef NUM_CASES

static void complete_something_somehow(unsigned int rndnum, int numreqs, MPI_Request reqs[],
                                       int *outcount, int indices[])
{
    int i, idx, flag;

#define COMPLETION_CASES (8)
    switch (rand_range(rndnum, 0, COMPLETION_CASES)) {
    case 0:
        MPI_Waitall(numreqs, reqs, MPI_STATUSES_IGNORE);
        *outcount = numreqs;
        for (i = 0; i < numreqs; ++i) {
            indices[i] = i;
        }
        break;

    case 1:
        MPI_Testsome(numreqs, reqs, outcount, indices, MPI_STATUS_IGNORE);
        if (*outcount == MPI_UNDEFINED) {
            *outcount = 0;
        }
        break;

    case 2:
        MPI_Waitsome(numreqs, reqs, outcount, indices, MPI_STATUS_IGNORE);
        if (*outcount == MPI_UNDEFINED) {
            *outcount = 0;
        }
        break;

    case 3:
        MPI_Waitany(numreqs, reqs, &idx, MPI_STATUS_IGNORE);
        if (idx == MPI_UNDEFINED) {
            *outcount = 0;
        }
        else {
            *outcount = 1;
            indices[0] = idx;
        }
        break;

    case 4:
        MPI_Testany(numreqs, reqs, &idx, &flag, MPI_STATUS_IGNORE);
        if (idx == MPI_UNDEFINED) {
            *outcount = 0;
        }
        else {
            *outcount = 1;
            indices[0] = idx;
        }
        break;

    case 5:
        MPI_Testall(numreqs, reqs, &flag, MPI_STATUSES_IGNORE);
        if (flag) {
            *outcount = numreqs;
            for (i = 0; i < numreqs; ++i) {
                indices[i] = i;
            }
        }
        else {
            *outcount = 0;
        }
        break;

    case 6:
        /* select a new random index and wait on it */
        rndnum = gen_prn(rndnum);
        idx = rand_range(rndnum, 0, numreqs);
        MPI_Wait(&reqs[idx], MPI_STATUS_IGNORE);
        *outcount = 1;
        indices[0] = idx;
        break;

    case 7:
        /* select a new random index and wait on it */
        rndnum = gen_prn(rndnum);
        idx = rand_range(rndnum, 0, numreqs);
        MPI_Test(&reqs[idx], &flag, MPI_STATUS_IGNORE);
        *outcount = (flag ? 1 : 0);
        indices[0] = idx;
        break;

    default:
        assert(0);
        break;
    }
#undef COMPLETION_CASES
}

int main(int argc, char **argv)
{
    int i, num_posted, num_completed;
    int wrank, wsize;
    unsigned int seed = 0x10bc;
    unsigned int post_seq, complete_seq;
    struct laundry larr[WINDOW];
    MPI_Request reqs[WINDOW];
    int outcount;
    int indices[WINDOW];
    MPI_Comm comms[NUM_COMMS];
    MPI_Comm comm;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    /* it is critical that all processes in the communicator start with a
     * consistent value for "post_seq" */
    post_seq = complete_seq = gen_prn(seed);

    num_completed = 0;
    num_posted = 0;

    /* construct all of the communicators, just dups of comm world for now */
    for (i = 0; i < NUM_COMMS; ++i) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

    /* fill the entire window of ops */
    for (i = 0; i < WINDOW; ++i) {
        reqs[i] = MPI_REQUEST_NULL;
        memset(&larr[i], 0, sizeof(struct laundry));
        larr[i].case_num = -1;

        /* randomly select a comm, using a new seed to avoid correlating
         * particular kinds of NBC ops with particular communicators */
        comm = comms[rand_range(gen_prn(post_seq), 0, NUM_COMMS)];

        start_random_nonblocking(comm, post_seq, &reqs[i], &larr[i]);
        ++num_posted;
        post_seq = gen_prn(post_seq);
    }

    /* now loop repeatedly, completing ops with "random" completion functions,
     * until we've posted and completed MAIN_ITERATIONS ops */
    while (num_completed < MAIN_ITERATIONS) {
        complete_something_somehow(complete_seq, WINDOW, reqs, &outcount, indices);
        complete_seq = gen_prn(complete_seq);
        for (i = 0; i < outcount; ++i) {
            int idx = indices[i];
            assert(reqs[idx] == MPI_REQUEST_NULL);
            if (larr[idx].case_num != -1) {
                check_after_completion(&larr[idx]);
                cleanup_laundry(&larr[idx]);
                ++num_completed;
                if (num_posted < MAIN_ITERATIONS) {
                    comm = comms[rand_range(gen_prn(post_seq), 0, NUM_COMMS)];
                    start_random_nonblocking(comm, post_seq, &reqs[idx], &larr[idx]);
                    ++num_posted;
                    post_seq = gen_prn(post_seq);
                }
            }
        }

        /* "randomly" and infrequently introduce some jitter into the system */
        if (0 == rand_range(gen_prn(complete_seq + wrank), 0, CHANCE_OF_SLEEP)) {
            usleep(JITTER_DELAY);       /* take a short nap */
        }
    }

    for (i = 0; i < NUM_COMMS; ++i) {
        MPI_Comm_free(&comms[i]);
    }

    if (wrank == 0) {
        if (errs)
            printf("found %d errors\n", errs);
        else
            printf(" No errors\n");
    }

    MPI_Finalize();

    return 0;
}
