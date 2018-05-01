/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "mpitest.h"

/* This test checks the remote completion of flush with RMA write-like operations
 * (PUT, ACC, GET_ACC, FOP, CAS) concurrently issued from different origin processes
 * to the same target over overlapping windows (i.e., two windows exposing the same
 * memory region)
 * 1. The first [nprocs-1] processes perform as origin, and the last process
 *    performs as target.
 * 2. Everyone allocates a buffer and creates [nprocs-1] windows over the same buffer.
 * 3. Every origin P[i] issues RMA operations and flush to target through
 *    wins[i] respectively, to different location winbuf[i] on target.
 * 4. Finally, every origin P[i] issues GET and flush to obtain winbuf[i] on
 *    target and checks the correctness. */

#define ITER 10
#define BUF_CNT 2

#if defined(TEST_CAS)
#define MPI_DATATYPE MPI_INT
#define DATATYPE int
#define DATATYPE_FORMAT "%d"
#else
#define MPI_DATATYPE MPI_DOUBLE
#define DATATYPE double
#define DATATYPE_FORMAT "%.1f"
#endif

DATATYPE local_buf[BUF_CNT], result_buf[BUF_CNT], compare_buf[BUF_CNT];
DATATYPE exp_target_val = 0.0;

const int verbose = 0;

int rank = -1, nprocs = 0;
int norigins, target;
MPI_Win *wins;
int win_size = 0, win_cnt = 0;

DATATYPE *winbuf = NULL, *my_base = NULL;

#define verbose_print(str,...) {                        \
            if (verbose) {                               \
                fprintf(stdout, str, ## __VA_ARGS__);   \
                fflush(stdout);                         \
            }                                           \
        }
#define error_print(str,...) {                        \
            fprintf(stderr, str, ## __VA_ARGS__);   \
            fflush(stderr);                         \
        }

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
static inline void issue_rma_op(DATATYPE * origin_addr, DATATYPE * result_addr /* NULL */ ,
                                DATATYPE * compare_addr /* NULL */ , int dst, MPI_Aint target_disp,
                                MPI_Win win)
{
    MPI_Put(origin_addr, 1, MPI_DATATYPE, dst, target_disp, 1, MPI_DATATYPE, win);
}
#elif defined(TEST_ACC)
static inline void issue_rma_op(DATATYPE * origin_addr, DATATYPE * result_addr /* NULL */ ,
                                DATATYPE * compare_addr /* NULL */ , int dst, MPI_Aint target_disp,
                                MPI_Win win)
{
    MPI_Accumulate(origin_addr, 1, MPI_DATATYPE, dst, target_disp, 1, MPI_DATATYPE, MPI_SUM, win);
}
#elif defined(TEST_GACC)
static inline void issue_rma_op(DATATYPE * origin_addr, DATATYPE * result_addr,
                                DATATYPE * compare_addr /* NULL */ , int dst, MPI_Aint target_disp,
                                MPI_Win win)
{
    MPI_Get_accumulate(origin_addr, 1, MPI_DATATYPE, result_addr, 1, MPI_DATATYPE, dst, target_disp,
                       1, MPI_DATATYPE, MPI_SUM, win);
}
#elif defined(TEST_FOP)
static inline void issue_rma_op(DATATYPE * origin_addr, DATATYPE * result_addr,
                                DATATYPE * compare_addr /* NULL */ , int dst, MPI_Aint target_disp,
                                MPI_Win win)
{
    MPI_Fetch_and_op(origin_addr, result_addr, MPI_DATATYPE, dst, target_disp, MPI_SUM, win);
}
#elif defined(TEST_CAS)
static inline void issue_rma_op(DATATYPE * origin_addr, DATATYPE * result_addr,
                                DATATYPE * compare_addr, int dst, MPI_Aint target_disp, MPI_Win win)
{
    MPI_Compare_and_swap(origin_addr, compare_addr, result_addr, MPI_DATATYPE, dst, target_disp,
                         win);
}
#else
#define issue_rma_op(loc_addr, result_addr, compare_addr, dst, target_disp, win)
#endif

static inline void set_iteration_data(int x)
{
    int i;

#if defined(TEST_CAS)
    for (i = 0; i < BUF_CNT; i++)
        compare_buf[i] = local_buf[i];  /* always equal, thus swap happens */
#endif

    for (i = 0; i < BUF_CNT; i++) {
        local_buf[i] = rank + i + x;

#if defined(TEST_CAS) || defined(TEST_PUT)
        exp_target_val = local_buf[i];  /* swap */
#else
        exp_target_val += local_buf[i]; /* sum */
#endif
    }
}

