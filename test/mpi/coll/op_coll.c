/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* A test of all op-based collectives with support for GPU buffers */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include "mpitest.h"

#define COUNT (10)

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

int *buf;
int *buf_h;
int *recvbuf;
int *recvbuf_h;
int *recvcounts;
MPI_Request req;
mtest_mem_type_e memtype;

void test_op_coll_with_root(int rank, int size, int root);

int main(int argc, char **argv)
{
    int rank, size;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    if (rank % 2 == 0)
        memtype = MTestArgListGetMemType(head, "evenmemtype");
    else
        memtype = MTestArgListGetMemType(head, "oddmemtype");
    MTestArgListDestroy(head);

    MTestAlloc(COUNT * size * sizeof(int), memtype, (void **) &buf_h, (void **) &buf, 0);
    MTestAlloc(COUNT * size * sizeof(int), memtype, (void **) &recvbuf_h, (void **) &recvbuf, 0);
    recvcounts = malloc(size * sizeof(int));
    test_op_coll_with_root(rank, size, 0);
    test_op_coll_with_root(rank, size, size - 1);

    MTestFree(memtype, buf_h, buf);
    MTestFree(memtype, recvbuf_h, recvbuf);
    MTest_Finalize(0);
    free(recvcounts);
    return 0;
}

void test_op_coll_with_root(int rank, int size, int root)
{
    int i, j;

    /* MPI_Reduce */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Reduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);
    if (rank == root) {
        MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
        for (i = 0; i < COUNT; ++i) {
            if (recvbuf_h[i] != ((size * (size - 1) / 2) + (i * size)))
                printf("got recvbuf[%d]=%d, expected %d\n", i, recvbuf_h[i],
                       ((size * (size - 1) / 2) + (i * size)));
            my_assert(recvbuf_h[i] == ((size * (size - 1) / 2) + (i * size)));
        }
    }

    /* MPI_Ireduce */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Ireduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    if (rank == root) {
        MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
        for (i = 0; i < COUNT; ++i) {
            if (recvbuf_h[i] != ((size * (size - 1) / 2) + (i * size)))
                printf("got recvbuf_h[%d]=%d, expected %d\n", i, recvbuf_h[i],
                       ((size * (size - 1) / 2) + (i * size)));
            my_assert(recvbuf_h[i] == ((size * (size - 1) / 2) + (i * size)));
        }
    }

    /* MPI_Allreduce */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Allreduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (i = 0; i < COUNT; ++i) {
        if (recvbuf_h[i] != ((size * (size - 1) / 2) + (i * size)))
            printf("got recvbuf_h[%d]=%d, expected %d\n", i, recvbuf_h[i],
                   ((size * (size - 1) / 2) + (i * size)));
        my_assert(recvbuf_h[i] == ((size * (size - 1) / 2) + (i * size)));
    }

    /* MPI_Iallreduce */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Iallreduce(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (i = 0; i < COUNT; ++i) {
        if (recvbuf_h[i] != ((size * (size - 1) / 2) + (i * size)))
            printf("got recvbuf_h[%d]=%d, expected %d\n", i, recvbuf_h[i],
                   ((size * (size - 1) / 2) + (i * size)));
        my_assert(recvbuf_h[i] == ((size * (size - 1) / 2) + (i * size)));
    }

    /* MPI_Reduce_scatter */
    for (i = 0; i < size; ++i) {
        recvcounts[i] = COUNT;
        for (j = 0; j < COUNT; ++j) {
            buf_h[i * COUNT + j] = rank + i;
            recvbuf_h[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MTestCopyContent(buf_h, buf, size * COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Reduce_scatter(buf, recvbuf, recvcounts, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (j = 0; j < COUNT; ++j) {
        my_assert(recvbuf_h[j] == (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf_h */
            my_assert(recvbuf_h[i * COUNT + j] == 0xdeadbeef);
        }
    }

    /* MPI_Ireduce_scatter */
    for (i = 0; i < size; ++i) {
        recvcounts[i] = COUNT;
        for (j = 0; j < COUNT; ++j) {
            buf_h[i * COUNT + j] = rank + i;
            recvbuf_h[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MTestCopyContent(buf_h, buf, size * COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Ireduce_scatter(buf, recvbuf, recvcounts, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (j = 0; j < COUNT; ++j) {
        my_assert(recvbuf_h[j] == (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf_h */
            my_assert(recvbuf_h[i * COUNT + j] == 0xdeadbeef);
        }
    }

    /* MPI_Reduce_scatter_block */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            buf_h[i * COUNT + j] = rank + i;
            recvbuf_h[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MTestCopyContent(buf_h, buf, size * COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Reduce_scatter_block(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (j = 0; j < COUNT; ++j) {
        my_assert(recvbuf_h[j] == (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf_h */
            my_assert(recvbuf_h[i * COUNT + j] == 0xdeadbeef);
        }
    }

    /* MPI_Ireduce_scatter_block */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            buf_h[i * COUNT + j] = rank + i;
            recvbuf_h[i * COUNT + j] = 0xdeadbeef;
        }
    }
    MTestCopyContent(buf_h, buf, size * COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Ireduce_scatter_block(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (j = 0; j < COUNT; ++j) {
        my_assert(recvbuf_h[j] == (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf_h */
            my_assert(recvbuf_h[i * COUNT + j] == 0xdeadbeef);
        }
    }

    /* MPI_Scan */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Scan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (i = 0; i < COUNT; ++i) {
        my_assert(recvbuf_h[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1))));
    }

    /* MPI_Iscan */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Iscan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (i = 0; i < COUNT; ++i) {
        my_assert(recvbuf_h[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1))));
    }

    /* MPI_Exscan */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Exscan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (i = 0; i < COUNT; ++i) {
        if (rank != 0)
            my_assert(recvbuf_h[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1)) - (rank + i)));
    }

    /* MPI_Iexscan */
    for (i = 0; i < COUNT; ++i) {
        buf_h[i] = rank + i;
        recvbuf_h[i] = 0xdeadbeef;
    }
    MTestCopyContent(buf_h, buf, COUNT * sizeof(int), memtype);
    MTestCopyContent(recvbuf_h, recvbuf, COUNT * sizeof(int), memtype);
    MPI_Iexscan(buf, recvbuf, COUNT, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MTestCopyContent(recvbuf, recvbuf_h, COUNT * sizeof(int), memtype);
    for (i = 0; i < COUNT; ++i) {
        if (rank != 0)
            my_assert(recvbuf_h[i] == ((rank * (rank + 1) / 2) + (i * (rank + 1)) - (rank + i)));
    }
}
