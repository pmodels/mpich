/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* The communicator, datatypes and op of a nonblocking or persistent collective may be freed as
 * soon as the call returns. Free them right after the call, then complete (and for persistent
 * collectives, restart) the operation and check the result. */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_nb_early_free
int run(const char *arg);
#endif

#define N 64
#define STARTS 3

static int errs = 0;
static int rank, size;

static void check(int cond, const char *what, int it)
{
    if (!cond) {
        if (errs < 10) {
            fprintf(stderr, "%d: %s mismatch (start %d)\n", rank, what, it);
        }
        errs++;
    }
}

static void usum(void *in, void *inout, int *len, MPI_Datatype * dt)
{
    int *a = in;
    int *b = inout;
    for (int i = 0; i < *len; i++) {
        b[i] += a[i];
    }
}

/* bcast of a vector type: the gaps must stay untouched */
static void test_bcast(bool persistent)
{
    int buf[2 * N];
    int root = size - 1;
    MPI_Comm comm;
    MPI_Datatype vec;
    MPI_Request req;

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Type_vector(N, 1, 2, MPI_INT, &vec);
    MPI_Type_commit(&vec);

    for (int it = 0; it < (persistent ? STARTS : 1); it++) {
        for (int i = 0; i < 2 * N; i++) {
            buf[i] = (rank == root && i % 2 == 0) ? it * 1000 + i : -1;
        }
        if (it == 0) {
            if (persistent) {
                MPI_Bcast_init(buf, 1, vec, root, comm, MPI_INFO_NULL, &req);
            } else {
                MPI_Ibcast(buf, 1, vec, root, comm, &req);
            }
            MPI_Type_free(&vec);
            MPI_Comm_free(&comm);
        }
        if (persistent) {
            MPI_Start(&req);
        }
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        for (int i = 0; i < 2 * N; i++) {
            int expect = (i % 2 == 0) ? it * 1000 + i : -1;
            check(buf[i] == expect, persistent ? "persistent bcast" : "ibcast", it);
        }
    }
    if (persistent) {
        MPI_Request_free(&req);
    }
}

/* reduce and allreduce with a user op */
static void test_reduce(bool persistent, bool all)
{
    int sbuf[N], rbuf[N];
    int root = 1 % size;
    MPI_Comm comm;
    MPI_Op op;
    MPI_Request req;

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Op_create(usum, 1, &op);

    for (int it = 0; it < (persistent ? STARTS : 1); it++) {
        for (int i = 0; i < N; i++) {
            sbuf[i] = (rank + 1) * (i + 1) + it;
            rbuf[i] = -1;
        }
        if (it == 0) {
            if (all) {
                if (persistent) {
                    MPI_Allreduce_init(sbuf, rbuf, N, MPI_INT, op, comm, MPI_INFO_NULL, &req);
                } else {
                    MPI_Iallreduce(sbuf, rbuf, N, MPI_INT, op, comm, &req);
                }
            } else {
                if (persistent) {
                    MPI_Reduce_init(sbuf, rbuf, N, MPI_INT, op, root, comm, MPI_INFO_NULL, &req);
                } else {
                    MPI_Ireduce(sbuf, rbuf, N, MPI_INT, op, root, comm, &req);
                }
            }
            MPI_Op_free(&op);
            MPI_Comm_free(&comm);
        }
        if (persistent) {
            MPI_Start(&req);
        }
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        if (all || rank == root) {
            for (int i = 0; i < N; i++) {
                int expect = size * (size + 1) / 2 * (i + 1) + size * it;
                check(rbuf[i] == expect, all ? "allreduce" : "reduce", it);
            }
        }
    }
    if (persistent) {
        MPI_Request_free(&req);
    }
}

/* alltoallw with a different vector type for each peer */
static void test_alltoallw(bool persistent)
{
    int *sbuf = malloc(2 * size * N * sizeof(int));
    int *rbuf = malloc(2 * size * N * sizeof(int));
    int *counts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    MPI_Datatype *types = malloc(size * sizeof(MPI_Datatype));
    MPI_Comm comm;
    MPI_Request req;

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    for (int i = 0; i < size; i++) {
        counts[i] = 1;
        displs[i] = i * 2 * N * sizeof(int);
        MPI_Type_vector(N, 1, 2, MPI_INT, &types[i]);
        MPI_Type_commit(&types[i]);
    }

    for (int it = 0; it < (persistent ? STARTS : 1); it++) {
        for (int i = 0; i < 2 * size * N; i++) {
            sbuf[i] = (i % 2 == 0) ? (rank * 100 + it) * 10000 + i : -2;
            rbuf[i] = -1;
        }
        if (it == 0) {
            if (persistent) {
                MPI_Alltoallw_init(sbuf, counts, displs, types, rbuf, counts, displs, types,
                                   comm, MPI_INFO_NULL, &req);
            } else {
                MPI_Ialltoallw(sbuf, counts, displs, types, rbuf, counts, displs, types, comm,
                               &req);
            }
            for (int i = 0; i < size; i++) {
                MPI_Type_free(&types[i]);
            }
            MPI_Comm_free(&comm);
        }
        if (persistent) {
            MPI_Start(&req);
        }
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        for (int peer = 0; peer < size; peer++) {
            for (int j = 0; j < 2 * N; j++) {
                int i = peer * 2 * N + j;
                /* what peer sent to us is the block at our index in its sbuf */
                int expect = -1;
                if (j % 2 == 0) {
                    expect = (peer * 100 + it) * 10000 + (rank * 2 * N + j);
                }
                check(rbuf[i] == expect, persistent ? "persistent alltoallw" : "ialltoallw", it);
            }
        }
    }
    if (persistent) {
        MPI_Request_free(&req);
    }

    free(sbuf);
    free(rbuf);
    free(counts);
    free(displs);
    free(types);
}

int run(const char *arg)
{
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (int persistent = 0; persistent < 2; persistent++) {
        test_bcast(persistent);
        test_reduce(persistent, false);
        test_reduce(persistent, true);
        test_alltoallw(persistent);
    }

    return errs;
}
