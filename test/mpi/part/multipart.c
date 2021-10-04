/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* Checks message matching with multiple partitioned calls.
 *
 * It checks three types of matching scenarios:
 * - same comm same tag: matches in program order of initialization calls
 * - same comm different tags: matches based on tags
 * - different comms: matches based on comms */

#define PARTITIONS 8
#define COUNT 64
#define TOT_COUNT (PARTITIONS * COUNT)

/* Internal variables */
static int bufs[3][TOT_COUNT];
static int rank, size;

/* Internal utility routines */
static void fill_data(int seed, int *buf)
{
    for (int i = 0; i < TOT_COUNT; i++)
        buf[i] = seed;
}

static int check_recv_buf(int seed, int *buf)
{
    int errs = 0;
    for (int i = 0; i < TOT_COUNT; i++) {
        if (buf[i] != seed) {
            if (errs < 10) {
                fprintf(stderr, "expected %d but received %d at buf[%d]\n", seed, buf[i], i);
                fflush(stderr);
            }
            errs++;
        }
    }
    return errs;
}

static int check_recv_stat(int source, int tag, MPI_Status stat)
{
    int errs = 0;

    if (stat.MPI_SOURCE != source) {
        fprintf(stderr, "expected status.MPI_SOURCE %d, but received %d\n",
                source, stat.MPI_SOURCE);
        fflush(stderr);
        errs++;
    }
    if (stat.MPI_TAG != tag) {
        fprintf(stderr, "expected status.MPI_TAG %d, but received %d\n", tag, stat.MPI_TAG);
        fflush(stderr);
        errs++;
    }
    if (stat.MPI_ERROR != MPI_SUCCESS) {
        fprintf(stderr, "expected status.MPI_ERROR %d, but received %d\n",
                MPI_SUCCESS, stat.MPI_ERROR);
        fflush(stderr);
        errs++;
    }
    return errs;
}

static void start_sends(MPI_Request * reqs, MPI_Status * stats)
{
    MPI_Start(&reqs[0]);
    fill_data(1, bufs[0]);
    MPI_Pready_range(0, PARTITIONS - 1, reqs[0]);

    MPI_Start(&reqs[1]);
    fill_data(2, bufs[1]);
    MPI_Pready_range(0, PARTITIONS - 1, reqs[1]);

    MPI_Start(&reqs[2]);
    fill_data(3, bufs[2]);
    MPI_Pready_range(0, PARTITIONS - 1, reqs[2]);

    MPI_Waitall(3, reqs, stats);
}

static void start_recvs(MPI_Request * reqs, MPI_Status * stats)
{
    fill_data(-1, bufs[0]);
    MPI_Start(&reqs[0]);

    fill_data(-1, bufs[1]);
    MPI_Start(&reqs[1]);

    fill_data(-1, bufs[2]);
    MPI_Start(&reqs[2]);
    MPI_Waitall(3, reqs, stats);
}

static int check_recv_results(MPI_Status * stats, int *exp_sources, int *exp_tags)
{
    int errs = 0;

    errs += check_recv_buf(1, bufs[0]);
    errs += check_recv_buf(2, bufs[1]);
    errs += check_recv_buf(3, bufs[2]);

    errs += check_recv_stat(exp_sources[0], exp_tags[0], stats[0]);
    errs += check_recv_stat(exp_sources[1], exp_tags[1], stats[1]);
    errs += check_recv_stat(exp_sources[2], exp_tags[2], stats[2]);

    return errs;
}

/* Test routines for different scenarios */
static int test_diff_comm(void)
{
    int errs = 0;
    MPI_Request reqs[3];
    MPI_Status stats[3];

    int source = 0, dest = size - 1;

    MPI_Comm comms[3];
    for (int i = 0; i < 3; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[i]);
    }

    if (rank == source) {
        MTestPrintfMsg(1, "Testing partitioned message matching with different comm same tag\n");

        /* Perform initialization with different tags.
         * Expect message should match following tags. */
        MPI_Psend_init(bufs[2], PARTITIONS, COUNT, MPI_INT, dest, 0,
                       comms[2], MPI_INFO_NULL, &reqs[2]);
        MPI_Psend_init(bufs[1], PARTITIONS, COUNT, MPI_INT, dest, 0,
                       comms[1], MPI_INFO_NULL, &reqs[1]);
        MPI_Psend_init(bufs[0], PARTITIONS, COUNT, MPI_INT, dest, 0,
                       comms[0], MPI_INFO_NULL, &reqs[0]);

        start_sends(reqs, stats);       /* start sends in the order of 0,1,2 */

    } else if (rank == dest) {
        MPI_Precv_init(bufs[0], PARTITIONS, COUNT, MPI_INT, source, 0,
                       comms[0], MPI_INFO_NULL, &reqs[0]);
        MPI_Precv_init(bufs[1], PARTITIONS, COUNT, MPI_INT, source, 0,
                       comms[1], MPI_INFO_NULL, &reqs[1]);
        MPI_Precv_init(bufs[2], PARTITIONS, COUNT, MPI_INT, source, 0,
                       comms[2], MPI_INFO_NULL, &reqs[2]);

        start_recvs(reqs, stats);       /* start receives in the order of 0,1,2 */

        int exp_sources[3] = { source, source, source };
        int exp_tags[3] = { 0, 0, 0 };
        errs = check_recv_results(stats, exp_sources, exp_tags);
    }

    if (rank == source || rank == dest) {
        for (int i = 0; i < 3; i++) {
            MPI_Request_free(&reqs[i]);
        }
    }

    for (int i = 0; i < 3; i++) {
        MPI_Comm_free(&comms[i]);
    }

    return errs;
}

