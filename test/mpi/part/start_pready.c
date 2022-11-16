/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* Demonstrate send-side partitioned data transfer.
 * - The send-side calls pready for partial partitions; the receive-side wait for entire message.
 * - Both sides use basic datatype.
 *
 * How to use the test:
 * - Separate tests are generated for wait|test, and for pready|pready_range|pready_list.
 * - The send/receive partitions, total count of basic elements, range of partitions
 *   processed by each pready_range|pready_list, and iteration of start-test|wait periods
 *   are set by input parameters.
 *
 * Extended from Example 4.1 in the MPI 4.0 standard.*/

/* Input parameters */
static int spart = 0, rpart = 0, iteration = 0, range = 0;
static MPI_Count tot_count = 0;
/* Internal variables */
static double *buf = NULL;
static MPI_Count scount = 0, rcount = 0;
static int rank, size;

/* Utility functions */
#define VAL(_index, _iter) (double) ((_index) + (_iter))

static void fill_send_partition(int sp, int iter)
{
    for (int i = 0; i < scount; i++) {
        int index = sp * scount + i;
        buf[index] = VAL(index, iter);
    }
}

static void reset_buf(void)
{
    memset(buf, 0, tot_count * sizeof(double));
}

static int check_recv_buf(int iter)
{
    int errs = 0;
    for (int i = 0; i < rpart; i++) {
        for (int j = 0; j < rcount; j++) {
            int index = i * rcount + j;
            double exp_val = VAL(index, iter);
            if (buf[index] != exp_val) {
                if (errs < 10) {
                    fprintf(stderr,
                            "expected %.1f but received %.1f at buf[%d] (partition %d count %d), iteration %d\n",
                            exp_val, buf[index], index, i, j, iter);
                    fflush(stderr);
                }
                errs++;
            }
        }
    }
    return errs;
}

/* Test subroutines */
#ifdef TEST_PREADY_RANGE
static void test_pready_range(int iter, MPI_Request req)
{
    for (int lo = 0; lo < spart; lo += range) {
        /* check edge value for high */
        int high = lo + range - 1 < spart ? lo + range - 1 : spart - 1;

        for (int i = lo; i <= high; i++) {
            fill_send_partition(i, iter);
        }

        MTestPrintfMsg(1, "Rank %d sends partition range %d:%d "
                       "with count %ld of basic elements\n", rank, lo, high, scount);
        MPI_Pready_range(lo, high, req);
    }
}
#endif

#ifdef TEST_PREADY_LIST
static void test_pready_list(int iter, MPI_Request req)
{
    int *arrayp = calloc(range, sizeof(int));

    for (int lo = 0; lo < spart; lo += range) {
        /* check edge value for high */
        int high = lo + range - 1 < spart ? lo + range - 1 : spart - 1;
        int np = 0;

        /* reset in case last array contains less partitions */
        memset(arrayp, 0, range * sizeof(int));
        for (int i = lo; i <= high; i++) {
            fill_send_partition(i, iter);
            arrayp[np++] = i;
        }

        MTestPrintfMsg(1, "Rank %d sends partition range %d:%d "
                       "with count %ld of basic elements\n", rank, lo, high, scount);
        MPI_Pready_list(np, arrayp, req);
    }

    free(arrayp);
}
#endif

#ifdef TEST_PREADY_LIST_NON_CONSECUTIVE
static void test_pready_list_non_consecutive(int iter, MPI_Request req)
{
    int *arrayp = calloc(range, sizeof(int));

    for (int lo = 0; lo < spart; lo += range) {
        /* check edge value for high */
        int high = lo + range - 1 < spart ? lo + range - 1 : spart - 1;
        int np_even = 0, np_odd = 0;

        /* reset in case last array contains less spart */
        memset(arrayp, 0, range * sizeof(int));

        /* issue even elements in current range */
        MTestPrintfMsg(1, "Rank %d sends even partitions [", rank, lo, high, scount);
        for (int i = lo; i <= high; i += 2) {
            fill_send_partition(i, iter);
            arrayp[np_even++] = i;
            MTestPrintfMsg(1, "%d ", i);
        }
        MTestPrintfMsg(1, "] with count %ld of basic elements\n", scount);
        MPI_Pready_list(np_even, arrayp, req);

        /* issue odd elements in current range */
        MTestPrintfMsg(1, "Rank %d sends odd partitions [", rank, lo, high, scount);
        for (int i = lo + 1; i <= high; i += 2) {
            fill_send_partition(i, iter);
            arrayp[np_odd++] = i;
            MTestPrintfMsg(1, "%d ", i);
        }
        MTestPrintfMsg(1, "] with count %ld of basic elements\n", scount);
        MPI_Pready_list(np_odd, arrayp, req);
    }
    free(arrayp);
}
#endif

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Request req = MPI_REQUEST_NULL;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr, "This test requires at least two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Setup input parameters and internal variables */
    MTestArgList *head = MTestArgListCreate(argc, argv);
    spart = MTestArgListGetInt_with_default(head, "spart", 8);
    rpart = MTestArgListGetInt_with_default(head, "rpart", 8);
    tot_count = MTestArgListGetLong_with_default(head, "tot_count", 64);
    range = MTestArgListGetInt_with_default(head, "range", 2);  /* valid only for pready_list|range */
    iteration = MTestArgListGetInt_with_default(head, "iteration", 5);
    MTestArgListDestroy(head);

    /* Send buffer size and receive buffer size must be identical */
    scount = tot_count / spart;
    rcount = tot_count / rpart;
    if (spart * scount != rpart * rcount) {
        fprintf(stderr, "Invalid partitions (%d, %d) or tot_count (%lld),"
                "(tot_count / spart * spart) and (tot_count / rpart * rpart) "
                "must be identical\n", spart, rpart, (long long) tot_count);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    buf = calloc(tot_count, sizeof(double));

    if (rank == 0) {
        MPI_Psend_init(buf, spart, scount, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);

        for (int iter = 0; iter < iteration; iter++) {
            reset_buf();
            MPI_Start(&req);

#ifdef TEST_PREADY_RANGE
            test_pready_range(iter, req);
#elif defined(TEST_PREADY_LIST)
            test_pready_list(iter, req);
#elif defined(TEST_PREADY_LIST_NON_CONSECUTIVE)
            test_pready_list_non_consecutive(iter, req);
#else /* default test pready */
            for (int i = 0; i < spart; i++) {
                fill_send_partition(i, iter);

                MTestPrintfMsg(1, "Rank %d sends partition %d with count %ld of basic elements\n",
                               rank, i, scount);
                MPI_Pready(i, req);
            }
#endif

#ifdef TEST_WAIT
            MPI_Wait(&req, MPI_STATUS_IGNORE);
#else
            int flag = 0;
            do {
                MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
            } while (!flag);
#endif
        }
        MPI_Request_free(&req);
    } else if (rank == 1) {
        MPI_Precv_init(buf, rpart, rcount, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);

        for (int iter = 0; iter < iteration; iter++) {
            reset_buf();
            MPI_Start(&req);

#ifdef TEST_WAIT
            MPI_Wait(&req, MPI_STATUS_IGNORE);
#else
            int flag = 0;
            do {
                MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
            } while (!flag);
#endif
            MTestPrintfMsg(1, "Rank %d received entire message with partitions %d "
                           "and count %ld of basic elements\n", rank, rpart, rcount);

            errs += check_recv_buf(iter);
        }
        MPI_Request_free(&req);
    }

    free(buf);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
