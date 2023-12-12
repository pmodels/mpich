/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>
#include "dtpools.h"
#include "mtest_dtp.h"

#ifdef MULTI_TESTS
#define run rma_lock_x_dt
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test for streaming ACC-like operations with lock";
*/

enum acc_type {
    ACC_TYPE__ACC,
    ACC_TYPE__GACC,
};

enum {
    LOCK_TYPE__LOCK,
    LOCK_TYPE__LOCKALL,
};

enum {
    FLUSH_TYPE__NONE,
    FLUSH_TYPE__FLUSH,
    FLUSH_TYPE__FLUSH_ALL,
};

enum {
    FLUSH_LOCAL_TYPE__NONE,
    FLUSH_LOCAL_TYPE__FLUSH_LOCAL,
    FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL,
};

static int do_multi_origin = 0;
static int do_multi_target = 0;

static int flush_type = FLUSH_TYPE__NONE;
static int flush_local_type = FLUSH_LOCAL_TYPE__NONE;

static struct mtest_obj orig, target, result;
static MPI_Aint base_type_size;

static int run_test(MPI_Comm comm, MPI_Win win, int count, enum acc_type acc)
{
    int rank, size, orig_rank, target_rank;
    int target_start_idx, target_end_idx;
    MPI_Datatype origtype, targettype, resulttype;
    MPI_Aint origcount, targetcount, resultcount;
    int errs = 0;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int lock_type = LOCK_TYPE__LOCK;
    if (!do_multi_origin && !do_multi_target) {
        /* default */
        orig_rank = 0;
        target_rank = size - 1;
        target_start_idx = target_rank;
        target_end_idx = target_rank;
        lock_type = LOCK_TYPE__LOCK;
    } else if (!do_multi_origin && do_multi_target) {
        /* lockall */
        orig_rank = 0;
        if (rank == orig_rank)
            target_rank = size - 1;
        else
            target_rank = rank;
        target_start_idx = orig_rank + 1;
        target_end_idx = target_rank;
        lock_type = LOCK_TYPE__LOCKALL;
    } else if (do_multi_origin && !do_multi_target) {
        /* contention */
        target_rank = size - 1;
        if (rank == target_rank)
            orig_rank = 0;
        else
            orig_rank = rank;
        target_start_idx = target_rank;
        target_end_idx = target_rank;
        lock_type = LOCK_TYPE__LOCK;
    } else {
        printf("Unsupported MULTI_ORIGIN and MULTI_TARGET combination!\n");
        return 1;
    }

    /* we do two barriers: the first one informs the target(s) that
     * the RMA operation should be visible at the target, and the
     * second one informs the origin that the target is done
     * checking. */
    if (rank == orig_rank) {
        MTest_dtp_init(&orig, 0, 1, count);

        if (lock_type == LOCK_TYPE__LOCK) {
            MPI_Win_lock(MPI_LOCK_SHARED, target_rank, 0, win);
        } else {
            MPI_Win_lock_all(0, win);
        }

        origcount = orig.dtp_obj.DTP_type_count;
        origtype = orig.dtp_obj.DTP_datatype;
        targetcount = target.dtp_obj.DTP_type_count;
        targettype = target.dtp_obj.DTP_datatype;
        resultcount = result.dtp_obj.DTP_type_count;
        resulttype = result.dtp_obj.DTP_datatype;

        int t;
        if (acc == ACC_TYPE__ACC) {
            for (t = target_start_idx; t <= target_end_idx; t++) {
                MPI_Accumulate((char *) orig.buf + orig.dtp_obj.DTP_buf_offset, origcount,
                               origtype, t, target.dtp_obj.DTP_buf_offset / base_type_size,
                               targetcount, targettype, MPI_REPLACE, win);
            }
        } else {
            if (!do_multi_origin && !do_multi_target) {
                /* default */
                MTest_dtp_init(&result, -1, -1, count);
            }

            for (t = target_start_idx; t <= target_end_idx; t++) {
                MPI_Get_accumulate((char *) orig.buf + orig.dtp_obj.DTP_buf_offset, origcount,
                                   origtype, (char *) result.buf + result.dtp_obj.DTP_buf_offset,
                                   resultcount, resulttype, t,
                                   target.dtp_obj.DTP_buf_offset / base_type_size, targetcount,
                                   targettype, MPI_REPLACE, win);
            }
        }

        if (flush_local_type == FLUSH_LOCAL_TYPE__FLUSH_LOCAL) {
            for (t = target_start_idx; t <= target_end_idx; t++)
                MPI_Win_flush_local(t, win);
            /* reset the send buffer to test local completion */
            MTest_dtp_init(&orig, 0, 0, count);
        } else if (flush_local_type == FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL) {
            MPI_Win_flush_local_all(win);
            /* reset the send buffer to test local completion */
            MTest_dtp_init(&orig, 0, 0, count);
        }

        if (flush_type == FLUSH_TYPE__FLUSH_ALL) {
            MPI_Win_flush_all(win);
            MPI_Barrier(comm);
            MPI_Barrier(comm);
        } else if (flush_type == FLUSH_TYPE__FLUSH) {
            for (t = target_start_idx; t <= target_end_idx; t++)
                MPI_Win_flush(t, win);
            MPI_Barrier(comm);
            MPI_Barrier(comm);
        }

        if (lock_type == LOCK_TYPE__LOCK) {
            MPI_Win_unlock(target_rank, win);
        } else {
            MPI_Win_unlock_all(win);
        }

        if (flush_type == FLUSH_TYPE__NONE) {
            MPI_Barrier(comm);
            MPI_Barrier(comm);
        }

        if (acc != ACC_TYPE__ACC) {
            /* check get results */
            /* this check is not valid for multi-origin tests, as some
             * origins might receive the value that has already been
             * overwritten by other origins */
            if (!do_multi_origin && !do_multi_target) {
                errs += MTest_dtp_check(&result, 1, 2, count, &orig, errs < 10);
            }
        }
    } else if (rank == target_rank) {
        MPI_Barrier(comm);
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);

        errs += MTest_dtp_check(&target, 0, 1, count, &orig, errs < 10);
        MTest_dtp_init(&target, 1, 2, count);

        MPI_Win_unlock(rank, win);
        MPI_Barrier(comm);
    } else {
        MPI_Barrier(comm);
        MPI_Barrier(comm);
    }

    return errs;
}

