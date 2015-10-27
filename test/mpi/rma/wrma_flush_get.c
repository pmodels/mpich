/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

#define ITER 10000
#define BUF_CNT 1
int local_buf[BUF_CNT], result_addr[BUF_CNT], check_addr[BUF_CNT];
#ifdef TEST_CAS
int compare_buf[BUF_CNT];
#endif

const int verbose = 0;

/* This test checks the remote completion of flush with RMA write-like operations
 * (PUT, ACC, GET_ACC, FOP, CAS), and confirms result by GET issued from another
 * process.
 * 1. Three processes create window by Win_allocate.
 * 2. P(origin) issues RMA operations and flush to P(target), then call
 *    send-recv to synchronize with P(checker).
 * 3. P(checker) then issues get to P(target) to check the results.
 */

int rank = -1, nproc = 0;
int origin = -1, target = -1, checker = -1;
MPI_Win win = MPI_WIN_NULL;
int *my_base = NULL;

/* Define operation name for error message */
#ifdef TEST_PUT
const char *rma_name = "Put";
#elif defined(TEST_ACC)
const char *rma_name = "Accumulate";
#elif defined(TEST_GACC)
const char *rma_name = "Get_accumulate";
#elif defined(TEST_FOP)
const char *rma_name = "Fetch_and_op";
#elif defined(TEST_CAS)
const char *rma_name = "Compare_and_swap";
#else
const char *rma_name = "None";
#endif

/* Issue functions for different RMA operations */
#ifdef TEST_PUT
static inline void issue_rma_op(int i)
{
    MPI_Put(&local_buf[i], 1, MPI_INT, target, i, 1, MPI_INT, win);
}
#elif defined(TEST_ACC)
static inline void issue_rma_op(int i)
{
    MPI_Accumulate(&local_buf[i], 1, MPI_INT, target, i, 1, MPI_INT, MPI_REPLACE, win);
}
#elif defined(TEST_GACC)
static inline void issue_rma_op(int i)
{
    MPI_Get_accumulate(&local_buf[i], 1, MPI_INT, &result_addr[i], 1, MPI_INT, target, i,
                       1, MPI_INT, MPI_REPLACE, win);
}
#elif defined(TEST_FOP)
static inline void issue_rma_op(int i)
{
    MPI_Fetch_and_op(&local_buf[i], &result_addr[i], MPI_INT, target, i, MPI_REPLACE, win);
}
#elif defined(TEST_CAS)
static inline void issue_rma_op(int i)
{
    compare_buf[i] = i;        /* always equal to window value, thus swap happens */
    MPI_Compare_and_swap(&local_buf[i], &compare_buf[i], &result_addr[i], MPI_INT, target, i, win);
}
#endif


/* Local check function for GET-like operations */
#if defined(TEST_GACC) || defined(TEST_FOP) || defined(TEST_CAS)

/* Check local result buffer for GET-like operations */
static int check_local_result(int iter)
{
    int i = 0;
    int errors = 0;

    for (i = 0; i < BUF_CNT; i++) {
        if (result_addr[i] != i) {
            printf("rank %d (iter %d) - check %s, got result_addr[%d] = %d, expected %d\n",
                   rank, iter, rma_name, i, result_addr[i], i);
            errors++;
        }
    }
    return errors;
}

#else
#define check_local_result(iter) (0)
#endif

static int run_test()
{
    int i = 0, x = 0;
    int errors = 0;
    int sbuf = 0, rbuf = 0;
    MPI_Status stat;

    for (x = 0; x < ITER; x++) {
        /* 1. Target resets window data */
        if (rank == target) {
            for (i = 0; i < BUF_CNT; i++)
                my_base[i] = i;
            MPI_Win_sync(win);  /* write is done on window */
        }

        MPI_Barrier(MPI_COMM_WORLD);

        /* 2. Every one resets local data */
        for (i = 0; i < BUF_CNT; i++) {
            local_buf[i] = BUF_CNT + x * BUF_CNT + i;
            result_addr[i] = 0;
        }

        /* 3. Origin issues RMA operation to target */
        if (rank == origin) {
            /* 3-1. Issue RMA. */
            for (i = 0; i < BUF_CNT; i++) {
                issue_rma_op(i);
            }
            MPI_Win_flush(target, win);

            /* 3-2. Sync with checker */
            MPI_Send(&sbuf, 1, MPI_INT, checker, 999, MPI_COMM_WORLD);

            /* 3-3. Check local result buffer */
            errors += check_local_result(x);
        }

        /* 4. Checker issues GET to target and checks result */
        if (rank == checker) {
            /* 4-1. Sync with origin */
            MPI_Recv(&rbuf, 1, MPI_INT, origin, 999, MPI_COMM_WORLD, &stat);

            /* 4-2. Get result and check */
            MPI_Get(check_addr, BUF_CNT, MPI_INT, target, 0, BUF_CNT, MPI_INT, win);
            MPI_Win_flush(target, win);

            for (i = 0; i < BUF_CNT; i++) {
                if (check_addr[i] != local_buf[i]) {
                    printf("rank %d (iter %d) - check %s, got check_addr[%d] = %d, expected %d\n",
                           rank, x, rma_name, i, check_addr[i], local_buf[i]);
                    errors++;
                }
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    return errors;
}

int main(int argc, char *argv[])
{
    int errors = 0, all_errors = 0;
    int win_size = sizeof(int) * BUF_CNT;
    int win_unit = sizeof(int);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc != 3) {
        if (rank == 0)
            printf("Error: must be run with three processes\n");
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

#if !defined(TEST_PUT) && !defined(TEST_ACC) && !defined(TEST_GACC) && !defined(TEST_FOP) && !defined(TEST_CAS)
    if (rank == 0)
        printf("Error: must specify operation type at compile time\n");
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Abort(MPI_COMM_WORLD, 1);
#endif

    /* Identify origin, target and checker ranks. */
    target = 0;
    checker = 2;
    origin = 1;

    MPI_Win_allocate(win_size, win_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &my_base, &win);

    /* Start checking. */
    MPI_Win_lock_all(0, win);

    errors = run_test();

    MPI_Win_unlock_all(win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    if (win != MPI_WIN_NULL)
        MPI_Win_free(&win);

    MPI_Finalize();

    return 0;
}
