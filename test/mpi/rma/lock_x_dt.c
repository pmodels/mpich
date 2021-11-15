/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dtpools.h"
#include "mtest_dtp.h"

/*
static char MTEST_Descrip[] = "Test for streaming ACC-like operations with lock";
*/

enum acc_type {
    ACC_TYPE__ACC,
    ACC_TYPE__GACC,
};

struct mtest_obj orig, target, result;
static MPI_Aint base_type_size;

int world_rank, world_size;

static int run_test(MPI_Comm comm, MPI_Win win, int count, enum acc_type acc)
{
    int rank, size, orig_rank, target_rank;
    int target_start_idx, target_end_idx;
    MPI_Datatype origtype, targettype, resulttype;
    MPI_Aint origcount, targetcount, resultcount;
    enum {
        LOCK_TYPE__LOCK,
        LOCK_TYPE__LOCKALL,
    } lock_type;
    enum {
        FLUSH_TYPE__NONE,
        FLUSH_TYPE__FLUSH,
        FLUSH_TYPE__FLUSH_ALL,
    } flush_type;
    enum {
        FLUSH_LOCAL_TYPE__NONE,
        FLUSH_LOCAL_TYPE__FLUSH_LOCAL,
        FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL,
    } flush_local_type;
    int errs = 0;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

#if defined(SINGLE_ORIGIN) && defined(SINGLE_TARGET)
    orig_rank = 0;
    target_rank = size - 1;
    target_start_idx = target_rank;
    target_end_idx = target_rank;
    lock_type = LOCK_TYPE__LOCK;
#elif defined(SINGLE_ORIGIN) && defined(MULTI_TARGET)
    orig_rank = 0;
    if (rank == orig_rank)
        target_rank = size - 1;
    else
        target_rank = rank;
    target_start_idx = orig_rank + 1;
    target_end_idx = target_rank;
    lock_type = LOCK_TYPE__LOCKALL;
#elif defined(MULTI_ORIGIN) && defined(SINGLE_TARGET)
    target_rank = size - 1;
    if (rank == target_rank)
        orig_rank = 0;
    else
        orig_rank = rank;
    target_start_idx = target_rank;
    target_end_idx = target_rank;
    lock_type = LOCK_TYPE__LOCK;
#else
#error "SINGLE|MULTI_ORIGIN and SINGLE|MULTI_TARGET not defined"
#endif

#if defined(USE_FLUSH)
    flush_type = FLUSH_TYPE__FLUSH;
#elif defined(USE_FLUSH_ALL)
    flush_type = FLUSH_TYPE__FLUSH_ALL;
#elif defined(USE_FLUSH_NONE)
    flush_type = FLUSH_TYPE__NONE;
#else
#error "FLUSH_TYPE not defined"
#endif

#if defined(USE_FLUSH_LOCAL)
    flush_local_type = FLUSH_LOCAL_TYPE__FLUSH_LOCAL;
#elif defined(USE_FLUSH_LOCAL_ALL)
    flush_local_type = FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL;
#elif defined(USE_FLUSH_LOCAL_NONE)
    flush_local_type = FLUSH_LOCAL_TYPE__NONE;
#else
#error "FLUSH_LOCAL_TYPE not defined"
#endif

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
#if !defined(MULTI_ORIGIN) && !defined(MULTI_TARGET)
            MTest_dtp_init(&result, -1, -1, count);
#endif

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
#if !defined(MULTI_ORIGIN) && !defined(MULTI_TARGET)
            /* check get results */
            /* this check is not valid for multi-origin tests, as some
             * origins might receive the value that has already been
             * overwritten by other origins */
            errs += MTest_dtp_check(&result, 1, 2, count, &orig, errs < 10);
#endif
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

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_DTP_GACC, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += lock_dt_test(dtp_args.seed, dtp_args.testsize,
                             dtp_args.count, dtp_args.basic_type,
                             dtp_args.u.rma.origmem, dtp_args.u.rma.targetmem,
                             dtp_args.u.rma.resultmem);

    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
