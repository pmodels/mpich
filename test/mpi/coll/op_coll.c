/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* A test of all op-based collectives with support for GPU buffers */

#include "mpitest.h"
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include "mtest_dtp.h"

#define COUNT (10)

#define my_assert(cond_)                                                  \
    do {                                                                  \
        if (!(cond_)) {                                                   \
            printf("assertion (%s) failed at line %d, aborting\n", #cond_, __LINE__); \
            MPI_Abort(MPI_COMM_WORLD, 1);                                 \
        }                                                                 \
    } while (0)

int *buf;
int *buf_h;
int *recvbuf;
int *recvbuf_h;
int *recvcounts;
MPI_Request req;
mtest_mem_type_e memtype;

void test_op_coll_with_root(int rank, int size, int root);

static int run_test(mtest_mem_type_e oddmem, mtest_mem_type_e evenmem)
{
    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        MTestPrintfMsg(1, "./op_coll -evenmemtype=%s -oddmemtype=%s\n",
                       MTest_memtype_name(evenmem), MTest_memtype_name(oddmem));
    }

    if (rank % 2 == 0)
        memtype = evenmem;
    else
        memtype = oddmem;

    MTestMalloc(COUNT * size * sizeof(int), memtype, (void **) &buf_h, (void **) &buf, rank);
    MTestMalloc(COUNT * size * sizeof(int), memtype, (void **) &recvbuf_h, (void **) &recvbuf,
                rank);
    recvcounts = malloc(size * sizeof(int));
    test_op_coll_with_root(rank, size, 0);
    test_op_coll_with_root(rank, size, size - 1);

    MTestFree(memtype, buf_h, buf);
    MTestFree(memtype, recvbuf_h, recvbuf);
    free(recvcounts);
    return 0;
}

int main(int argc, char **argv)
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_COLL_NOCOUNT, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += run_test(dtp_args.u.coll.evenmem, dtp_args.u.coll.oddmem);
    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
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
