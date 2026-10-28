/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* MPI_IN_PLACE alltoall, alltoallv and alltoallw, blocking, nonblocking and persistent. The
 * alltoallv blocks have gaps and non-monotonic displacements, and the alltoallw blocks use
 * different types for different peers. The send and receive counts of a pair of ranks are
 * equal, as MPI_IN_PLACE requires, and the tests run the algorithms that are forced by CVAR
 * (see testlist). Persistent alltoall is started twice with new data, so the copy of the
 * in-place buffer must be repeated on every start. */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run coll_alltoall_inplace
int run(const char *arg);
#endif

#define GAP_INT (-1)
#define GAP_BYTE ((char) 0xAB)

/* elements from i to j; symmetric, so the counts agree in both directions */
static int nelem(int i, int j)
{
    return 1 + ((i + j) % 3);
}

/* the k-th element that src sends to dst in the given round */
static int value(int round, int src, int dst, int k)
{
    return round * 1000000 + src * 10000 + dst * 100 + k;
}

static int test_alltoall(MPI_Comm comm, int mode);
static int test_alltoallv(MPI_Comm comm, bool nonblocking);
static int test_alltoallw(MPI_Comm comm, bool nonblocking);

int run(const char *arg)
{
    int errs = 0;
    MPI_Comm comm = MPI_COMM_WORLD;

    /* mode 0: blocking, 1: nonblocking, 2: persistent */
    for (int mode = 0; mode < 3; mode++) {
        errs += test_alltoall(comm, mode);
    }
    errs += test_alltoallv(comm, false);
    errs += test_alltoallv(comm, true);
    errs += test_alltoallw(comm, false);
    errs += test_alltoallw(comm, true);

    return errs;
}

/* a block of 3 ints per peer, in place */
static int test_alltoall(MPI_Comm comm, int mode)
{
    int errs = 0;
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    const int count = 3;
    int *buf = malloc(size * count * sizeof(int));
    MPI_Request req = MPI_REQUEST_NULL;

    if (mode == 2) {
        MPI_Alltoall_init(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, buf, count, MPI_INT, comm,
                          MPI_INFO_NULL, &req);
    }

    for (int round = 0; round < 2; round++) {
        for (int j = 0; j < size; j++) {
            for (int k = 0; k < count; k++) {
                buf[j * count + k] = value(round, rank, j, k);
            }
        }

        if (mode == 0) {
            MPI_Alltoall(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, buf, count, MPI_INT, comm);
        } else if (mode == 1) {
            MPI_Ialltoall(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, buf, count, MPI_INT, comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        } else {
            MPI_Start(&req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }

        for (int j = 0; j < size; j++) {
            for (int k = 0; k < count; k++) {
                if (buf[j * count + k] != value(round, j, rank, k)) {
                    fprintf(stderr, "[%d] alltoall mode %d round %d: got %d expected %d "
                            "from %d, element %d\n", rank, mode, round,
                            buf[j * count + k], value(round, j, rank, k), j, k);
                    errs++;
                }
            }
        }
    }

    if (mode == 2) {
        MPI_Request_free(&req);
    }
    free(buf);
    return errs;
}

/* alltoallv with the blocks in reverse order, and a gap after each block */
static int test_alltoallv(MPI_Comm comm, bool nonblocking)
{
    int errs = 0;
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    const int stride = 4;
    int *counts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    int *buf = malloc(size * stride * sizeof(int));

    for (int j = 0; j < size; j++) {
        counts[j] = nelem(j, rank);
        displs[j] = (size - 1 - j) * stride;
    }

    for (int i = 0; i < size * stride; i++) {
        buf[i] = GAP_INT;
    }
    for (int j = 0; j < size; j++) {
        for (int k = 0; k < counts[j]; k++) {
            buf[displs[j] + k] = value(0, rank, j, k);
        }
    }

    if (nonblocking) {
        MPI_Request req;
        MPI_Ialltoallv(MPI_IN_PLACE, NULL, NULL, MPI_DATATYPE_NULL, buf, counts, displs,
                       MPI_INT, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        MPI_Alltoallv(MPI_IN_PLACE, NULL, NULL, MPI_DATATYPE_NULL, buf, counts, displs,
                      MPI_INT, comm);
    }

    for (int j = 0; j < size; j++) {
        for (int k = 0; k < stride; k++) {
            int pos = displs[j] + k;
            int expected = (k < counts[j]) ? value(0, j, rank, k) : GAP_INT;
            if (buf[pos] != expected) {
                fprintf(stderr, "[%d] alltoallv nb=%d: got %d expected %d at block %d, "
                        "element %d\n", rank, nonblocking, buf[pos], expected, j, k);
                errs++;
            }
        }
    }

    free(buf);
    free(displs);
    free(counts);
    return errs;
}

/* alltoallw with byte displacements in reverse order, and a gap of 8 bytes after each block.
 * The block for a pair is 2*m ints: MPI_INT with 2*m elements, or a 2-int contiguous type with
 * m elements, chosen by the parity of i+j. */
static int test_alltoallw(MPI_Comm comm, bool nonblocking)
{
    int errs = 0;
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    const int stride = 32;      /* bytes; the largest block is 24 bytes */
    MPI_Datatype pair;
    MPI_Type_contiguous(2, MPI_INT, &pair);
    MPI_Type_commit(&pair);

    MPI_Datatype *types = malloc(size * sizeof(MPI_Datatype));
    int *counts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));
    char *buf = malloc(size * stride);

    for (int j = 0; j < size; j++) {
        int m = nelem(j, rank);
        if ((j + rank) % 2) {
            types[j] = MPI_INT;
            counts[j] = 2 * m;
        } else {
            types[j] = pair;
            counts[j] = m;
        }
        displs[j] = (size - 1 - j) * stride;
    }

    memset(buf, GAP_BYTE, (size_t) size * stride);
    for (int j = 0; j < size; j++) {
        int *p = (int *) (buf + displs[j]);
        for (int k = 0; k < 2 * nelem(j, rank); k++) {
            p[k] = value(0, rank, j, k);
        }
    }

    if (nonblocking) {
        MPI_Request req;
        MPI_Ialltoallw(MPI_IN_PLACE, NULL, NULL, NULL, buf, counts, displs, types, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        MPI_Alltoallw(MPI_IN_PLACE, NULL, NULL, NULL, buf, counts, displs, types, comm);
    }

    for (int j = 0; j < size; j++) {
        int n = 2 * nelem(j, rank);
        int *p = (int *) (buf + displs[j]);
        for (int k = 0; k < n; k++) {
            if (p[k] != value(0, j, rank, k)) {
                fprintf(stderr, "[%d] alltoallw nb=%d: got %d expected %d from %d, element %d\n",
                        rank, nonblocking, p[k], value(0, j, rank, k), j, k);
                errs++;
            }
        }
        for (int b = 4 * n; b < stride; b++) {
            if (buf[displs[j] + b] != GAP_BYTE) {
                fprintf(stderr, "[%d] alltoallw nb=%d: gap after block %d changed\n",
                        rank, nonblocking, j);
                errs++;
                break;
            }
        }
    }

    MPI_Type_free(&pair);
    free(buf);
    free(displs);
    free(counts);
    free(types);
    return errs;
}
