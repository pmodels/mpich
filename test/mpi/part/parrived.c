/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* Demonstrate receive-side partial completion notification.
 * - The receive-side uses parrived to check partition completion and check results for it.
 * - Send-side uses a derived datatype and the receive-side uses a basic datatype.
 *
 * How to use the test:
 * - Separate tests are generated for wait|test.
 * - The send/receive partitions, total count of basic elements, and iteration of
 *   start-test|wait periods are set by input parameters.
 *
 * Extended from Example 4.4 in the MPI 4.0 standard.*/

/* Input parameters */
static int spart = 0, rpart = 0, iteration = 0, stride = 2;
static MPI_Count tot_count = 0;
/* Internal variables */
static double *buf = NULL;
static MPI_Count scount = 0, rcount = 0;

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

static int check_recv_partition(int rp, int iter)
{
    int errs = 0;
    for (int i = 0; i < rcount; i++) {
        int index = rp * rcount + i;
#ifdef TEST_STRIDED
        double exp_val = (i % stride) ? 0.0 : VAL(index, iter);
#else
        double exp_val = VAL(index, iter);
#endif
        if (buf[index] != exp_val) {
            if (errs < 10) {
                fprintf(stderr,
                        "expected %.1f but received %.1f at buf[%d] (partition %d count %lld off %d), iteration %d\n",
                        exp_val, buf[index], index, rp, (long long) rcount, i, iter);
                fflush(stderr);
            }
            errs++;
        }
    }
    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0, flag;
    int rank, size;
    MPI_Request req = MPI_REQUEST_NULL;
    MPI_Datatype send_type;
#ifdef TEST_STRIDED
    MPI_Datatype recv_type;
#endif

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
    iteration = MTestArgListGetInt_with_default(head, "iteration", 5);
    stride = MTestArgListGetInt_with_default(head, "stride", 2);
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
#ifdef TEST_STRIDED
    if (scount % stride != 0 || rcount % stride != 0) {
        fprintf(stderr, "the send and recv counts must be even");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* select one double out of 2 */
    MPI_Datatype tmp_send, tmp_recv;
    MPI_Type_vector(scount / stride, 1, stride, MPI_DOUBLE, &tmp_send);
    MPI_Type_vector(rcount / stride, 1, stride, MPI_DOUBLE, &tmp_recv);

    /* need to resize the datatype to take the last empty double into account */
    const int send_extent = scount * sizeof(double);
    const int recv_extent = rcount * sizeof(double);
    MPI_Type_create_resized(tmp_send, 0, send_extent, &send_type);
    MPI_Type_create_resized(tmp_recv, 0, recv_extent, &recv_type);

    /* commit all that */
    MPI_Type_commit(&send_type);
    MPI_Type_commit(&recv_type);
    MPI_Type_free(&tmp_send);
    MPI_Type_free(&tmp_recv);
#else
    MPI_Type_contiguous(scount, MPI_DOUBLE, &send_type);
    MPI_Type_commit(&send_type);
#endif

    if (rank == 0) {
        MPI_Psend_init(buf, spart, 1, send_type, 1, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);

        for (int iter = 0; iter < iteration; iter++) {
            reset_buf();
            MPI_Start(&req);

#ifdef TEST_PREADY_OUT_OF_ORDER
            /* pready from last partition to first  */
            for (int i = spart - 1; i >= 0; i--) {
                fill_send_partition(i, iter);

                MTestPrintfMsg(1, "Rank %d sends partition %d with count %ld of basic elements\n",
                               rank, i, scount);
                MPI_Pready(i, req);
            }
#else /* default pready from first partition to last */
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
            do {
                MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
            } while (!flag);
#endif
        }

        MPI_Request_free(&req);
    } else if (rank == 1) {
#ifdef TEST_STRIDED
        MPI_Precv_init(buf, rpart, 1, recv_type, 0, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);
#else
        MPI_Precv_init(buf, rpart, rcount, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);
#endif

        for (int iter = 0; iter < iteration; iter++) {
            reset_buf();
            MPI_Start(&req);

            for (int i = 0; i < rpart; i += 2) {
                /* mimic receive-side computation that handles two partitions differently. */
                int part1_completed = 0, part2_completed = 0;
                while (part1_completed == 0 || part2_completed == 0) {

                    MPI_Parrived(req, i, &flag);
                    if (flag && part1_completed == 0) {
                        part1_completed = 1;
                        MTestPrintfMsg(1, "Rank %d Received partition %d "
                                       "with count %ld of basic elements\n", rank, i, rcount);

                        errs += check_recv_partition(i, iter);
                    }

                    if (i + 1 < rpart) {
                        MPI_Parrived(req, i + 1, &flag);
                        if (flag && part2_completed == 0) {
                            part2_completed = 1;
                            MTestPrintfMsg(1, "Rank %d Received partition %d "
                                           "with count %ld of basic elements\n", rank, i + 1,
                                           rcount);

                            errs += check_recv_partition(i + 1, iter);
                        }
                    } else {
                        part2_completed = 1;
                    }
                }
            }

#ifdef TEST_WAIT
            MPI_Wait(&req, MPI_STATUS_IGNORE);
#else
            MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
            if (!flag) {
                fprintf(stderr,
                        "all partitions are arrived on receiver but request is not complete, iteration %d\n",
                        iter);
                fflush(stderr);
                errs++;
            }
#endif
        }

        MPI_Request_free(&req);
    }

    MPI_Type_free(&send_type);
#ifdef TEST_STRIDED
    MPI_Type_free(&recv_type);
#endif
    free(buf);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
