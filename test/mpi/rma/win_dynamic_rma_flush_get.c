/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "mpitest.h"

/* This test checks RMA operations that access to dynamic window with multiple attached regions.
 * It generates separate tests for each RMA operation type accessing to a single remote region,
 * and for Put, Accumulate, Get_accumulate accessing to two remote regions within a single operation.
 * Each test uses Get to check results.*/
#define MPI_DATATYPE MPI_INT
#define DATATYPE int
#define DATATYPE_FORMAT "%d"

#define ITER 10
#define MEM_CNT 8192
#define MAX_RMA_CNT (MEM_CNT * 2)

static DATATYPE mem_region[MAX_RMA_CNT];
static MPI_Win dyn_win = MPI_WIN_NULL;
static MPI_Aint *mem_ptrs = NULL;
static int rank = -1, nprocs = 0;
static int rma_cnt = 4096;      /* only touch the first regions */

static DATATYPE local_buf[MAX_RMA_CNT], check_buf[MAX_RMA_CNT], result_buf[MAX_RMA_CNT],
    compare_buf[MAX_RMA_CNT];

#define error_print(str,...) {                        \
            fprintf(stderr, str, ## __VA_ARGS__);     \
            fflush(stderr);                           \
        }

enum rma_op {
    TEST_PUT,
    TEST_ACC,
    TEST_GACC,
    TEST_FOP,
    TEST_CAS,
    TEST_NUM_OP
};

static enum rma_op op = TEST_PUT;

/* Define operation name for error message */
const char *rma_names[TEST_NUM_OP] = {
    "Put",
    "Accumulate",
    "Get_accumulate",
    "Fetch_and_op",
    "Compare_and_swap"
};

/* Issue functions for different RMA operations */
static inline void issue_rma_op(int dst, MPI_Aint target_disp)
{
    switch (op) {
        case TEST_PUT:
            MPI_Put(local_buf, rma_cnt, MPI_DATATYPE, dst, target_disp, rma_cnt, MPI_DATATYPE,
                    dyn_win);
            break;
        case TEST_ACC:
            MPI_Accumulate(local_buf, rma_cnt, MPI_DATATYPE, dst, target_disp, rma_cnt,
                           MPI_DATATYPE, MPI_REPLACE, dyn_win);
            break;
        case TEST_GACC:
            MPI_Get_accumulate(local_buf, rma_cnt, MPI_DATATYPE, result_buf, rma_cnt, MPI_DATATYPE,
                               dst, target_disp, rma_cnt, MPI_DATATYPE, MPI_REPLACE, dyn_win);
            break;
        case TEST_FOP:
            MPI_Fetch_and_op(local_buf, result_buf, MPI_DATATYPE, dst, target_disp, MPI_REPLACE,
                             dyn_win);
            break;
        case TEST_CAS:
            MPI_Compare_and_swap(local_buf, compare_buf, result_buf, MPI_DATATYPE, dst, target_disp,
                                 dyn_win);
            break;
        default:
            MPI_Abort(MPI_COMM_WORLD, -1);      /* Invalid op */
            break;
    }
}

static void set_iteration_data(int x)
{
    int i;
    for (i = 0; i < rma_cnt; i++)
        local_buf[i] = rank + i + x;

    if (op == TEST_CAS) {
        /* Need equals to remote window's value */
        if (x == 0)
            compare_buf[0] = 0;
        else
            compare_buf[0] = rank + x - 1;
    }

    memset(check_buf, 0, sizeof(check_buf));
}

static int check_iteration_data(int x)
{
    int i, errs = 0;
    for (i = 0; i < rma_cnt; i++) {
        if (check_buf[i] != rank + i + x) {     /* always replace */
            error_print("rank %d (iter %d) - check %s, got buf[%d] = "
                        DATATYPE_FORMAT ", expected " DATATYPE_FORMAT "\n", rank, x,
                        rma_names[op], i, check_buf[i], rank + i + x);
            errs++;
        }
    }
    return errs;
}

