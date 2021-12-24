/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>
#include "rma_test_op.h"

#ifdef MULTI_TESTS
#define run rma_win_dynamic_rma_flush_get
int run(const char *arg);
#endif

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

static int do_collattach = 0;

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

/* Issue functions for different RMA operations */
static void issue_rma_op(int dst, MPI_Aint target_disp)
{
    switch (test_op) {
        case TEST_OP_PUT:
            MPI_Put(local_buf, rma_cnt, MPI_DATATYPE, dst, target_disp, rma_cnt, MPI_DATATYPE,
                    dyn_win);
            break;
        case TEST_OP_ACC:
            MPI_Accumulate(local_buf, rma_cnt, MPI_DATATYPE, dst, target_disp, rma_cnt,
                           MPI_DATATYPE, MPI_REPLACE, dyn_win);
            break;
        case TEST_OP_GACC:
            MPI_Get_accumulate(local_buf, rma_cnt, MPI_DATATYPE, result_buf, rma_cnt, MPI_DATATYPE,
                               dst, target_disp, rma_cnt, MPI_DATATYPE, MPI_REPLACE, dyn_win);
            break;
        case TEST_OP_FOP:
            MPI_Fetch_and_op(local_buf, result_buf, MPI_DATATYPE, dst, target_disp, MPI_REPLACE,
                             dyn_win);
            break;
        case TEST_OP_CAS:
            MPI_Compare_and_swap(local_buf, compare_buf, result_buf, MPI_DATATYPE, dst, target_disp,
                                 dyn_win);
            break;
        default:
            break;
    }
}

static void set_iteration_data(int x)
{
    int i;
    for (i = 0; i < rma_cnt; i++)
        local_buf[i] = rank + i + x;

    if (test_op == TEST_OP_CAS) {
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
                        get_rma_name(), i, check_buf[i], rank + i + x);
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
    if (do_collattach) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "coll_attach", "true");
    }

    MPI_Win_create_dynamic(info, MPI_COMM_WORLD, &dyn_win);
    MPI_Win_attach(dyn_win, &mem_region[0], sizeof(DATATYPE) * MEM_CNT);
    MPI_Win_attach(dyn_win, &mem_region[MEM_CNT], sizeof(DATATYPE) * MEM_CNT);

    if (info != MPI_INFO_NULL) {
        MPI_Info_free(&info);
    }
}

static void destroy_windows(void)
{
    MPI_Win_detach(dyn_win, &mem_region[0]);
    MPI_Win_detach(dyn_win, &mem_region[MEM_CNT]);
    MPI_Win_free(&dyn_win);
}

int run(const char *arg)
{
    int errors = 0;
    char *op_str = NULL;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* Processing input arguments */
    MTestArgList *head = MTestArgListCreate_arg(arg);
    parse_test_op(head);
    do_collattach = MTestArgListGetInt_with_default(head, "collattach", 0);
    rma_cnt = MTestArgListGetInt(head, "count");

    if (rma_cnt < 0 || rma_cnt >= MAX_RMA_CNT) {
        if (rank == 0)
            error_print("Invalid input: rma_cnt = %d but valid range is [0, %d]\n",
                        rma_cnt, MAX_RMA_CNT);
        errors++;
        goto exit;
    }

    assert_test_op();

    MTestArgListDestroy(head);

    /* Force cnt be 1 for FOP and CAS */
    if (test_op == TEST_OP_FOP || test_op == TEST_OP_CAS) {
        rma_cnt = 1;
    }

    init_window();
    MPI_Barrier(MPI_COMM_WORLD);

    errors = run_test();

    MPI_Barrier(MPI_COMM_WORLD);
    destroy_windows();

  exit:
    return errors;
}
