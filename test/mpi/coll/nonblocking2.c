/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* A basic test of all 17 nonblocking collective operations specified by the
 * MPI-3 standard.  It only exercises the intracommunicator functionality,
 * does not use MPI_IN_PLACE, and only transmits/receives simple integer types
 * with relatively small counts.  It does check a few fancier issues, such as
 * ensuring that "premature user releases" of MPI_Op and MPI_Datatype objects
 * does not result in an error or segfault. */

#include "mpitest.h"
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define COUNT (10)
#define PRIME (17)

#define my_check(recvbuf, idx, val) \
    do { \
        if (recvbuf[idx] != (val)) { \
            if (errs < 10) { \
                fprintf(stderr, "Line %d: %s[%d] expect %d, got %d\n", __LINE__, #recvbuf, idx, val, recvbuf[idx]); \
            } \
            errs++; \
        } \
    } while (0)

static void sum_fn(void *invec, void *inoutvec, int *len, MPI_Datatype * datatype)
{
    int i;
    int *in = invec;
    int *inout = inoutvec;

    if (*datatype == MPI_INT) {
        for (i = 0; i < *len; ++i) {
            inout[i] = in[i] + inout[i];
        }
    } else {
        int stride;
        MPI_Aint lb, extent;
        MPI_Type_get_extent(*datatype, &lb, &extent);

        stride = extent / sizeof(int);

        for (i = 0; i < *len; ++i) {
            inout[i * stride] += in[i * stride];
        }
    }
}

int *buf = NULL;
int *recvbuf = NULL;
int *sendcounts = NULL;
int *recvcounts = NULL;
int *sdispls = NULL;
int *rdispls = NULL;
MPI_Datatype *sendtypes = NULL;
MPI_Datatype *recvtypes = NULL;
signed char *buf_alias = NULL;
MPI_Request req;

int test_icoll_with_root(int rank, int size, int root, MPI_Datatype datatype,
                         int stride, MPI_Op op_user);

int main(int argc, char **argv)
{
    int errs = 0;
    int rank, size;
    int noncontig_stride = 2;
    MPI_Datatype dt_noncontig;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Datatype dt_vector;
    MPI_Type_vector(1, 1, noncontig_stride, MPI_INT, &dt_vector);
    MPI_Type_create_resized(dt_vector, 0, sizeof(int) * noncontig_stride, &dt_noncontig);
    MPI_Type_free(&dt_vector);
    MPI_Type_commit(&dt_noncontig);

    MPI_Op op = MPI_OP_NULL;
    MPI_Op_create(sum_fn, /*commute= */ 1, &op);

    buf = malloc(COUNT * size * sizeof(int) * noncontig_stride);
    recvbuf = malloc(COUNT * size * sizeof(int) * noncontig_stride);
    sendcounts = malloc(size * sizeof(int));
    recvcounts = malloc(size * sizeof(int));
    sdispls = malloc(size * sizeof(int));
    rdispls = malloc(size * sizeof(int));
    sendtypes = malloc(size * sizeof(MPI_Datatype));
    recvtypes = malloc(size * sizeof(MPI_Datatype));

    errs += test_icoll_with_root(rank, size, 0, MPI_INT, 1, op);
    errs += test_icoll_with_root(rank, size, size - 1, MPI_INT, 1, op);
    errs += test_icoll_with_root(rank, size, 0, dt_noncontig, noncontig_stride, op);
    errs += test_icoll_with_root(rank, size, size - 1, dt_noncontig, noncontig_stride, op);

    MPI_Type_free(&dt_noncontig);
    MPI_Op_free(&op);
    MTest_Finalize(errs);
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

int test_icoll_with_root(int rank, int size, int root, MPI_Datatype datatype,
                         int stride, MPI_Op op_user)
{
    int errs = 0;
    int i, j;

    MPI_Op op_sum = MPI_SUM;
    if (datatype != MPI_INT) {
        op_sum = op_user;
    }

    /* MPI_Ibcast */
    for (i = 0; i < COUNT; ++i) {
        if (rank == root) {
            buf[i * stride] = i;
        } else {
            buf[i * stride] = 0xdeadbeef;
        }
    }
    MPI_Ibcast(buf, COUNT, datatype, root, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    for (i = 0; i < COUNT; ++i) {
        my_check(buf, i * stride, i);
    }

    /* MPI_Ibcast (again, but designed to stress scatter/allgather impls) */
    if (datatype == MPI_INT) {
        buf_alias = (signed char *) buf;
        assert(COUNT * size * sizeof(int) > PRIME);
        for (i = 0; i < PRIME; ++i) {
            if (rank == root)
                buf_alias[i] = i;
            else
                buf_alias[i] = 0xdb;
        }
        for (i = PRIME; i < COUNT * size * sizeof(int); ++i) {
            buf_alias[i] = 0xbf;
        }
        MPI_Ibcast(buf_alias, PRIME, MPI_SIGNED_CHAR, root, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        for (i = 0; i < PRIME; ++i) {
            my_check(buf_alias, i, i);
        }
    }

    /* MPI_Ibarrier */
    if (datatype == MPI_INT) {
        MPI_Ibarrier(MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    /* MPI_Ireduce */
    for (i = 0; i < COUNT; ++i) {
        buf[i * stride] = rank + i;
        recvbuf[i * stride] = 0xdeadbeef;
    }
    MPI_Ireduce(buf, recvbuf, COUNT, datatype, op_sum, root, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    if (rank == root) {
        for (i = 0; i < COUNT; ++i) {
            my_check(recvbuf, i * stride, ((size * (size - 1) / 2) + (i * size)));
        }
    }

    /* same again, use a user op and free it before the wait */
    {
        MPI_Op tmp_op = MPI_OP_NULL;
        MPI_Op_create(sum_fn, /*commute= */ 1, &tmp_op);

        for (i = 0; i < COUNT; ++i) {
            buf[i * stride] = rank + i;
            recvbuf[i * stride] = 0xdeadbeef;
        }
        MPI_Ireduce(buf, recvbuf, COUNT, datatype, tmp_op, root, MPI_COMM_WORLD, &req);
        MPI_Op_free(&tmp_op);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        if (rank == root) {
            for (i = 0; i < COUNT; ++i) {
                my_check(recvbuf, i * stride, ((size * (size - 1) / 2) + (i * size)));
            }
        }
    }

    /* MPI_Iallreduce */
    for (i = 0; i < COUNT; ++i) {
        buf[i * stride] = rank + i;
        recvbuf[i * stride] = 0xdeadbeef;
    }
    MPI_Iallreduce(buf, recvbuf, COUNT, datatype, op_sum, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < COUNT; ++i) {
        my_check(recvbuf, i * stride, ((size * (size - 1) / 2) + (i * size)));
    }

    /* MPI_Ialltoallv (a weak test, neither irregular nor sparse) */
    for (i = 0; i < size; ++i) {
        sendcounts[i] = COUNT;
        recvcounts[i] = COUNT;
        sdispls[i] = COUNT * i;
        rdispls[i] = COUNT * i;
        for (j = 0; j < COUNT; ++j) {
            buf[(i * COUNT + j) * stride] = rank + (i * j);
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
    }
    MPI_Ialltoallv(buf, sendcounts, sdispls, datatype, recvbuf, recvcounts, rdispls, datatype,
                   MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            my_check(recvbuf, (i * COUNT + j) * stride, (i + (rank * j)));
        }
    }

    /* MPI_Igather */
    for (i = 0; i < size * COUNT; ++i) {
        buf[i * stride] = rank + i;
        recvbuf[i * stride] = 0xdeadbeef;
    }
    MPI_Igather(buf, COUNT, datatype, recvbuf, COUNT, datatype, root, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    if (rank == root) {
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                my_check(recvbuf, (i * COUNT + j) * stride, i + j);
            }
        }
    } else {
        for (i = 0; i < size * COUNT; ++i) {
            my_check(recvbuf, i * stride, 0xdeadbeef);
        }
    }

    /* same test again, just use a dup'ed datatype and free it before the wait */
    {
        MPI_Datatype type = MPI_DATATYPE_NULL;
        MPI_Type_dup(datatype, &type);

        for (i = 0; i < size * COUNT; ++i) {
            buf[i * stride] = rank + i;
            recvbuf[i * stride] = 0xdeadbeef;
        }
        MPI_Igather(buf, COUNT, datatype, recvbuf, COUNT, type, root, MPI_COMM_WORLD, &req);
        MPI_Type_free(&type);   /* should cause implementations that don't refcount
                                 * correctly to blow up or hang in the wait */
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        if (rank == root) {
            for (i = 0; i < size; ++i) {
                for (j = 0; j < COUNT; ++j) {
                    my_check(recvbuf, (i * COUNT + j) * stride, i + j);
                }
            }
        } else {
            for (i = 0; i < size * COUNT; ++i) {
                my_check(recvbuf, i * stride, 0xdeadbeef);
            }
        }
    }

    /* MPI_Iscatter */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            if (rank == root)
                buf[(i * COUNT + j) * stride] = i + j;
            else
                buf[(i * COUNT + j) * stride] = 0xdeadbeef;
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
    }
    MPI_Iscatter(buf, COUNT, datatype, recvbuf, COUNT, datatype, root, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (j = 0; j < COUNT; ++j) {
        my_check(recvbuf, j * stride, rank + j);
    }
    if (rank != root) {
        for (i = 0; i < size * COUNT; ++i) {
            /* check we didn't corrupt the sendbuf somehow */
            my_check(buf, i * stride, 0xdeadbeef);
        }
    }

    /* MPI_Iscatterv */
    for (i = 0; i < size; ++i) {
        /* weak test, just test the regular case where all counts are equal */
        sendcounts[i] = COUNT;
        sdispls[i] = i * COUNT;
        for (j = 0; j < COUNT; ++j) {
            if (rank == root)
                buf[(i * COUNT + j) * stride] = i + j;
            else
                buf[(i * COUNT + j) * stride] = 0xdeadbeef;
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
    }
    MPI_Iscatterv(buf, sendcounts, sdispls, datatype, recvbuf, COUNT, datatype, root,
                  MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (j = 0; j < COUNT; ++j) {
        my_check(recvbuf, j * stride, rank + j);
    }
    if (rank != root) {
        for (i = 0; i < size * COUNT; ++i) {
            /* check we didn't corrupt the sendbuf somehow */
            my_check(buf, i * stride, 0xdeadbeef);
        }
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf */
            my_check(recvbuf, (i * COUNT + j) * stride, 0xdeadbeef);
        }
    }

    /* MPI_Ireduce_scatter */
    for (i = 0; i < size; ++i) {
        recvcounts[i] = COUNT;
        for (j = 0; j < COUNT; ++j) {
            buf[(i * COUNT + j) * stride] = rank + i;
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
    }
    MPI_Ireduce_scatter(buf, recvbuf, recvcounts, datatype, op_sum, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (j = 0; j < COUNT; ++j) {
        my_check(recvbuf, j * stride, (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf */
            my_check(recvbuf, (i * COUNT + j) * stride, 0xdeadbeef);
        }
    }

    /* MPI_Ireduce_scatter_block */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            buf[(i * COUNT + j) * stride] = rank + i;
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
    }
    MPI_Ireduce_scatter_block(buf, recvbuf, COUNT, datatype, op_sum, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (j = 0; j < COUNT; ++j) {
        my_check(recvbuf, j * stride, (size * rank + ((size - 1) * size) / 2));
    }
    for (i = 1; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            /* check we didn't corrupt the rest of the recvbuf */
            my_check(recvbuf, (i * COUNT + j) * stride, 0xdeadbeef);
        }
    }

    /* MPI_Igatherv */
    for (i = 0; i < size * COUNT; ++i) {
        buf[i * stride] = 0xdeadbeef;
        recvbuf[i * stride] = 0xdeadbeef;
    }
    for (i = 0; i < COUNT; ++i) {
        buf[i * stride] = rank + i;
    }
    for (i = 0; i < size; ++i) {
        recvcounts[i] = COUNT;
        rdispls[i] = i * COUNT;
    }
    MPI_Igatherv(buf, COUNT, datatype, recvbuf, recvcounts, rdispls, datatype, root, MPI_COMM_WORLD,
                 &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    if (rank == root) {
        for (i = 0; i < size; ++i) {
            for (j = 0; j < COUNT; ++j) {
                my_check(recvbuf, (i * COUNT + j) * stride, i + j);
            }
        }
    } else {
        for (i = 0; i < size * COUNT; ++i) {
            my_check(recvbuf, i * stride, 0xdeadbeef);
        }
    }

    /* MPI_Ialltoall */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            buf[(i * COUNT + j) * stride] = rank + (i * j);
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
    }
    MPI_Ialltoall(buf, COUNT, datatype, recvbuf, COUNT, datatype, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            my_check(recvbuf, (i * COUNT + j) * stride, (i + (rank * j)));
        }
    }

    /* MPI_Iallgather */
    for (i = 0; i < size * COUNT; ++i) {
        buf[i * stride] = rank + i;
        recvbuf[i * stride] = 0xdeadbeef;
    }
    MPI_Iallgather(buf, COUNT, datatype, recvbuf, COUNT, datatype, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            my_check(recvbuf, (i * COUNT + j) * stride, i + j);
        }
    }

    /* MPI_Iallgatherv */
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
        recvcounts[i] = COUNT;
        rdispls[i] = i * COUNT;
    }
    for (i = 0; i < COUNT; ++i)
        buf[i * stride] = rank + i;
    MPI_Iallgatherv(buf, COUNT, datatype, recvbuf, recvcounts, rdispls, datatype, MPI_COMM_WORLD,
                    &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            my_check(recvbuf, (i * COUNT + j) * stride, i + j);
        }
    }

    /* MPI_Iscan */
    for (i = 0; i < COUNT; ++i) {
        buf[i * stride] = rank + i;
        recvbuf[i * stride] = 0xdeadbeef;
    }
    MPI_Iscan(buf, recvbuf, COUNT, datatype, op_sum, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < COUNT; ++i) {
        my_check(recvbuf, i * stride, ((rank * (rank + 1) / 2) + (i * (rank + 1))));
    }

    /* MPI_Iexscan */
    for (i = 0; i < COUNT; ++i) {
        buf[i * stride] = rank + i;
        recvbuf[i * stride] = 0xdeadbeef;
    }
    MPI_Iexscan(buf, recvbuf, COUNT, datatype, op_sum, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < COUNT; ++i) {
        if (rank != 0)
            my_check(recvbuf, i * stride,
                     ((rank * (rank + 1) / 2) + (i * (rank + 1)) - (rank + i)));
    }

    /* MPI_Ialltoallw (a weak test, neither irregular nor sparse) */
    for (i = 0; i < size; ++i) {
        sendcounts[i] = COUNT;
        recvcounts[i] = COUNT;
        sdispls[i] = COUNT * i * sizeof(int) * stride;
        rdispls[i] = COUNT * i * sizeof(int) * stride;
        sendtypes[i] = datatype;
        recvtypes[i] = datatype;
        for (j = 0; j < COUNT; ++j) {
            buf[(i * COUNT + j) * stride] = rank + (i * j);
            recvbuf[(i * COUNT + j) * stride] = 0xdeadbeef;
        }
    }
    MPI_Ialltoallw(buf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                   MPI_COMM_WORLD, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    for (i = 0; i < size; ++i) {
        for (j = 0; j < COUNT; ++j) {
            my_check(recvbuf, (i * COUNT + j) * stride, (i + (rank * j)));
        }
    }
    return errs;
}