static int run_test(void)
{
    int errors = 0;
    int x;
    int dst = 0;
    MPI_Aint target_disp = 0;

    dst = (rank + 1) % nprocs;

    /* reset window buffer */
    MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, dyn_win);
    memset(mem_region, 0, sizeof(DATATYPE) * MEM_CNT * 2);
    MPI_Win_unlock(rank, dyn_win);

    /* start RMA access */
    MPI_Win_lock_all(0, dyn_win);
    MPI_Barrier(MPI_COMM_WORLD);

    target_disp = mem_ptrs[dst * 2];

    for (x = 0; x < ITER; x++) {
        set_iteration_data(x);

        issue_rma_op(dst, target_disp);
        MPI_Win_flush(dst, dyn_win);

        MPI_Get(check_buf, rma_cnt, MPI_DATATYPE, dst, target_disp, rma_cnt, MPI_DATATYPE, dyn_win);
        MPI_Win_flush(dst, dyn_win);

        errors += check_iteration_data(x);
    }

    MPI_Win_unlock_all(dyn_win);

    return errors;
}

static void init_window(void)
{
    mem_ptrs = malloc(nprocs * 2 * sizeof(MPI_Aint));

    MPI_Get_address(&mem_region[0], &mem_ptrs[rank * 2]);
    MPI_Get_address(&mem_region[MEM_CNT], &mem_ptrs[rank * 2 + 1]);

    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, mem_ptrs, 2, MPI_AINT, MPI_COMM_WORLD);

    MPI_Info info = MPI_INFO_NULL;
#ifdef USE_INFO_COLL_ATTACH
    MPI_Info_create(&info);
    MPI_Info_set(info, "coll_attach", "true");
#endif

    MPI_Win_create_dynamic(info, MPI_COMM_WORLD, &dyn_win);
    MPI_Win_attach(dyn_win, &mem_region[0], sizeof(DATATYPE) * MEM_CNT);
    MPI_Win_attach(dyn_win, &mem_region[MEM_CNT], sizeof(DATATYPE) * MEM_CNT);

#ifdef USE_INFO_COLL_ATTACH
    MPI_Info_free(&info);
#endif
}

static void destroy_windows(void)
{
    MPI_Win_detach(dyn_win, &mem_region[0]);
    MPI_Win_detach(dyn_win, &mem_region[MEM_CNT]);
    MPI_Win_free(&dyn_win);
}

int main(int argc, char *argv[])
{
    int errors = 0, all_errors = 0;
    char *op_str = NULL;
    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* Processing input arguments */
    MTestArgList *head = MTestArgListCreate(argc, argv);
    rma_cnt = MTestArgListGetInt(head, "count");
    op_str = MTestArgListGetString(head, "op");

    if (rma_cnt < 0 || rma_cnt >= MAX_RMA_CNT) {
        if (rank == 0)
            error_print("Invalid input: rma_cnt = %d but valid range is [0, %d]\n",
                        rma_cnt, MAX_RMA_CNT);
        errors++;
        goto exit;
    }

    if (!strncmp(op_str, "put", strlen("put"))) {
        op = TEST_PUT;
    } else if (!strncmp(op_str, "acc", strlen("acc"))) {
        op = TEST_ACC;
    } else if (!strncmp(op_str, "gacc", strlen("gacc"))) {
        op = TEST_GACC;
    } else if (!strncmp(op_str, "fop", strlen("fop"))) {
        op = TEST_FOP;
    } else if (!strncmp(op_str, "cas", strlen("cas"))) {
        op = TEST_CAS;
    } else {
        if (rank == 0)
            error_print("Invalid op: op = %s but valid option is one of put,acc,gacc,fop,cas\n",
                        op_str);
        errors++;
        goto exit;
    }

    MTestArgListDestroy(head);

    /* Force cnt be 1 for FOP and CAS */
    if (op == TEST_FOP || op == TEST_CAS)
        rma_cnt = 1;

    init_window();
    MPI_Barrier(MPI_COMM_WORLD);

    errors = run_test();

    MPI_Barrier(MPI_COMM_WORLD);
    destroy_windows();

  exit:
    MTest_Finalize(errors);
    return MTestReturnValue(all_errors);
}
