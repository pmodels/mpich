/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>
#include "rma_test_op.h"

#ifdef MULTI_TESTS
#define run rma_overlap_wins_rma
int run(const char *arg);
#endif

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
#define DATATYPE int
#else
#define DATATYPE double
#endif

static MPI_Datatype mpi_type = MPI_DATATYPE_NULL;
static int type_size = 0;

static void *local_buf, *result_buf, *compare_buf;
static int exp_target_val = 0;

static const int verbose = 0;

static int rank = -1, nprocs = 0;
static int norigins, target;
static MPI_Win *wins;
static int win_size = 0, win_cnt = 0;

static void *winbuf = NULL, *my_base = NULL;

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

/* Issue functions for different RMA operations */
static inline void issue_rma_op(void *origin_addr, void *result_addr /* NULL */ ,
                                void *compare_addr /* NULL */ , int dst, MPI_Aint target_disp,
                                MPI_Win win)
{
    switch (test_op) {
        case TEST_OP_PUT:
            MPI_Put(origin_addr, 1, mpi_type, dst, target_disp, 1, mpi_type, win);
            break;
        case TEST_OP_ACC:
            MPI_Accumulate(origin_addr, 1, mpi_type, dst, target_disp, 1, mpi_type, MPI_SUM, win);
            break;
        case TEST_OP_GACC:
            MPI_Get_accumulate(origin_addr, 1, mpi_type, result_addr, 1, mpi_type, dst, target_disp,
                               1, mpi_type, MPI_SUM, win);
            break;
        case TEST_OP_FOP:
            MPI_Fetch_and_op(origin_addr, result_addr, mpi_type, dst, target_disp, MPI_SUM, win);
            break;
        case TEST_OP_CAS:
            MPI_Compare_and_swap(origin_addr, compare_addr, result_addr, mpi_type, dst, target_disp,
                                 win);
            break;
    };
}

static void set_value(void *p, int value)
{
    if (mpi_type == MPI_INT) {
        *(int *) p = (int) value;
    } else if (mpi_type == MPI_DOUBLE) {
        *(double *) p = (double) value;
    }
}

static inline void set_iteration_data(int x)
{
    int i;

    if (test_op == TEST_OP_CAS) {
        memcpy(compare_buf, local_buf, BUF_CNT * type_size);
    }

    for (i = 0; i < BUF_CNT; i++) {
        int val = rank + i + x;
        set_value((char *) local_buf + i * type_size, val);

        if (test_op == TEST_OP_CAS || test_op == TEST_OP_PUT) {
            exp_target_val = val;       /* swap */
        } else {
            exp_target_val += val;      /* sum */
        }
    }
}

static void print_value(void *p)
{
    if (mpi_type == MPI_INT) {
        printf("%d ", *(int *) p);
    } else if (mpi_type == MPI_DOUBLE) {
        printf("%lf ", *(double *) p);
    }
}

static void print_origin_data(void)
{
    int i;

    printf("[%d] local_buf: ", rank);
    for (i = 0; i < BUF_CNT; i++) {
        print_value((char *) local_buf + i * type_size);
    }
    printf("\n");

    printf("[%d] result_buf: ", rank);
    for (i = 0; i < BUF_CNT; i++) {
        print_value((char *) result_buf + i * type_size);
    }
    printf("\n");
}

static void print_target_data(void)
{
    int i;
    printf("[%d] winbuf: ", rank);
    for (i = 0; i < win_cnt; i++) {
        print_value((char *) winbuf + i * type_size);
    }
    printf("\n");
    fflush(stdout);
}

static int run_test(void)
{
    int errors = 0;
    int i, x;
    int dst = 0, target_disp = 0;
    MPI_Win win = MPI_WIN_NULL;

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
    local_buf = calloc(BUF_CNT, type_size);
    result_buf = calloc(BUF_CNT, type_size);
    compare_buf = calloc(BUF_CNT, type_size);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank != target) {

        /* 3. Origins issue RMA to target over its working window */
        MPI_Win_lock(MPI_LOCK_SHARED, dst, 0, win);
        verbose_print("[%d] RMA start, test %s (dst=%d, target_disp=%d, win 0x%x) - flush\n",
                      rank, get_rma_name(), dst, target_disp, win);

        for (x = 0; x < ITER; x++) {
            /* update local buffers and expected value in every iteration */
            set_iteration_data(x);

            char *p_local = local_buf;
            char *p_result = result_buf;
            char *p_compare = compare_buf;
            for (i = 0; i < BUF_CNT; i++) {
                issue_rma_op(p_local, p_result, p_compare, dst, target_disp, win);
                p_local += type_size;
                p_result += type_size;
                p_compare += type_size;
            }
            MPI_Win_flush(dst, win);

            if (verbose)
                print_origin_data();
        }

        /* 4. Check correctness of final target value */
        int int_target_val = 0;
        if (mpi_type == MPI_INT) {
            MPI_Get(&int_target_val, 1, mpi_type, dst, target_disp, 1, mpi_type, win);
            MPI_Win_flush(dst, win);
        } else if (mpi_type == MPI_DOUBLE) {
            double target_val;
            MPI_Get(&target_val, 1, mpi_type, dst, target_disp, 1, mpi_type, win);
            MPI_Win_flush(dst, win);
            int_target_val = (int) target_val;
        }
        if (int_target_val != exp_target_val) {
            error_print("rank %d (iter %d) - check %s, got target_val = %d, expected %d\n",
                        rank, x, get_rma_name(), int_target_val, exp_target_val);
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

    free(local_buf);
    free(result_buf);
    free(compare_buf);
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

int run(const char *arg)
{
    int errors = 0;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    parse_test_op(head);
    MTestArgListDestroy(head);

    assert_test_op();
    if (test_op == TEST_OP_CAS) {
        mpi_type = MPI_INT;
        type_size = sizeof(int);
    } else {
        mpi_type = MPI_DOUBLE;
        type_size = sizeof(double);
    }

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

    return errors;
}
