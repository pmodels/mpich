/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* A basic test of all 17 nonblocking collective operations specified by the
 * MPI-3 standard.  It only exercises the intracommunicator functionality,
 * does not use MPI_IN_PLACE, and only transmits/receives simple integer types
 * with relatively small counts.  It does check a few fancier issues, such as
 * ensuring that "premature user releases" of MPI_Op and MPI_Datatype objects
 * does not result in an error or segfault. */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>

#define COUNT (10)
#define PRIME (17)

#define my_assert(cond_)                                                  \
    do {                                                                  \
        if (!(cond_)) {                                                   \
            fprintf(stderr, "assertion (%s) failed, aborting\n", #cond_); \
            MPI_Abort(MPI_COMM_WORLD, 1);                                 \
        }                                                                 \
    } while (0)

static void sum_fn(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype)
{
    int i;
    int *in = invec;
    int *inout = inoutvec;
    for (i = 0; i < *len; ++i) {
        inout[i] = in[i] + inout[i];
    }
}


int main(int argc, char **argv)
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
    MPI_Request req;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    buf = malloc(COUNT * size * sizeof(int));
    recvbuf = malloc(COUNT * size * sizeof(int));
    sendcounts = malloc(size * sizeof(int));
    recvcounts = malloc(size * sizeof(int));
    sdispls = malloc(size * sizeof(int));
    rdispls = malloc(size * sizeof(int));
    sendtypes = malloc(size * sizeof(MPI_Datatype));
    recvtypes = malloc(size * sizeof(MPI_Datatype));

    /* MPI_Ibcast */
    for (i = 0; i < COUNT; ++i) {
        if (rank == 0) {
            buf[i] = i;
        }
        else {
            buf[i] = 0xdeadbeef;
        }
    }
    MPI_Ibcast(buf, COUNT, MPI_INT, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    for (i = 0; i < COUNT; ++i) {
        if (buf[i] != i)
            printf("buf[%d]=%d i=%d\n", i, buf[i], i);
        my_assert(buf[i] == i);
    }

    /* MPI_Ibcast (again, but designed to stress scatter/allgather impls) */
    buf_alias = (signed char *) buf;
    my_assert(COUNT * size * sizeof(int) > PRIME);      /* sanity */
    for (i = 0; i < PRIME; ++i) {
        if (rank == 0)
            buf_alias[i] = i;
        else
            buf_alias[i] = 0xdb;
    }
    for (i = PRIME; i < COUNT * size * sizeof(int); ++i) {
        buf_alias[i] = 0xbf;
    }
    MPI_Ibcast(buf_alias, PRIME, MPI_SIGNED_CHAR, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < PRIME; ++i) {
        if (buf_alias[i] != i)
            printf("buf_alias[%d]=%d i=%d\n", i, buf_alias[i], i);
        my_assert(buf_alias[i] == i);
    }

    /* MPI_Ibarrier */
    MPI_Ibarrier(MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    /* MPI_Ireduce */
    for (i = 0; i < COUNT; ++i) {
        buf[i] = rank + i;
        recvbuf[i] = 0xdeadbeef;
    }
    MPI_Ireduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    if (rank == 0) {
        for (i = 0; i < COUNT; ++i) {
            if (recvbuf[i] != ((size * (size - 1) / 2) + (i * size)))
                printf("got recvbuf[%d]=%d, expected %d\n", i, recvbuf[i],
                       ((size * (size - 1) / 2) + (i * size)));
            my_assert(recvbuf[i] == ((size * (size - 1) / 2) + (i * size)));
        }
    }

    /* same again, use a user op and free it before the wait */
    {
        MPI_Op op = MPI_OP_NULL;
        MPI_Op_create(sum_fn, /*commute= */ 1, &op);

        for (i = 0; i < COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Ireduce(buf, recvbuf, COUNT, MPI_INT, op, 0, MPI_COMM_WORLD, &req);
        MPI_Op_free(&op);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        if (rank == 0) {
            for (i = 0; i < COUNT; ++i) {
                if (recvbuf[i] != ((size * (size - 1) / 2) + (i * size)))
                    printf("got recvbuf[%d]=%d, expected %d\n", i, recvbuf[i],
                           ((size * (size - 1) / 2) + (i * size)));
                my_assert(recvbuf[i] == ((size * (size - 1) / 2) + (i * size)));
            }
        }
    }

    /* MPI_Iallreduce */
    for (i = 0; i < COUNT; ++i) {
        buf[i] = rank + i;
        recvbuf[i] = 0xdeadbeef;
    }
    MPI_Iallreduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < COUNT; ++i) {
        if (recvbuf[i] != ((size * (size - 1) / 2) + (i * size)))
            printf("got recvbuf[%d]=%d, expected %d\n", i, recvbuf[i],
                   ((size * (size - 1) / 2) + (i * size)));
        my_assert(recvbuf[i] == ((size * (size - 1) / 2) + (i * size)));
    }

    /* MPI_Ialltoallv (a weak test, neither irregular nor sparse) */
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
                   MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /*printf("recvbuf[%d*COUNT+%d]=%d, expecting %d\n", i, j, recvbuf[i*COUNT+j], (i + (rank * j))); */
            my_assert(recvbuf[i * COUNT + j] == (i + (rank * j)));
        }
    }

    /* MPI_Igather */
    for (i = 0; i < size * COUNT; ++i) {
        buf[i] = rank + i;
        recvbuf[i] = 0xdeadbeef;
    }
    MPI_Igather(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
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

    /* same test again, just use a dup'ed datatype and free it before the wait */
    {
        MPI_Datatype type = MPI_DATATYPE_NULL;
        MPI_Type_dup(MPI_INT, &type);

        for (i = 0; i < size * COUNT; ++i) {
            buf[i] = rank + i;
            recvbuf[i] = 0xdeadbeef;
        }
        MPI_Igather(buf, COUNT, MPI_INT, recvbuf, COUNT, type, 0, MPI_COMM_WORLD, &req);
        MPI_Type_free(&type);   /* should cause implementations that don't refcount
                                 * correctly to blow up or hang in the wait */
        MPI_Wait(&req, MPI_STATUS_IGNORE);
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
    }

    /* MPI_Iscatter */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            if (rank == 0)
                buf[i * COUNT + j] = i + j;
            else
                buf[i * COUNT + j] = 0xdeadbeef;
            recvbuf[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MPI_Iscatter(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, 0, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (j = 0; j < COUNT; ++j) {
        my_assert(recvbuf[j] == rank + j);
    }
    if (rank != 0) {
        for (i = 0; i < size * COUNT; ++i) {
            /* check we didn't corrupt the sendbuf somehow */
            my_assert(buf[i] == 0xdeadbeef);
        }
    }

    /* MPI_Iscatterv */
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
    MPI_Iscatterv(buf, sendcounts, sdispls, MPI_INT, recvbuf, COUNT, MPI_INT, 0, MPI_COMM_WORLD,
                  &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
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

    /* MPI_Ireduce_scatter */
    for (i = 0; i < size; ++i) {
        recvcounts[i] = COUNT;
        for (j = 0; j < COUNT; ++j) {
            buf[i * COUNT + j] = rank + i;
            recvbuf[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MPI_Ireduce_scatter(buf, recvbuf, recvcounts, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (j = 0; j < COUNT; ++j) {
        my_assert(recvbuf[j] == (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf */
            my_assert(recvbuf[i * COUNT + j] == 0xdeadbeef);
        }
    }

    /* MPI_Ireduce_scatter_block */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            buf[i * COUNT + j] = rank + i;
            recvbuf[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MPI_Ireduce_scatter_block(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (j = 0; j < COUNT; ++j) {
        my_assert(recvbuf[j] == (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf */
            my_assert(recvbuf[i * COUNT + j] == 0xdeadbeef);
        }
    }

    /* MPI_Igatherv */
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
    MPI_Igatherv(buf, COUNT, MPI_INT, recvbuf, recvcounts, rdispls, MPI_INT, 0, MPI_COMM_WORLD,
                 &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
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

    /* MPI_Ialltoall */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            buf[i * COUNT + j] = rank + (i * j);
            recvbuf[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MPI_Ialltoall(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /*printf("recvbuf[%d*COUNT+%d]=%d, expecting %d\n", i, j, recvbuf[i*COUNT+j], (i + (i * j))); */
            my_assert(recvbuf[i * COUNT + j] == (i + (rank * j)));
        }
    }

    /* MPI_Iallgather */
    for (i = 0; i < size * COUNT; ++i) {
        buf[i] = rank + i;
        recvbuf[i] = 0xdeadbeef;
    }
    MPI_Iallgather(buf, COUNT, MPI_INT, recvbuf, COUNT, MPI_INT, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            my_assert(recvbuf[i * COUNT + j] == i + j);
        }
    }

    /* MPI_Iallgatherv */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            recvbuf[i * COUNT + j] = 0xdeadbeef;
        }
        recvcounts[i] = COUNT;
        rdispls[i] = i * COUNT;
    }
    for (i = 0; i < COUNT; ++i)
        buf[i] = rank + i;
    MPI_Iallgatherv(buf, COUNT, MPI_INT, recvbuf, recvcounts, rdispls, MPI_INT, MPI_COMM_WORLD,
                    &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            my_assert(recvbuf[i * COUNT + j] == i + j);
        }
    }

    /* MPI_Iscan */
    for (i = 0; i < COUNT; ++i) {
        buf[i] = rank + i;
        recvbuf[i] = 0xdeadbeef;
    }
    MPI_Iscan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < COUNT; ++i) {
        my_assert(recvbuf[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1))));
    }

    /* MPI_Iexscan */
    for (i = 0; i < COUNT; ++i) {
        buf[i] = rank + i;
        recvbuf[i] = 0xdeadbeef;
    }
    MPI_Iexscan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < COUNT; ++i) {
        if (rank == 0)
            my_assert(recvbuf[i] == 0xdeadbeef);
        else
            my_assert(recvbuf[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1)) - (rank + i)));
    }

    /* MPI_Ialltoallw (a weak test, neither irregular nor sparse) */
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
                   MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /*printf("recvbuf[%d*COUNT+%d]=%d, expecting %d\n", i, j, recvbuf[i*COUNT+j], (i + (rank * j))); */
            my_assert(recvbuf[i * COUNT + j] == (i + (rank * j)));
        }
    }

    if (rank == 0)
        printf(" No Errors\n");


    MPI_Finalize();
    free(buf);
    free(recvbuf);
    free(sendcounts);
    free(recvcounts);
    free(rdispls);
    free(sdispls);
    free(recvtypes);
    free(sendtypes);
    return 0;
}
