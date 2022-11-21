/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#include "mpithreadtest.h"

/* Multithreaded version of test/mpi/part/parrived_wait.
 *
 * Each thread on the sender side prepares the partitions by using pready_range;
 * each thread on the receiver side receives the partitions by using parrived.
 *
 * How to use the test:.
 * - The send/receive partitions, total count of basic elements, and iteration of
 *   start-test|wait periods are set by input parameters.
 * - 4 threads are created on each side, can be changed by setting `NUM_THREADS`.
 *
 * Extended from Example 4.4 in the MPI 4.0 standard.*/

#define NUM_THREADS 4

/* Input parameters */
static int spart = 0, rpart = 0, iteration = 0;
static MPI_Count tot_count = 0;
/* Internal variables */
static double *buf = NULL;
static MPI_Count scount = 0, rcount = 0;
static int rank, size, thread_errs[NUM_THREADS];

typedef struct {
    int tid;
    int iter;
    MPI_Request req;
} thread_arg_t;
static thread_arg_t thread_args[NUM_THREADS];

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

static int check_recv_partition(int tid, int rp, int iter)
{
    int errs = 0;
    for (int i = 0; i < rcount; i++) {
        int index = rp * rcount + i;
        double exp_val = VAL(index, iter);
        if (buf[index] != exp_val) {
            if (errs < 10) {
                fprintf(stderr, "Rank %d tid %d expected %.1f but received %.1f "
                        "at buf[%d] (partition %d count %lld off %d), iteration %d\n",
                        rank, tid, exp_val, buf[index], index, rp, (long long) rcount, i, iter);
                fflush(stderr);
            }
            errs++;
        }
    }
    return errs;
}

/* Set partitions range for each thread by mimicking OpenMP static scheduling.*/
static void set_partitions_range(int partitions, int tid, int *lo_ptr, int *high_ptr)
{
    int partition_th = partitions / NUM_THREADS;        /* chunk size */
    int lo = tid * partition_th;
    int high = lo + partition_th - 1;

    if (high >= partitions)
        high = partitions - 1;  /* last chunk may be less than chunk size */
    if (tid == NUM_THREADS - 1)
        high += partitions % NUM_THREADS;       /* last thread need handle all remaining partitions */

    *lo_ptr = lo;
    *high_ptr = high;
}

/* Test subroutines */
static MTEST_THREAD_RETURN_TYPE run_send_test(void *arg)
{
    thread_arg_t *thread_arg = (thread_arg_t *) arg;
    int tid = thread_arg->tid;
    int iter = thread_arg->iter;
    MPI_Request req = thread_arg->req;

    int lo = 0, high = 0;
    set_partitions_range(spart, tid, &lo, &high);

    for (int i = lo; i <= high; i++)
        fill_send_partition(i, iter);
    MPI_Pready_range(lo, high, req);

    MTestPrintfMsg(1, "Rank %d tid %d sent partitions range %d:%d "
                   "with count %ld of basic elements\n", rank, tid, lo, high, scount);

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

static MTEST_THREAD_RETURN_TYPE run_recv_test(void *arg)
{
    thread_arg_t *thread_arg = (thread_arg_t *) arg;
    int tid = thread_arg->tid;
    int iter = thread_arg->iter;
    MPI_Request req = thread_arg->req;

    int lo = 0, high = 0;
    set_partitions_range(rpart, tid, &lo, &high);

    for (int i = lo; i <= high; i += 2) {
        /* mimic receive-side computation that handles two partitions differently. */
        int part1_completed = 0, part2_completed = 0;
        while (part1_completed == 0 || part2_completed == 0) {
            int flag = 0;
            MPI_Parrived(req, i, &flag);
            if (flag && part1_completed == 0) {
                part1_completed = 1;
                MTestPrintfMsg(1, "Rank %d tid %d received partition %d "
                               "with count %ld of basic elements\n", rank, tid, i, rcount);

                thread_errs[tid] += check_recv_partition(tid, i, iter);
            }

            if (i + 1 < rpart) {
                MPI_Parrived(req, i + 1, &flag);
                if (flag && part2_completed == 0) {
                    part2_completed = 1;
                    MTestPrintfMsg(1, "Rank %d tid %d received partition %d "
                                   "with count %ld of basic elements\n", rank, tid, i + 1, rcount);

                    thread_errs[tid] += check_recv_partition(tid, i + 1, iter);
                }
            } else {
                part2_completed = 1;
            }
        }
    }
    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int provided = -1;
    MPI_Request req = MPI_REQUEST_NULL;
    MPI_Datatype send_type;

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE is not supported\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

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
    MTestArgListDestroy(head);

    /* Send buffer size and receive buffer size must be identical */
    scount = tot_count / spart;
    rcount = tot_count / rpart;
    if (spart * scount != rpart * rcount) {
        fprintf(stderr, "Invalid partitions (%d, %d) or tot_count (%ld):\n"
                "(tot_count / spart * spart) and (tot_count / rpart * rpart) "
                "must be identical\n", spart, rpart, tot_count);
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    buf = calloc(tot_count, sizeof(double));
    MPI_Type_contiguous(scount, MPI_DOUBLE, &send_type);
    MPI_Type_commit(&send_type);

    if (rank == 0) {
        MPI_Psend_init(buf, spart, 1, send_type, 1, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);

        for (int iter = 0; iter < iteration; iter++) {
            reset_buf();
            MPI_Start(&req);

            /* Multiple threads prepare and send partitions in parallel */
            for (int tid = 0; tid < NUM_THREADS; tid++) {
                thread_args[tid].tid = tid;
                thread_args[tid].iter = iter;
                thread_args[tid].req = req;
                errs += MTest_Start_thread(run_send_test, &thread_args[tid]);
            }
            errs += MTest_Join_threads();

            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }

        MPI_Request_free(&req);
    } else if (rank == 1) {
        MPI_Precv_init(buf, rpart, rcount, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);

        for (int iter = 0; iter < iteration; iter++) {
            reset_buf();
            MPI_Start(&req);

            /* Multiple threads receive and process received partitions in parallel */
            for (int tid = 0; tid < NUM_THREADS; tid++) {
                thread_args[tid].tid = tid;
                thread_args[tid].iter = iter;
                thread_args[tid].req = req;
                errs += MTest_Start_thread(run_recv_test, &thread_args[tid]);
            }
            errs += MTest_Join_threads();

            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }

        MPI_Request_free(&req);
    }

    MPI_Type_free(&send_type);
    free(buf);

    for (int tid = 0; tid < NUM_THREADS; tid++) {
        errs += thread_errs[tid];
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