static void print_origin_data(void)
{
    int i;

    printf("[%d] local_buf: ", rank);
    for (i = 0; i < BUF_CNT; i++)
        printf(DATATYPE_FORMAT " ", local_buf[i]);
    printf("\n");

    printf("[%d] result_buf: ", rank);
    for (i = 0; i < BUF_CNT; i++)
        printf(DATATYPE_FORMAT " ", result_buf[i]);
    printf("\n");
}

static void print_target_data(void)
{
    int i;
    printf("[%d] winbuf: ", rank);
    for (i = 0; i < win_cnt; i++)
        printf(DATATYPE_FORMAT " ", winbuf[i]);
    printf("\n");
    fflush(stdout);
}

static int run_test()
{
    int errors = 0;
    int i, x;
    int dst = 0, target_disp = 0;
    MPI_Win win = MPI_WIN_NULL;
    DATATYPE target_val = 0.0;

    /* 1. Specify working window and displacement.
     *  - Target:  no RMA issued, always check results on wins[0].
     *  - Origins: issue RMA on different window and different memory location */
    if (rank == target) {
        win = wins[0];
    } else {
        win = wins[rank];
        target_disp = rank;
    }
    dst = target;

    /* 2. Every one resets local data */
    memset(local_buf, 0, sizeof(local_buf));
    memset(result_buf, 0, sizeof(result_buf));
    memset(compare_buf, 0, sizeof(compare_buf));

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank != target) {

        /* 3. Origins issue RMA to target over its working window */
        MPI_Win_lock(MPI_LOCK_SHARED, dst, 0, win);
        verbose_print("[%d] RMA start, test %s (dst=%d, target_disp=%d, win 0x%x) - flush\n",
                      rank, rma_name, dst, target_disp, win);

        for (x = 0; x < ITER; x++) {
            /* update local buffers and expected value in every iteration */
            set_iteration_data(x);

            for (i = 0; i < BUF_CNT; i++)
                issue_rma_op(&local_buf[i], &result_buf[i], &compare_buf[i], dst, target_disp, win);
            MPI_Win_flush(dst, win);

            if (verbose)
                print_origin_data();
        }

        /* 4. Check correctness of final target value */
        MPI_Get(&target_val, 1, MPI_DATATYPE, dst, target_disp, 1, MPI_DATATYPE, win);
        MPI_Win_flush(dst, win);
        if (target_val != exp_target_val) {
            error_print("rank %d (iter %d) - check %s, got target_val = "
                        DATATYPE_FORMAT ", expected " DATATYPE_FORMAT "\n", rank, x,
                        rma_name, target_val, exp_target_val);
            errors++;
        }

        MPI_Win_unlock(dst, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* 5. Every one prints window buffer */
    if (verbose && rank == target) {
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
        print_target_data();
        MPI_Win_unlock(rank, win);
    }

    return errors;
}

static void init_windows(void)
{
    int i = 0;

    /* Everyone creates norigins overlapping windows. */
    winbuf = malloc(win_size);
    memset(winbuf, 0, win_size);

    wins = malloc(norigins * sizeof(MPI_Win));
    for (i = 0; i < norigins; i++) {
        wins[i] = MPI_WIN_NULL;
        MPI_Win_create(winbuf, win_size, sizeof(DATATYPE), MPI_INFO_NULL, MPI_COMM_WORLD, &wins[i]);
    }
}

static void destroy_windows(void)
{
    int i = 0;
    for (i = 0; i < norigins; i++) {
        if (wins[i] != MPI_WIN_NULL)
            MPI_Win_free(&wins[i]);
    }
    free(wins);
    free(winbuf);
}

int main(int argc, char *argv[])
{
    int errors = 0, all_errors = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs < 3) {
        if (rank == 0) {
            error_print("Error: must use at least 3 processes\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* The last rank performs as target, all others are origin.
     * Every origin accesses to a different memory location on the target. */
    target = nprocs - 1;
    norigins = nprocs - 1;
    win_cnt = nprocs - 1;
    win_size = sizeof(DATATYPE) * win_cnt;

    if (rank == 0) {
        verbose_print("[%d] %d origins, target rank = %d\n", rank, norigins, target);
    }

    init_windows();
    MPI_Barrier(MPI_COMM_WORLD);

    /* start test */
    errors = run_test();

    MPI_Barrier(MPI_COMM_WORLD);

    destroy_windows();
    MTest_Finalize(errors);

    return MTestReturnValue(all_errors);
}