static int lock_dt_test(int seed, int testsize, int count, const char *basic_type,
                        mtest_mem_type_e origmem, mtest_mem_type_e targetmem,
                        mtest_mem_type_e resultmem)
{
    int err, errs = 0;
    int rank, size;
    int minsize = 2;
    int i;
    MPI_Comm comm;
    MPI_Win win;
    DTP_pool_s dtp;

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    static char test_desc[200];
    snprintf(test_desc, 200,
             "lock_dt: -seed=%d -testsize=%d -type=%s -count=%d -origmem=%s -targetmem=%s -resultmem=%s",
             seed, testsize, basic_type, count, MTest_memtype_name(origmem),
             MTest_memtype_name(targetmem), MTest_memtype_name(resultmem));
    if (world_rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating orig pool (%s,%d)\n", basic_type, count);
        fflush(stderr);
    }

    if (MTestIsBasicDtype(dtp.DTP_base_type)) {
        MPI_Aint lb, extent;
        MPI_Type_get_extent(dtp.DTP_base_type, &lb, &extent);
        base_type_size = extent;
    } else {
        /* accumulate tests cannot use struct types */
        goto fn_exit;
    }

    MTest_dtp_obj_start(&orig, "origin", dtp, origmem, 0, false);
    MTest_dtp_obj_start(&target, "target", dtp, targetmem, 1, true);
    MTest_dtp_obj_start(&result, "result", dtp, resultmem, 2, false);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL) {
            /* for NULL comms, make sure these processes create the
             * same number of objects, so the target knows what
             * datatype layout to check for */
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, testsize);
            continue;
        }

        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        MPI_Win_create(target.buf, target.maxbufsize, base_type_size, MPI_INFO_NULL, comm, &win);

        for (i = 0; i < testsize; i++) {
            errs += MTest_dtp_create(&orig, true);
            errs += MTest_dtp_create(&target, false);
            errs += MTest_dtp_create(&result, true);

            /* do a barrier to ensure that the target buffer is
             * initialized before we start writing data to it */
            MPI_Barrier(comm);

            errs += run_test(comm, win, count, ACC_TYPE__ACC);
            errs += run_test(comm, win, count, ACC_TYPE__GACC);

            MTest_dtp_destroy(&orig);
            MTest_dtp_destroy(&target);
            MTest_dtp_destroy(&result);
        }
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    MTest_dtp_obj_finish(&orig);
    MTest_dtp_obj_finish(&target);
    MTest_dtp_obj_finish(&result);

  fn_exit:
    DTP_pool_free(dtp);
    return errs;
}

int run(const char *arg)
{
    int errs = 0;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    if (MTestArgListGetInt_with_default(head, "contention", 0)) {
        do_multi_origin = 1;
        do_multi_target = 0;
    }
    if (MTestArgListGetInt_with_default(head, "lockall", 0)) {
        do_multi_origin = 0;
        do_multi_target = 1;
    }
    if (MTestArgListGetInt_with_default(head, "flush", 0)) {
        flush_type = FLUSH_TYPE__FLUSH;
    }
    if (MTestArgListGetInt_with_default(head, "flushall", 0)) {
        flush_type = FLUSH_TYPE__FLUSH_ALL;
    }
    if (MTestArgListGetInt_with_default(head, "flushlocal", 0)) {
        flush_type = FLUSH_LOCAL_TYPE__FLUSH_LOCAL;
    }
    if (MTestArgListGetInt_with_default(head, "flushlocalall", 0)) {
        flush_type = FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL;
    }
    MTestArgListDestroy(head);

    struct dtp_args dtp_args;
    dtp_args_init_arg(&dtp_args, MTEST_DTP_GACC, arg);
    while (dtp_args_get_next(&dtp_args)) {
        errs += lock_dt_test(dtp_args.seed, dtp_args.testsize,
                             dtp_args.count, dtp_args.basic_type,
                             dtp_args.u.rma.origmem, dtp_args.u.rma.targetmem,
                             dtp_args.u.rma.resultmem);

    }
    dtp_args_finalize(&dtp_args);

    return errs;
}
