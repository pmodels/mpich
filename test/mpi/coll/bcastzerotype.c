/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpitest.h>
#include <assert.h>

#ifdef MULTI_TESTS
#define run coll_bcastzerotype
int run(const char *arg);
#endif

/* Test broadcast behavior with zero-sized types.
 *
 * A signature is (count, type) with size count * type_size. Ranks may use
 * different (count, type) pairs as long as the signatures match. A count of 0
 * and a zero-sized type both give an empty signature, so they must match each
 * other across ranks: every rank has to take the same path.
 *
 * Each case runs on its own duplicated communicator, so messages left behind by
 * one case cannot match receives of another. After each case a zero-byte sync
 * between every pair of ranks runs, followed by an MPI_Iprobe that must find
 * nothing: a message a rank should not have received is then detected. A rank
 * stuck in a receive that never matches hangs, which the test harness timeout
 * reports. */

#define NELEM (10)
#define SYNC_TAG 1234

static MPI_Datatype make_zero_type(void)
{
    MPI_Datatype type = MPI_DATATYPE_NULL;
    int type_size;

    MPI_Type_contiguous(0, MPI_INT, &type);
    MPI_Type_commit(&type);
    MPI_Type_size(type, &type_size);
    assert(type_size == 0);
    return type;
}

static void check_no_leftovers(MPI_Comm comm, int wrank, int wsize)
{
    MPI_Request reqs[wsize];
    int i, flag;
    MPI_Status status;

    /* zero-byte sync with every other rank. Messages from one source arrive in
     * order, so after the sync from rank p, every message p sent earlier on
     * this communicator has arrived. */
    int nreq = 0;
    for (i = 0; i < wsize; i++) {
        if (i != wrank) {
            MPI_Isend(NULL, 0, MPI_BYTE, i, SYNC_TAG, comm, &reqs[nreq++]);
        }
    }
    for (i = 0; i < wsize; i++) {
        if (i != wrank) {
            MPI_Recv(NULL, 0, MPI_BYTE, i, SYNC_TAG, comm, MPI_STATUS_IGNORE);
        }
    }
    MPI_Waitall(nreq, reqs, MPI_STATUSES_IGNORE);

    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, &status);
    if (flag) {
        fprintf(stderr, "rank %d: leftover message from rank %d, tag %d\n",
                wrank, status.MPI_SOURCE, status.MPI_TAG);
    }
    assert(!flag);
}

/* Each rank passes (root_count, root_type) if it is the root, otherwise
 * (other_count, other_type). Blocking or nonblocking. */
static void bcast_case(int root_count, MPI_Datatype root_type,
                       int other_count, MPI_Datatype other_type,
                       int nonblocking, int wrank, int wsize)
{
    int buf[NELEM];
    int i;
    int count;
    MPI_Datatype type;
    MPI_Request req;
    MPI_Comm comm;

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    for (i = 0; i < NELEM; i++) {
        buf[i] = wrank * NELEM + i;
    }
    if (wrank == 0) {
        count = root_count;
        type = root_type;
    } else {
        count = other_count;
        type = other_type;
    }

    if (nonblocking) {
        MPI_Ibcast(buf, count, type, 0, comm, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        MPI_Bcast(buf, count, type, 0, comm);
    }

    /* no data moves with an empty signature: the buffer is unchanged */
    for (i = 0; i < NELEM; i++) {
        assert(buf[i] == wrank * NELEM + i);
    }

    check_no_leftovers(comm, wrank, wsize);
    MPI_Comm_free(&comm);
}

int run(const char *arg)
{
    MPI_Datatype zero_type;
    int wrank, wsize;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    zero_type = make_zero_type();

    /* uniform: every rank has (NELEM, zero-size type) */
    bcast_case(NELEM, zero_type, NELEM, zero_type, 0, wrank, wsize);
    bcast_case(NELEM, zero_type, NELEM, zero_type, 1, wrank, wsize);

    /* mixed: root (0, MPI_INT), others (NELEM, zero-size type) */
    bcast_case(0, MPI_INT, NELEM, zero_type, 0, wrank, wsize);
    bcast_case(0, MPI_INT, NELEM, zero_type, 1, wrank, wsize);

    /* mixed, reversed: root (NELEM, zero-size type), others (0, MPI_INT) */
    bcast_case(NELEM, zero_type, 0, MPI_INT, 0, wrank, wsize);
    bcast_case(NELEM, zero_type, 0, MPI_INT, 1, wrank, wsize);

    MPI_Type_free(&zero_type);

    return 0;
}