static int test_same_comm_diff_tag(void)
{
    int errs = 0;
    MPI_Request reqs[3];
    MPI_Status stats[3];

    int source = 0, dest = size - 1;

    if (rank == source) {
        MTestPrintfMsg(1, "Testing partitioned message matching with same comm different tag\n");

        /* Perform initialization with different tags.
         * Expect message should match following tags. */
        MPI_Psend_init(bufs[2], PARTITIONS, COUNT, MPI_INT, dest, 2,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[2]);
        MPI_Psend_init(bufs[1], PARTITIONS, COUNT, MPI_INT, dest, 1,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[1]);
        MPI_Psend_init(bufs[0], PARTITIONS, COUNT, MPI_INT, dest, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[0]);

        start_sends(reqs, stats);       /* start sends in the order of 0,1,2 */

    } else if (rank == dest) {
        MPI_Precv_init(bufs[0], PARTITIONS, COUNT, MPI_INT, source, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[0]);
        MPI_Precv_init(bufs[1], PARTITIONS, COUNT, MPI_INT, source, 1,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[1]);
        MPI_Precv_init(bufs[2], PARTITIONS, COUNT, MPI_INT, source, 2,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[2]);

        start_recvs(reqs, stats);       /* start receives in the order of 0,1,2 */

        int exp_sources[3] = { source, source, source };
        int exp_tags[3] = { 0, 1, 2 };
        errs = check_recv_results(stats, exp_sources, exp_tags);
    }

    if (rank == source || rank == dest) {
        for (int i = 0; i < 3; i++) {
            MPI_Request_free(&reqs[i]);
        }
    }

    return errs;
}

static int test_same_comm_same_tag(void)
{
    int errs = 0;
    MPI_Request reqs[3];
    MPI_Status stats[3];

    int source = 0, dest = size - 1;

    if (rank == source) {
        MTestPrintfMsg(1, "Testing partitioned message matching with same comm same tag\n");

        /* Perform initialization and start/pready in different order.
         * Expect message should match following the order of initialization calls. */
        MPI_Psend_init(bufs[0], PARTITIONS, COUNT, MPI_INT, dest, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[0]);
        MPI_Psend_init(bufs[1], PARTITIONS, COUNT, MPI_INT, dest, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[1]);
        MPI_Psend_init(bufs[2], PARTITIONS, COUNT, MPI_INT, dest, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[2]);

        MPI_Start(&reqs[2]);
        fill_data(3, bufs[2]);
        MPI_Pready_range(0, PARTITIONS - 1, reqs[2]);

        MPI_Start(&reqs[0]);
        fill_data(1, bufs[0]);
        MPI_Pready_range(0, PARTITIONS - 1, reqs[0]);

        MPI_Start(&reqs[1]);
        fill_data(2, bufs[1]);
        MPI_Pready_range(0, PARTITIONS - 1, reqs[1]);

        MPI_Waitall(3, reqs, stats);

    } else if (rank == dest) {
        MPI_Precv_init(bufs[0], PARTITIONS, COUNT, MPI_INT, source, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[0]);
        MPI_Precv_init(bufs[1], PARTITIONS, COUNT, MPI_INT, source, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[1]);
        MPI_Precv_init(bufs[2], PARTITIONS, COUNT, MPI_INT, source, 0,
                       MPI_COMM_WORLD, MPI_INFO_NULL, &reqs[2]);

        start_recvs(reqs, stats);       /* start receives in the order of 0,1,2 */

        int exp_sources[3] = { source, source, source };
        int exp_tags[3] = { 0, 0, 0 };
        errs = check_recv_results(stats, exp_sources, exp_tags);
    }

    if (rank == source || rank == dest) {
        for (int i = 0; i < 3; i++) {
            MPI_Request_free(&reqs[i]);
        }
    }

    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    errs += test_same_comm_same_tag();

    errs += test_same_comm_diff_tag();

    errs += test_diff_comm();

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
