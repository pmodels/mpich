/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/* Basic tests for collective large APIs. */

#if 0
#define CTYPE   int
#define MPITYPE MPI_INT
#else
#define CTYPE   char
#define MPITYPE MPI_SIGNED_CHAR
#endif
#define STRIDE 2        /* stride for noncontig datatype */

#define MAX_PROC 20

enum {
    INVALID,
    ALLGATHER,
    ALLGATHERV,
    ALLREDUCE,
    ALLTOALL,
    ALLTOALLV,
    ALLTOALLW,
    BCAST,
    GATHER,
    GATHERV,
    REDUCE,
    SCATTER,
    SCATTERV,
    SCAN,
    EXSCAN,
    REDUCE_SCATTER,
    REDUCE_SCATTER_BLOCK,
    REDUCE_LOCAL,
    ALLCOLL,
};

static int parse_coll_name(const char *name);
static void init_buf(void **buf, MPI_Count count, int stride, CTYPE val);
static void check_buf(void *buf, MPI_Count count, int stride, CTYPE val);

typedef void (*TEST_FN) (MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);

static void test_ALLGATHER(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_ALLGATHERV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_ALLREDUCE(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_ALLTOALL(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_ALLTOALLV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_ALLTOALLW(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_BCAST(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_GATHER(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_GATHERV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_REDUCE(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_SCATTER(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_SCATTERV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_SCAN(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_EXSCAN(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);
static void test_REDUCE_SCATTER(MPI_Count count, MPI_Datatype datatype, int stride,
                                bool is_nonblock);
static void test_REDUCE_SCATTER_BLOCK(MPI_Count count, MPI_Datatype datatype, int stride,
                                      bool is_nonblock);
static void test_REDUCE_LOCAL(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock);

static void uop(void *inbuf, void *inoutbuf, int *count, MPI_Datatype * dtype);

TEST_FN tests[ALLCOLL] = {
    0,
    test_ALLGATHER,
    test_ALLGATHERV,
    test_ALLREDUCE,
    test_ALLTOALL,
    test_ALLTOALLV,
    test_ALLTOALLW,
    test_BCAST,
    test_GATHER,
    test_GATHERV,
    test_REDUCE,
    test_SCATTER,
    test_SCATTERV,
    test_SCAN,
    test_EXSCAN,
    test_REDUCE_SCATTER,
    test_REDUCE_SCATTER_BLOCK,
    test_REDUCE_LOCAL,
};

int errs;
MPI_Comm comm = MPI_COMM_WORLD;
int rank, nprocs;

MPI_Op use_op = MPI_SUM;

int main(int argc, char **argv)
{
    /* the tests array must match the enum order. Use a simple assert to check. */
    assert(tests[REDUCE_LOCAL] == test_REDUCE_LOCAL);

    int option = 0;
    int coll = INVALID;
    bool is_nonblock = false;
    const char *coll_name = NULL;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-option=", 8) == 0) {
            option = atoi(argv[i] + 8);
        } else if (strncmp(argv[i], "-coll=", 6) == 0) {
            coll_name = argv[i] + 6;
            coll = parse_coll_name(argv[i] + 6);
        } else if (strncmp(argv[i], "-nonblock", 9) == 0) {
            is_nonblock = true;
        }
    }

    if (coll == INVALID) {
        printf("Invalid -coll= option\n");
        goto fn_exit;
    }

    MTest_Init(&argc, &argv);

    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);
    if (nprocs > MAX_PROC) {
        printf("The number of procs (%d) exceed MAX_PROC (%d)\n", nprocs, MAX_PROC);
        goto fn_exit;
    }

    MPI_Count count;
    MPI_Datatype datatype;
    int stride;
    bool datatype_need_free = false;

    switch (option) {
        case 0:
            /* small count, just test the _c API */
            count = 1024;
            datatype = MPITYPE;
            stride = 1;
            break;
        case 1:
            /* large count, contiguous datatype */
            count = (MPI_Count) INT_MAX + 100;
            datatype = MPITYPE;
            stride = 1;
            break;
        case 2:
            /* large count, non-contig datatype */
            count = (MPI_Count) INT_MAX + 100;
            MPI_Type_create_resized(MPITYPE, 0, sizeof(CTYPE) * STRIDE, &datatype);
            MPI_Type_commit(&datatype);
            stride = STRIDE;
            datatype_need_free = true;

            MPI_Op_create(uop, 1, &use_op);
            break;
        default:
            if (rank == 0) {
                printf("Unexpected option: %d\n", option);
            }
            goto fn_exit;
    }

    for (int c = INVALID + 1; c < ALLCOLL; c++) {
        if (coll == ALLCOLL || coll == c) {
            tests[c] (count, datatype, stride, is_nonblock);
        }
    }

    if (datatype_need_free) {
        MPI_Type_free(&datatype);
        MPI_Op_free(&use_op);
    }

    MTest_Finalize(errs);
  fn_exit:
    return MTestReturnValue(errs);
}

static int parse_coll_name(const char *name)
{
    if (strcmp(name, "allgather") == 0)
        return ALLGATHER;
    if (strcmp(name, "allgatherv") == 0)
        return ALLGATHERV;
    if (strcmp(name, "allreduce") == 0)
        return ALLREDUCE;
    if (strcmp(name, "alltoall") == 0)
        return ALLTOALL;
    if (strcmp(name, "alltoallv") == 0)
        return ALLTOALLV;
    if (strcmp(name, "alltoallw") == 0)
        return ALLTOALLW;
    if (strcmp(name, "bcast") == 0)
        return BCAST;
    if (strcmp(name, "gather") == 0)
        return GATHER;
    if (strcmp(name, "gatherv") == 0)
        return GATHERV;
    if (strcmp(name, "reduce") == 0)
        return REDUCE;
    if (strcmp(name, "scatter") == 0)
        return SCATTER;
    if (strcmp(name, "scatterv") == 0)
        return SCATTERV;
    if (strcmp(name, "scan") == 0)
        return SCAN;
    if (strcmp(name, "exscan") == 0)
        return EXSCAN;
    if (strcmp(name, "reduce_scatter") == 0)
        return REDUCE_SCATTER;
    if (strcmp(name, "reduce_scatter_block") == 0)
        return REDUCE_SCATTER_BLOCK;
    if (strcmp(name, "reduce_local") == 0)
        return REDUCE_LOCAL;
    if (strcmp(name, "all") == 0)
        return ALLCOLL;
    printf("Invalid coll name \"%s\"\n", name);
    return INVALID;
}

static void init_buf(void **buf, MPI_Count count, int stride, CTYPE val)
{
    MPI_Aint total_size = count * stride * sizeof(CTYPE);
    *buf = malloc(total_size);

    CTYPE *intbuf = *buf;
    for (MPI_Count i = 0; i < count; i++) {
        intbuf[i * stride] = val;
    }
}

static void check_buf(void *buf, MPI_Count count, int stride, CTYPE val)
{
    CTYPE *intbuf = buf;
    for (MPI_Count i = 0; i < count; i++) {
        if (intbuf[i * stride] != val) {
            if (errs < 10) {
                printf("i = %ld, expect %d, got %d\n",
                       (long) i, (int) val, (int) intbuf[i * stride]);
            }
            errs++;
        }
    }
}

static void uop(void *inbuf, void *inoutbuf, int *count, MPI_Datatype * dtype)
{
    CTYPE *in = inbuf;
    CTYPE *inout = inoutbuf;
    for (int i = 0; i < (*count * STRIDE); i += STRIDE) {
        inout[i] += in[i];
    }
}

#define ANNOUNCE(testname) \
    do { \
        if (rank == 0) { \
            MTestPrintfMsg(1, "Test large count %s, is_nonblock=%d, count=%ld\n", #testname, is_nonblock, (long) count); \
        } \
    } while (0)

static void test_ALLGATHER(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(ALLGATHER);

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Allgather_c(sendbuf, count, datatype, recvbuf, count, datatype, comm);
    } else {
        MPI_Request req;
        MPI_Iallgather_c(sendbuf, count, datatype, recvbuf, count, datatype, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < nprocs; i++) {
        void *buf = (char *) recvbuf + i * (count * stride * sizeof(CTYPE));
        check_buf(buf, count, stride, i);
    }
    free(sendbuf);
    free(recvbuf);
}

static void test_ALLGATHERV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(ALLGATHERV);

    MPI_Count counts[MAX_PROC];
    MPI_Aint displs[MAX_PROC];
    for (int i = 0; i < nprocs; i++) {
        counts[i] = count;
        displs[i] = i * count;
    }

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Allgatherv_c(sendbuf, count, datatype, recvbuf, counts, displs, datatype, comm);
    } else {
        MPI_Request req;
        MPI_Iallgatherv_c(sendbuf, count, datatype, recvbuf, counts, displs, datatype, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < nprocs; i++) {
        void *buf = (char *) recvbuf + i * (count * stride * sizeof(CTYPE));
        check_buf(buf, count, stride, i);
    }
    free(sendbuf);
    free(recvbuf);
}

static void test_ALLREDUCE(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(ALLREDUCE);

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    init_buf(&recvbuf, count, stride, -1);

    if (!is_nonblock) {
        MPI_Allreduce_c(sendbuf, recvbuf, count, datatype, use_op, comm);
    } else {
        MPI_Request req;
        MPI_Iallreduce_c(sendbuf, recvbuf, count, datatype, use_op, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < nprocs; i++) {
        int val = nprocs * (nprocs - 1) / 2;
        check_buf(recvbuf, count, stride, val);
    }
    free(sendbuf);
    free(recvbuf);
}

static void test_ALLTOALL(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(ALLTOALL);

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count * nprocs, stride, rank);
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Alltoall_c(sendbuf, count, datatype, recvbuf, count, datatype, comm);
    } else {
        MPI_Request req;
        MPI_Ialltoall_c(sendbuf, count, datatype, recvbuf, count, datatype, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < nprocs; i++) {
        void *buf = (char *) recvbuf + i * (count * stride * sizeof(CTYPE));
        check_buf(buf, count, stride, i);
    }
    free(sendbuf);
    free(recvbuf);
}

static void test_ALLTOALLV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(ALLTOALLV);

    MPI_Count counts[MAX_PROC];
    MPI_Aint displs[MAX_PROC];
    for (int i = 0; i < nprocs; i++) {
        counts[i] = count;
        displs[i] = i * count;
    }

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count * nprocs, stride, rank);
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Alltoallv_c(sendbuf, counts, displs, datatype, recvbuf, counts, displs, datatype, comm);
    } else {
        MPI_Request req;
        MPI_Ialltoallv_c(sendbuf, counts, displs, datatype, recvbuf, counts, displs, datatype, comm,
                         &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < nprocs; i++) {
        void *buf = (char *) recvbuf + i * (count * stride * sizeof(CTYPE));
        check_buf(buf, count, stride, i);
    }
    free(sendbuf);
    free(recvbuf);
}

static void test_ALLTOALLW(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(ALLTOALLW);

    MPI_Count counts[MAX_PROC];
    MPI_Aint displs[MAX_PROC];
    MPI_Datatype types[MAX_PROC];
    for (int i = 0; i < nprocs; i++) {
        counts[i] = count;
        displs[i] = i * count * stride * sizeof(CTYPE);
        types[i] = datatype;
    }

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count * nprocs, stride, rank);
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Alltoallw_c(sendbuf, counts, displs, types, recvbuf, counts, displs, types, comm);
    } else {
        MPI_Request req;
        MPI_Ialltoallw_c(sendbuf, counts, displs, types, recvbuf, counts, displs, types, comm,
                         &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < nprocs; i++) {
        void *buf = (char *) recvbuf + i * (count * stride * sizeof(CTYPE));
        check_buf(buf, count, stride, i);
    }
    free(sendbuf);
    free(recvbuf);
}

static void test_BCAST(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(BCAST);

    int root = 0;
    void *buf;
    if (rank == root) {
        init_buf(&buf, count, stride, rank);
    } else {
        init_buf(&buf, count, stride, -1);
    }

    if (!is_nonblock) {
        MPI_Bcast_c(buf, count, datatype, root, comm);
    } else {
        MPI_Request req;
        MPI_Ibcast_c(buf, count, datatype, root, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    if (rank != root) {
        check_buf(buf, count, stride, root);
    }
    free(buf);
}

static void test_GATHER(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(GATHER);

    int root = 0;

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    if (rank == root) {
        init_buf(&recvbuf, count * nprocs, stride, -1);
    }

    if (!is_nonblock) {
        MPI_Gather_c(sendbuf, count, datatype, recvbuf, count, datatype, root, comm);
    } else {
        MPI_Request req;
        MPI_Igather_c(sendbuf, count, datatype, recvbuf, count, datatype, root, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    if (rank == root) {
        for (int i = 0; i < nprocs; i++) {
            void *buf = (char *) recvbuf + i * (count * stride * sizeof(CTYPE));
            check_buf(buf, count, stride, i);
        }
    }
    free(sendbuf);
    if (rank == root) {
        free(recvbuf);
    }
}

static void test_GATHERV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(GATHERV);

    int root = 0;

    MPI_Count counts[MAX_PROC];
    MPI_Aint displs[MAX_PROC];
    if (rank == root) {
        for (int i = 0; i < nprocs; i++) {
            counts[i] = count;
            displs[i] = i * count;
        }
    }

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    if (rank == root) {
        init_buf(&recvbuf, count * nprocs, stride, -1);
    }

    if (!is_nonblock) {
        MPI_Gatherv_c(sendbuf, count, datatype, recvbuf, counts, displs, datatype, root, comm);
    } else {
        MPI_Request req;
        MPI_Igatherv_c(sendbuf, count, datatype, recvbuf, counts, displs, datatype, root, comm,
                       &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    if (rank == root) {
        for (int i = 0; i < nprocs; i++) {
            void *buf = (char *) recvbuf + i * (count * stride * sizeof(CTYPE));
            check_buf(buf, count, stride, i);
        }
        free(recvbuf);
    }
    free(sendbuf);
}

static void test_REDUCE(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(REDUCE);

    int root = 0;

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    if (rank == root) {
        init_buf(&recvbuf, count * nprocs, stride, -1);
    }

    if (!is_nonblock) {
        MPI_Reduce_c(sendbuf, recvbuf, count, datatype, use_op, root, comm);
    } else {
        MPI_Request req;
        MPI_Ireduce_c(sendbuf, recvbuf, count, datatype, use_op, root, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    if (rank == root) {
        int val = nprocs * (nprocs - 1) / 2;
        check_buf(recvbuf, count, stride, val);
        free(recvbuf);
    }
    free(sendbuf);
}

static void test_SCATTER(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(SCATTER);

    int root = 0;

    void *sendbuf, *recvbuf;
    if (rank == root) {
        init_buf(&sendbuf, count * nprocs, stride, rank);
    }
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Scatter_c(sendbuf, count, datatype, recvbuf, count, datatype, root, comm);
    } else {
        MPI_Request req;
        MPI_Iscatter_c(sendbuf, count, datatype, recvbuf, count, datatype, root, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    check_buf(recvbuf, count, stride, root);

    if (rank == root) {
        free(sendbuf);
    }
    free(recvbuf);
}

static void test_SCATTERV(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(SCATTERV);

    int root = 0;

    MPI_Count counts[MAX_PROC];
    MPI_Aint displs[MAX_PROC];
    if (rank == root) {
        for (int i = 0; i < nprocs; i++) {
            counts[i] = count;
            displs[i] = i * count;
        }
    }

    void *sendbuf, *recvbuf;
    if (rank == root) {
        init_buf(&sendbuf, count * nprocs, stride, rank);
    }
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Scatterv_c(sendbuf, counts, displs, datatype, recvbuf, count, datatype, root, comm);
    } else {
        MPI_Request req;
        MPI_Iscatterv_c(sendbuf, counts, displs, datatype, recvbuf, count, datatype, root, comm,
                        &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    check_buf(recvbuf, count, stride, root);

    if (rank == root) {
        free(sendbuf);
    }
    free(recvbuf);
}

static void test_SCAN(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(SCAN);

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Scan_c(sendbuf, recvbuf, count, datatype, use_op, comm);
    } else {
        MPI_Request req;
        MPI_Iscan_c(sendbuf, recvbuf, count, datatype, use_op, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    int val = rank * (rank + 1) / 2;
    check_buf(recvbuf, count, stride, val);

    free(sendbuf);
    free(recvbuf);
}

static void test_EXSCAN(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(EXSCAN);

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count, stride, rank);
    init_buf(&recvbuf, count * nprocs, stride, -1);

    if (!is_nonblock) {
        MPI_Exscan_c(sendbuf, recvbuf, count, datatype, use_op, comm);
    } else {
        MPI_Request req;
        MPI_Iexscan_c(sendbuf, recvbuf, count, datatype, use_op, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    if (rank > 0) {
        int val = rank * (rank - 1) / 2;
        check_buf(recvbuf, count, stride, val);
    }

    free(sendbuf);
    free(recvbuf);
}

static void test_REDUCE_SCATTER(MPI_Count count, MPI_Datatype datatype, int stride,
                                bool is_nonblock)
{
    ANNOUNCE(REDUCE_SCATTER);

    MPI_Count counts[MAX_PROC];
    for (int i = 0; i < nprocs; i++) {
        counts[i] = count;
    }

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count * nprocs, stride, rank);
    init_buf(&recvbuf, count, stride, -1);

    if (!is_nonblock) {
        MPI_Reduce_scatter_c(sendbuf, recvbuf, counts, datatype, use_op, comm);
    } else {
        MPI_Request req;
        MPI_Ireduce_scatter_c(sendbuf, recvbuf, counts, datatype, use_op, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    int val = nprocs * (nprocs - 1) / 2;
    check_buf(recvbuf, count, stride, val);

    free(sendbuf);
    free(recvbuf);
}

static void test_REDUCE_SCATTER_BLOCK(MPI_Count count, MPI_Datatype datatype, int stride,
                                      bool is_nonblock)
{
    ANNOUNCE(REDUCE_SCATTER_BLOCK);

    void *sendbuf, *recvbuf;
    init_buf(&sendbuf, count * nprocs, stride, rank);
    init_buf(&recvbuf, count, stride, -1);

    if (!is_nonblock) {
        MPI_Reduce_scatter_block_c(sendbuf, recvbuf, count, datatype, use_op, comm);
    } else {
        MPI_Request req;
        MPI_Ireduce_scatter_block_c(sendbuf, recvbuf, count, datatype, use_op, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }

    int val = nprocs * (nprocs - 1) / 2;
    check_buf(recvbuf, count, stride, val);

    free(sendbuf);
    free(recvbuf);
}

static void test_REDUCE_LOCAL(MPI_Count count, MPI_Datatype datatype, int stride, bool is_nonblock)
{
    ANNOUNCE(REDUCE_LOCAL);

    void *inbuf, *inoutbuf;
    init_buf(&inbuf, count, stride, 1);
    init_buf(&inoutbuf, count, stride, 2);

    MPI_Reduce_local_c(inbuf, inoutbuf, count, datatype, use_op);

    check_buf(inoutbuf, count, stride, 3);

    free(inbuf);
    free(inoutbuf);
}
